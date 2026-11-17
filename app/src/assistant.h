#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <atomic>
#include <cctype>
namespace hext {
enum class AssistantTask {Chat,Version,Capabilities,Authentication};
enum class ConnectionState {Disconnected,Checking,Connected,Missing,NeedsLogin,Incompatible,Error};
inline std::atomic<unsigned long long> assistantSequence{0};
inline std::string lower(std::string value){
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return value;
}
inline bool providerVersion(int provider,const std::string& output){
    auto value=lower(output);
    return value.find(provider==1?"codex":"claude")!=std::string::npos&&
        value.find_first_of("0123456789")!=std::string::npos;
}
inline bool providerCapabilities(int provider,const std::string& output){
    if(provider==1)return output.find("--output-last-message")!=std::string::npos&&
        output.find("--sandbox")!=std::string::npos&&output.find("--ephemeral")!=std::string::npos;
    return output.find("--output-format")!=std::string::npos&&
        output.find("--permission-mode")!=std::string::npos&&output.find("--no-session-persistence")!=std::string::npos;
}
inline bool providerAuthenticated(int provider,DWORD code,const std::string& output){
    if(code)return false;
    auto value=lower(output);
    if(provider==1)return value.find("logged in")!=std::string::npos&&value.find("not logged in")==std::string::npos;
    value.erase(std::remove_if(value.begin(),value.end(),[](unsigned char c){return std::isspace(c);}),value.end());
    if(value.find("\"loggedin\":false")!=std::string::npos)return false;
    return value.find("\"loggedin\":true")!=std::string::npos||
        value.find("\"authmethod\":\"claude.ai\"")!=std::string::npos||
        value.find("\"authmethod\":\"oauth_token\"")!=std::string::npos||
        value.find("\"authmethod\":\"api_key\"")!=std::string::npos||
        value.find("\"authmethod\":\"api_key_helper\"")!=std::string::npos||
        value.find("\"authmethod\":\"third_party\"")!=std::string::npos;
}
inline std::wstring quoteArgument(const std::wstring& value){
    std::wstring out=L"\"";size_t slashes=0;
    for(auto c:value){
        if(c==L'\\'){slashes++;continue;}
        if(c==L'"'){out.append(slashes*2+1,L'\\');out+=c;}
        else{out.append(slashes,L'\\');out+=c;}
        slashes=0;
    }
    out.append(slashes*2,L'\\');return out+L"\"";
}
inline std::string utf8(const std::wstring& text){
    if(text.empty())return {};
    int size=WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    std::string out(size,0);WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size,nullptr,nullptr);
    return out;
}
inline std::wstring wide(const std::string& text){
    if(text.empty())return {};
    int size=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring out(size,0);MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size);
    return out;
}
struct AssistantBridge {
    HANDLE process=nullptr,job=nullptr,output=nullptr,error=nullptr;
    bool busy=false;DWORD exitCode=0;ULONGLONG started=0,timeoutMs=120000;
    std::string result,diagnostic;
    std::filesystem::path answerFile;
    ~AssistantBridge(){cancel();}
    static void close(HANDLE& h){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);h=nullptr;}
    void cancel(){
        if(job)TerminateJobObject(job,1);
        close(process);close(job);close(output);close(error);busy=false;
        if(!answerFile.empty()){DeleteFileW(answerFile.c_str());answerFile.clear();}
    }
    static void drain(HANDLE pipe,std::string& target){
        char bytes[4096];DWORD available=0,read=0;
        int chunks=0;
        while(pipe&&chunks++<16&&PeekNamedPipe(pipe,nullptr,0,nullptr,&available,nullptr)&&available){
            if(!ReadFile(pipe,bytes,std::min<DWORD>(available,sizeof(bytes)),&read,nullptr)||!read)break;
            if(target.size()<1024*1024)target.append(bytes,std::min<size_t>(read,1024*1024-target.size()));
        }
    }
    bool start(const std::wstring& executable,int provider,const std::wstring& prompt,
               const std::filesystem::path& work,AssistantTask task=AssistantTask::Chat){
        cancel();result.clear();diagnostic.clear();exitCode=0;
        std::error_code ec;std::filesystem::create_directories(work,ec);
        if(ec||!std::filesystem::is_regular_file(executable,ec))return false;
        SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
        auto tag=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++assistantSequence);
        auto inputFile=work/(L".hext-input-"+tag+L".tmp");
        HANDLE input=CreateFileW(inputFile.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_DELETE,
            &security,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
        if(input==INVALID_HANDLE_VALUE)return false;
        auto bytes=utf8(prompt);DWORD written=0;
        if(!WriteFile(input,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)||written!=bytes.size()){
            CloseHandle(input);return false;
        }
        SetFilePointer(input,0,nullptr,FILE_BEGIN);
        HANDLE outputWrite=nullptr,errorWrite=nullptr;
        if(!CreatePipe(&output,&outputWrite,&security,65536)||!CreatePipe(&error,&errorWrite,&security,65536)){
            CloseHandle(input);close(outputWrite);close(errorWrite);cancel();return false;
        }
        SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);SetHandleInformation(error,HANDLE_FLAG_INHERIT,0);
        std::vector<std::wstring> args;
        timeoutMs=task==AssistantTask::Chat?120000:15000;
        if(task==AssistantTask::Version)args={L"--version"};
        else if(task==AssistantTask::Capabilities)args=provider==1?
            std::vector<std::wstring>{L"exec",L"--help"}:std::vector<std::wstring>{L"--help"};
        else if(task==AssistantTask::Authentication)args=provider==1?
            std::vector<std::wstring>{L"login",L"status"}:std::vector<std::wstring>{L"auth",L"status"};
        else if(provider==1){
            answerFile=work/(L".hext-answer-"+tag+L".txt");
            args={L"exec",L"--skip-git-repo-check",L"--sandbox",L"read-only",L"--ephemeral",
                  L"--color",L"never",L"--output-last-message",answerFile.wstring(),L"-"};
        }else if(provider==2)args={L"-p",L"--output-format",L"text",L"--permission-mode",L"plan",L"--no-session-persistence"};
        std::wstring command=quoteArgument(executable);
        for(const auto& arg:args)command+=L" "+quoteArgument(arg);
        STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
        startup.hStdInput=input;startup.hStdOutput=outputWrite;startup.hStdError=errorWrite;
        PROCESS_INFORMATION info{};
        job=CreateJobObjectW(nullptr,nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};
        limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        bool ready=job&&SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limit,sizeof(limit));
        BOOL created=ready&&CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,
            CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,work.c_str(),&startup,&info);
        CloseHandle(input);close(outputWrite);close(errorWrite);
        if(!created){cancel();return false;}
        process=info.hProcess;
        if(!AssignProcessToJobObject(job,process)){
            TerminateProcess(process,1);CloseHandle(info.hThread);cancel();return false;
        }
        ResumeThread(info.hThread);CloseHandle(info.hThread);
        busy=true;started=GetTickCount64();return true;
    }
    bool poll(){
        if(!busy)return false;
        drain(output,result);drain(error,diagnostic);
        if(GetTickCount64()-started>timeoutMs){cancel();exitCode=WAIT_TIMEOUT;return true;}
        if(WaitForSingleObject(process,0)!=WAIT_OBJECT_0)return false;
        drain(output,result);drain(error,diagnostic);GetExitCodeProcess(process,&exitCode);
        if(!answerFile.empty()){
            std::ifstream file(answerFile,std::ios::binary);
            if(file){
                result.resize(1024*1024);file.read(result.data(),static_cast<std::streamsize>(result.size()));
                result.resize(static_cast<size_t>(file.gcount()));
            }
        }
        close(process);close(job);close(output);close(error);busy=false;
        if(!answerFile.empty()){DeleteFileW(answerFile.c_str());answerFile.clear();}
        return true;
    }
};
}
