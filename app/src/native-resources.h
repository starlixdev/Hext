#pragma once
#include "personal-net.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <thread>
namespace hext {
inline std::wstring resourceLower(std::wstring value){if(!value.empty())CharLowerBuffW(value.data(),static_cast<DWORD>(value.size()));return value;}
inline bool discoverableApplication(const std::filesystem::path& path){
    if(!safeAsset(path,true))return false;
    auto name=resourceLower(path.filename().wstring());
    for(auto blocked:{L"cmd.exe",L"powershell.exe",L"pwsh.exe",L"wscript.exe",L"cscript.exe",L"mshta.exe",L"rundll32.exe",L"regsvr32.exe",L"msiexec.exe",L"reg.exe"})if(name==blocked)return false;
    for(auto part:{L"unins",L"uninstall",L"setup.exe",L"installer"})if(name.find(part)!=std::wstring::npos)return false;
    return true;
}
class ApplicationDiscovery {
    std::thread worker;std::atomic<bool> running{false},cancelled{false};mutable std::mutex mutex;
    std::vector<Json> entries;bool finished=false;std::string search;
public:
    ~ApplicationDiscovery(){cancelled=true;if(worker.joinable())worker.join();}
    bool busy()const{return running;}
    size_t size()const{std::lock_guard<std::mutex> lock(mutex);return entries.size();}
    void cancel(){cancelled=true;std::lock_guard<std::mutex> lock(mutex);finished=false;}
    std::vector<Json> snapshot()const{std::lock_guard<std::mutex> lock(mutex);return entries;}
    std::optional<Json> find(const std::string& id)const{std::lock_guard<std::mutex> lock(mutex);for(auto& item:entries)if(item.value("id","")==id)return item;return {};}
    std::optional<Json> take(){std::lock_guard<std::mutex> lock(mutex);if(!finished)return {};finished=false;Json matches=Json::array();auto query=resourceLower(wide(search));for(auto& e:entries){auto name=resourceLower(wide(e.value("name","")));auto stem=resourceLower(std::filesystem::path(wide(e.value("path",""))).stem().wstring());bool match=query.empty()||name.find(query)!=std::wstring::npos||stem.find(query)!=std::wstring::npos||(stem.size()>=3&&query.find(stem)!=std::wstring::npos);if(match&&matches.size()<10)matches.push_back({{"id",e["id"]},{"name",e["name"]}});}return matches;}
    bool start(const std::string& query){
        if(running.exchange(true))return false;
        if(worker.joinable())worker.join();
        cancelled=false;
        {std::lock_guard<std::mutex> lock(mutex);finished=false;search=query;}
        worker=std::thread([this]{std::vector<Json> found;std::set<std::wstring> seen;auto deadline=GetTickCount64()+8000;
            auto add=[&](std::filesystem::path path,const std::wstring& title){
                if(cancelled||found.size()>=300||!discoverableApplication(path))return;
                std::error_code error;path=std::filesystem::canonical(path,error);if(error)return;auto key=resourceLower(path.wstring());if(!seen.insert(key).second){for(auto& item:found)if(resourceLower(wide(item.value("path","")))==key&&title.size()>wide(item.value("name","")).size()){item["name"]=utf8(title);break;}return;}
                auto id="app-"+pkceChallenge(utf8(resourceLower(path.wstring()))).substr(0,36);
                found.push_back({{"id",id},{"name",utf8(title.empty()?path.stem().wstring():title)},{"path",utf8(path.wstring())},{"app",true},{"stamp",std::filesystem::last_write_time(path,error).time_since_epoch().count()}});
            };
            try{
                for(auto hive:{HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE})for(auto view:{KEY_WOW64_64KEY,KEY_WOW64_32KEY}){
                    HKEY root=nullptr;if(RegOpenKeyExW(hive,L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths",0,KEY_READ|view,&root)!=ERROR_SUCCESS)continue;
                    for(DWORD index=0;index<300&&!cancelled&&GetTickCount64()<deadline;index++){wchar_t name[512]{};DWORD size=512;if(RegEnumKeyExW(root,index,name,&size,nullptr,nullptr,nullptr,nullptr)!=ERROR_SUCCESS)break;wchar_t path[32768]{};DWORD bytes=sizeof(path);
                        if(RegGetValueW(root,name,nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,path,&bytes)==ERROR_SUCCESS){std::wstring p=path;if(p.size()>1&&p.front()==L'"'&&p.back()==L'"')p=p.substr(1,p.size()-2);add(p,std::filesystem::path(name).stem().wstring());}}
                    RegCloseKey(root);
                }
                auto apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
                if(SUCCEEDED(apartment)){
                    for(auto folder:{FOLDERID_Programs,FOLDERID_CommonPrograms}){
                        PWSTR location=nullptr;if(FAILED(SHGetKnownFolderPath(folder,KF_FLAG_DONT_VERIFY,nullptr,&location)))continue;std::filesystem::path root(location);CoTaskMemFree(location);std::error_code error;
                        std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,error),end;
                        for(;it!=end&&!error&&!cancelled&&GetTickCount64()<deadline;it.increment(error)){
                            if(it.depth()>5||(GetFileAttributesW(it->path().c_str())&FILE_ATTRIBUTE_REPARSE_POINT)){it.disable_recursion_pending();continue;}if(resourceLower(it->path().extension().wstring())!=L".lnk")continue;
                            IShellLinkW* link=nullptr;if(FAILED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_IShellLinkW,reinterpret_cast<void**>(&link))))continue;IPersistFile* file=nullptr;
                            if(SUCCEEDED(link->QueryInterface(IID_IPersistFile,reinterpret_cast<void**>(&file)))){
                                if(SUCCEEDED(file->Load(it->path().c_str(),STGM_READ))){wchar_t target[32768]{},arguments[32768]{};WIN32_FIND_DATAW data{};
                                    // Never resolve network links or execute shortcut-provided arguments.
                                    if(SUCCEEDED(link->GetArguments(arguments,32768))&&!arguments[0]&&SUCCEEDED(link->GetPath(target,32768,&data,SLGP_RAWPATH)))add(target,it->path().stem().wstring());}
                                file->Release();
                            }link->Release();
                        }
                    }CoUninitialize();
                }
            }catch(...){}
            {std::lock_guard<std::mutex> lock(mutex);if(!cancelled){entries=std::move(found);finished=true;}}running=false;
        });return true;
    }
};
inline bool plainTextFile(const std::filesystem::path& path){auto extension=resourceLower(path.extension().wstring());return extension==L".txt"||extension==L".md"||extension==L".csv"||extension==L".json"||extension==L".yaml"||extension==L".yml"||extension==L".log";}
inline std::optional<std::string> readAuthorizedText(const std::filesystem::path& path){
    if(!safeAsset(path,false)||!plainTextFile(path)||std::filesystem::canonical(path)!=path)return {};
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file==INVALID_HANDLE_VALUE)return {};
    LARGE_INTEGER length{};std::string value;DWORD count=0;bool ok=GetFileSizeEx(file,&length)&&length.QuadPart>=0&&length.QuadPart<=32768;
    if(ok){value.resize(static_cast<size_t>(length.QuadPart));ok=ReadFile(file,value.data(),static_cast<DWORD>(value.size()),&count,nullptr)&&count==value.size();}CloseHandle(file);
    if(!ok||value.find('\0')!=std::string::npos)return {};
    if(value.rfind("\xEF\xBB\xBF",0)==0)value.erase(0,3);
    if(!value.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0))return {};
    for(unsigned char c:value)if(c<32&&c!=9&&c!=10&&c!=13)return {};
    return value;
}
inline bool safeTextName(const std::string& value){
    if(value.empty()||value.size()>128||value.back()==' '||value.back()=='.')return false;
    for(unsigned char c:value)if(c<32||std::string("/\\:*?\"<>|").find(static_cast<char>(c))!=std::string::npos)return false;
    auto name=std::filesystem::path(wide(value));auto ext=resourceLower(name.extension().wstring());if(ext!=L".txt"&&ext!=L".md")return false;auto stem=resourceLower(name.stem().wstring());stem=stem.substr(0,stem.find(L'.'));
    for(auto forbidden:{L"con",L"prn",L"aux",L"nul",L"com1",L"com2",L"com3",L"com4",L"com5",L"com6",L"com7",L"com8",L"com9",L"lpt1",L"lpt2",L"lpt3",L"lpt4",L"lpt5",L"lpt6",L"lpt7",L"lpt8",L"lpt9"})if(stem==forbidden)return false;
    return value.front()!='.';
}
inline std::optional<std::filesystem::path> createAuthorizedText(const std::filesystem::path& folder,const std::string& name,const std::string& contents){
    if(contents.size()>16000||contents.find('\0')!=std::string::npos||!safeTextName(name)||!safeAsset(folder,false)||!std::filesystem::is_directory(folder)||std::filesystem::canonical(folder)!=folder)return {};
    for(unsigned char c:contents)if(c<32&&c!=9&&c!=10&&c!=13)return {};
    if(!contents.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,contents.data(),static_cast<int>(contents.size()),nullptr,0))return {};
    auto target=folder/wide(name),temporary=folder/(L".hext-"+wide(randomID())+L".tmp");
    HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file==INVALID_HANDLE_VALUE)return {};
    DWORD written=0;bool ok=WriteFile(file,contents.data(),static_cast<DWORD>(contents.size()),&written,nullptr)&&written==contents.size()&&FlushFileBuffers(file);CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    if(!ok){DeleteFileW(temporary.c_str());return {};}auto verify=readAuthorizedText(target);if(!verify||*verify!=contents)return {};return target;
}
}
