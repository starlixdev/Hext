#pragma once
#include "personal-core.h"
#include <userenv.h>
#include <thread>
#include <mutex>
namespace hext {
struct AgentProcessResult {DWORD code=ERROR_GEN_FAILURE;std::string output,error;bool timedOut=false,cancelled=false,truncated=false;};
struct OwnedHandle {
    HANDLE value=nullptr;OwnedHandle()=default;explicit OwnedHandle(HANDLE h):value(h){}~OwnedHandle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    OwnedHandle(const OwnedHandle&)=delete;OwnedHandle& operator=(const OwnedHandle&)=delete;
};
inline std::wstring systemExecutable(const wchar_t* relative){wchar_t path[MAX_PATH]{};if(!GetSystemDirectoryW(path,MAX_PATH))return {};return (std::filesystem::path(path)/relative).wstring();}
inline std::wstring encodedPowerShell(const std::string& script){
    auto text=wide("[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);$OutputEncoding=[Console]::OutputEncoding;$ProgressPreference='SilentlyContinue';$ErrorActionPreference='Stop';$env:TEMP=$env:HEXT_WORKSPACE;$env:TMP=$env:HEXT_WORKSPACE;[Environment]::CurrentDirectory=$env:HEXT_WORKSPACE;New-PSDrive -Name Hext -PSProvider FileSystem -Root $env:TEMP -Scope Global|Out-Null;Set-Location 'Hext:\\';"+script);
    DWORD size=0;CryptBinaryToStringW(reinterpret_cast<const BYTE*>(text.data()),static_cast<DWORD>(text.size()*2),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size);
    std::wstring encoded(size,L'\0');if(!CryptBinaryToStringW(reinterpret_cast<const BYTE*>(text.data()),static_cast<DWORD>(text.size()*2),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,encoded.data(),&size))throw std::runtime_error("Script encoding failed");while(!encoded.empty()&&encoded.back()==0)encoded.pop_back();return encoded;
}
inline bool agentID(const std::string& id){return id.size()==32&&std::all_of(id.begin(),id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
struct AgentContainer {
    PSID sid=nullptr;std::filesystem::path workspace;std::wstring name;
    AgentContainer()=default;AgentContainer(const AgentContainer&)=delete;~AgentContainer(){if(sid)FreeSid(sid);}
    bool open(const std::string& id){
        if(!agentID(id))return false;
        name=L"Hext.Task."+wide(id);
        auto result=CreateAppContainerProfile(name.c_str(),L"Hext isolated task",L"No network capabilities; task-local workspace",nullptr,0,&sid);
        if(result==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))result=DeriveAppContainerSidFromAppContainerName(name.c_str(),&sid);
        if(FAILED(result)||!sid)return false;
        LPWSTR stringSid=nullptr,folder=nullptr;if(!ConvertSidToStringSidW(sid,&stringSid))return false;
        result=GetAppContainerFolderPath(stringSid,&folder);LocalFree(stringSid);if(FAILED(result)||!folder)return false;
        workspace=std::filesystem::path(folder)/L"LocalState";CoTaskMemFree(folder);
        std::error_code error;std::filesystem::create_directories(workspace,error);if(error)return false;
        auto attributes=GetFileAttributesW(workspace.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        // Grant the unique container SID only on this Hext-owned workspace, never
        // on a user-selected folder. Low-integrity children need its low label.
        LPWSTR containerSid=nullptr;if(!ConvertSidToStringSidW(sid,&containerSid))return false;
        PACL previous=nullptr,updated=nullptr;PSECURITY_DESCRIPTOR descriptor=nullptr;
        auto code=GetNamedSecurityInfoW(const_cast<wchar_t*>(workspace.c_str()),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&previous,nullptr,&descriptor);
        EXPLICIT_ACCESSW entry{};entry.grfAccessPermissions=FILE_ALL_ACCESS;entry.grfAccessMode=GRANT_ACCESS;entry.grfInheritance=SUB_CONTAINERS_AND_OBJECTS_INHERIT;entry.Trustee.TrusteeForm=TRUSTEE_IS_SID;entry.Trustee.TrusteeType=TRUSTEE_IS_UNKNOWN;entry.Trustee.ptstrName=reinterpret_cast<LPWSTR>(sid);
        if(code==ERROR_SUCCESS)code=SetEntriesInAclW(1,&entry,previous,&updated);
        if(code==ERROR_SUCCESS)code=SetNamedSecurityInfoW(const_cast<wchar_t*>(workspace.c_str()),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,updated,nullptr);
        if(updated)LocalFree(updated);
        if(descriptor)LocalFree(descriptor);
        LocalFree(containerSid);if(code!=ERROR_SUCCESS)return false;
        PSECURITY_DESCRIPTOR label=nullptr;if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"S:(ML;OICI;NW;;;LW)",SDDL_REVISION_1,&label,nullptr))return false;PACL sacl=nullptr;BOOL present=FALSE,defaulted=FALSE;GetSecurityDescriptorSacl(label,&present,&sacl,&defaulted);
        code=SetNamedSecurityInfoW(const_cast<wchar_t*>(workspace.c_str()),SE_FILE_OBJECT,LABEL_SECURITY_INFORMATION,nullptr,nullptr,nullptr,sacl);LocalFree(label);return code==ERROR_SUCCESS;
    }
};
inline AgentProcessResult agentProcess(const std::wstring& executable,const std::vector<std::wstring>& arguments,const std::filesystem::path& work,std::atomic<bool>& cancel,DWORD timeout=30000,PSID container=nullptr,const std::string& input="",size_t outputLimit=65536){
    outputLimit=std::clamp<size_t>(outputLimit,65536,1048576);
    AgentProcessResult result;if(cancel){result.cancelled=true;result.code=ERROR_CANCELLED;return result;}
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};OwnedHandle readOut,writeOut,readErr,writeErr,readIn,writeIn,job;
    if(!CreatePipe(&readOut.value,&writeOut.value,&sa,0)||!CreatePipe(&readErr.value,&writeErr.value,&sa,0)||!CreatePipe(&readIn.value,&writeIn.value,&sa,0)){result.code=GetLastError();return result;}
    SetHandleInformation(readOut.value,HANDLE_FLAG_INHERIT,0);SetHandleInformation(readErr.value,HANDLE_FLAG_INHERIT,0);SetHandleInformation(writeIn.value,HANDLE_FLAG_INHERIT,0);
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;startup.StartupInfo.wShowWindow=SW_HIDE;
    startup.StartupInfo.hStdInput=readIn.value;startup.StartupInfo.hStdOutput=writeOut.value;startup.StartupInfo.hStdError=writeErr.value;
    SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,container?2:1,0,&bytes);std::vector<BYTE> attributes(bytes);startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if(!InitializeProcThreadAttributeList(startup.lpAttributeList,container?2:1,0,&bytes)){result.code=GetLastError();return result;}
    struct Cleanup{LPPROC_THREAD_ATTRIBUTE_LIST p;~Cleanup(){DeleteProcThreadAttributeList(p);}}cleanup{startup.lpAttributeList};
    HANDLE inherited[]={readIn.value,writeOut.value,writeErr.value};
    if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr)){result.code=GetLastError();return result;}
    SECURITY_CAPABILITIES capabilities{};capabilities.AppContainerSid=container;
    if(container&&!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,&capabilities,sizeof(capabilities),nullptr,nullptr)){result.code=GetLastError();return result;}
    job.value=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_ACTIVE_PROCESS|JOB_OBJECT_LIMIT_JOB_MEMORY;limits.BasicLimitInformation.ActiveProcessLimit=8;limits.JobMemoryLimit=384ULL*1024*1024;
    if(!job.value||!SetInformationJobObject(job.value,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){result.code=GetLastError();return result;}
    if(container){JOBOBJECT_BASIC_UI_RESTRICTIONS ui{JOB_OBJECT_UILIMIT_ALL};if(!SetInformationJobObject(job.value,JobObjectBasicUIRestrictions,&ui,sizeof(ui))){result.code=GetLastError();return result;}}
    auto command=quoteArgument(executable);for(auto& arg:arguments)command+=L" "+quoteArgument(arg);std::vector<wchar_t> mutableCommand(command.begin(),command.end());mutableCommand.push_back(0);
    wchar_t windows[MAX_PATH]{},local[32768]{};GetWindowsDirectoryW(windows,MAX_PATH);GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
    // Windows needs the real per-user profile location to resolve the lowbox namespace.
    // Knowing this path grants no access; the AppContainer token remains the boundary.
    std::wstring env=L"COMSPEC="+systemExecutable(L"cmd.exe")+L'\0'+L"HEXT_WORKSPACE="+work.wstring()+L'\0'+L"LOCALAPPDATA="+local+L'\0'+L"PATH="+systemExecutable(L"")+L";"+systemExecutable(L"WindowsPowerShell/v1.0")+L'\0'+L"SystemDrive="+std::filesystem::path(windows).root_name().wstring()+L'\0'+L"SystemRoot="+windows+L'\0'+L"TEMP="+work.wstring()+L'\0'+L"TMP="+work.wstring()+L'\0'+L"USERPROFILE="+work.wstring()+L'\0'+L"windir="+windows+L'\0';env.push_back(0);
    PROCESS_INFORMATION process{};DWORD flags=CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT|CREATE_UNICODE_ENVIRONMENT;
    if(!CreateProcessW(executable.c_str(),mutableCommand.data(),nullptr,nullptr,TRUE,flags,container?env.data():nullptr,work.empty()?nullptr:work.c_str(),&startup.StartupInfo,&process)){result.code=GetLastError();result.error="CreateProcessW failed: "+std::to_string(result.code);return result;}
    OwnedHandle child(process.hProcess),thread(process.hThread);
    if(!AssignProcessToJobObject(job.value,child.value)){result.code=GetLastError();TerminateProcess(child.value,result.code);return result;}
    CloseHandle(readIn.value);readIn.value=nullptr;CloseHandle(writeOut.value);writeOut.value=nullptr;CloseHandle(writeErr.value);writeErr.value=nullptr;
    if(cancel){TerminateJobObject(job.value,ERROR_CANCELLED);result.cancelled=true;result.code=ERROR_CANCELLED;return result;}
    ResumeThread(thread.value);
    if(!input.empty()){DWORD wrote=0;if(input.size()>8192||!WriteFile(writeIn.value,input.data(),static_cast<DWORD>(input.size()),&wrote,nullptr)||wrote!=input.size()){TerminateJobObject(job.value,1);result.code=ERROR_WRITE_FAULT;return result;}}
    CloseHandle(writeIn.value);writeIn.value=nullptr;auto deadline=GetTickCount64()+timeout;
    auto drain=[&](HANDLE h,std::string& target){char buffer[4096];DWORD available=0,n=0;for(int i=0;i<16&&PeekNamedPipe(h,nullptr,0,nullptr,&available,nullptr)&&available;i++){if(!ReadFile(h,buffer,std::min<DWORD>(available,sizeof(buffer)),&n,nullptr)||!n)break;auto count=std::min<size_t>(n,outputLimit-target.size());target.append(buffer,count);if(count<n)result.truncated=true;}};
    for(;;){drain(readOut.value,result.output);drain(readErr.value,result.error);if(WaitForSingleObject(child.value,10)==WAIT_OBJECT_0)break;if(cancel||GetTickCount64()>=deadline||result.truncated){result.cancelled=cancel.load();result.timedOut=!result.cancelled&&!result.truncated;TerminateJobObject(job.value,1);WaitForSingleObject(child.value,1500);break;}}
    drain(readOut.value,result.output);drain(readErr.value,result.error);GetExitCodeProcess(child.value,&result.code);return result;
}
inline AgentProcessResult sandboxPowerShell(const std::string& id,const std::string& script,std::atomic<bool>& cancel){
    AgentContainer sandbox;if(!sandbox.open(id)){AgentProcessResult result;result.code=ERROR_ACCESS_DENIED;result.error="AppContainer unavailable; no unrestricted fallback was run.";return result;}
    return agentProcess(systemExecutable(L"WindowsPowerShell/v1.0/powershell.exe"),{L"-NoLogo",L"-NoProfile",L"-NonInteractive",L"-EncodedCommand",encodedPowerShell(script)},sandbox.workspace,cancel,45000,sandbox.sid);
}
class AgentWorker {
    std::thread worker;std::mutex mutex;std::atomic<bool> stop{false},active{false};std::optional<Json> completed;
public:
    ~AgentWorker(){cancel();if(worker.joinable())worker.join();}
    bool busy()const{return active;}
    void cancel(){stop=true;std::lock_guard<std::mutex> lock(mutex);completed.reset();}
    bool start(std::function<Json(std::atomic<bool>&)> operation){if(active)return false;if(worker.joinable())worker.join();stop=false;active=true;{std::lock_guard<std::mutex> lock(mutex);completed.reset();}worker=std::thread([this,operation]{Json result;try{result=operation(stop);}catch(const std::exception&){result={{"success",false},{"error","Native operation failed"}};}catch(...){result={{"success",false},{"error","Native operation failed"}};}std::lock_guard<std::mutex> lock(mutex);if(!stop)completed=std::move(result);active=false;});return true;}
    std::optional<Json> poll(){std::lock_guard<std::mutex> lock(mutex);auto value=std::move(completed);completed.reset();return value;}
};
}
