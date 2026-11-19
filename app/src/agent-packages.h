#pragma once
#include "agent-process.h"
#include <appmodel.h>
#include <wintrust.h>
#include <softpub.h>
#include <regex>
namespace hext {
inline bool packageToken(const std::string& id,size_t maximum){return !id.empty()&&id.size()<=maximum&&id[0]!='-'&&std::all_of(id.begin(),id.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='+'||c=='-';});}
inline std::wstring verifiedWinGet(){
    UINT32 count=0,size=0;auto code=GetPackagesByPackageFamily(L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe",&count,nullptr,&size,nullptr);
    if(code!=ERROR_INSUFFICIENT_BUFFER||!count||count>16||size>32768)return {};
    std::vector<PWSTR> names(count);std::vector<wchar_t> strings(size);
    if(GetPackagesByPackageFamily(L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe",&count,names.data(),&size,strings.data())!=ERROR_SUCCESS)return {};
    for(UINT32 i=0;i<count;i++){
        UINT32 length=0;if(GetPackagePathByFullName(names[i],&length,nullptr)!=ERROR_INSUFFICIENT_BUFFER||length>32768)continue;std::vector<wchar_t> path(length);
        if(GetPackagePathByFullName(names[i],&length,path.data())!=ERROR_SUCCESS)continue;
        auto executable=std::filesystem::path(path.data())/L"winget.exe";if(!std::filesystem::exists(executable))continue;
        WINTRUST_FILE_INFO file{sizeof(file),executable.c_str(),nullptr,nullptr};WINTRUST_DATA trust{};trust.cbStruct=sizeof(trust);trust.dwUIChoice=WTD_UI_NONE;trust.fdwRevocationChecks=WTD_REVOKE_NONE;trust.dwUnionChoice=WTD_CHOICE_FILE;trust.pFile=&file;trust.dwStateAction=WTD_STATEACTION_VERIFY;trust.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL;GUID policy=WINTRUST_ACTION_GENERIC_VERIFY_V2;auto verified=WinVerifyTrust(nullptr,&policy,&trust);trust.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&policy,&trust);if(verified==ERROR_SUCCESS)return executable.wstring();
    }return {};
}
inline bool officialWinGetSource(const std::wstring& executable,std::atomic<bool>& cancel){
    auto result=agentProcess(executable,{L"source",L"export",L"--name",L"winget",L"--disable-interactivity"},std::filesystem::temp_directory_path(),cancel,20000);if(result.code)return false;
    auto at=result.output.find('{');auto parsed=at==std::string::npos?std::optional<Json>{}:parseJSON(result.output.substr(at));if(!parsed)return false;
    auto test=[](const Json& source){return source.is_object()&&source.value("Name","")=="winget"&&source.value("Type","")=="Microsoft.PreIndexed.Package"&&source.value("Arg","")=="https://cdn.winget.microsoft.com/cache";};
    if(test(*parsed))return true;
    if(parsed->contains("Sources")&&(*parsed)["Sources"].is_array())for(auto& source:(*parsed)["Sources"])if(test(source))return true;
    return false;
}
inline std::vector<std::wstring> packageArguments(const std::string& command,const std::string& id="",const std::string& version=""){
    if(!packageToken(id,128)||(!version.empty()&&!packageToken(version,64)))throw std::runtime_error("Invalid package ID/version");
    std::vector<std::wstring> args={wide(command),L"--id",wide(id),L"--exact",L"--source",L"winget",L"--disable-interactivity",L"--accept-source-agreements"};
    if(!version.empty()){args.push_back(L"--version");args.push_back(wide(version));}return args;
}
inline Json agentPackages(const std::string& operation,const Json& args,std::atomic<bool>& cancel){
    auto executable=verifiedWinGet();if(executable.empty())return {{"success",false},{"error","Signed Microsoft App Installer / WinGet is unavailable. Install or repair it through Microsoft Store; Hext did not download an installer."},{"requires_setup","packages"},{"recoverable",true}};
    if(!officialWinGetSource(executable,cancel))return {{"success",false},{"error","The WinGet source is unavailable or differs from the official community source. Hext did not accept an alternate repository."},{"recoverable",true}};
    if(operation=="packages.search"){
        auto query=args.at("query").get<std::string>();if(query.empty()||query[0]=='-')throw std::runtime_error("Invalid package query");
        auto result=agentProcess(executable,{L"search",L"--query",wide(query),L"--source",L"winget",L"--disable-interactivity",L"--accept-source-agreements"},std::filesystem::temp_directory_path(),cancel,45000);
        return {{"success",result.code==0&&!result.truncated},{"exit_code",result.code},{"text",utf8Excerpt(result.output,0,12000)},{"untrusted",true},{"source","https://cdn.winget.microsoft.com/cache"},{"recoverable",true}};
    }
    auto id=args.at("id").get<std::string>(),version=args.at("version").get<std::string>();if(!packageToken(id,128)||!packageToken(version,64))throw std::runtime_error("Invalid package target");
    auto metadata=agentProcess(executable,packageArguments("show",id,version),std::filesystem::temp_directory_path(),cancel,45000);
    if(metadata.code||metadata.truncated||metadata.output.empty())return {{"success",false},{"error","WinGet could not inspect this exact package version. Search again or use the publisher's official setup manually."},{"recoverable",true}};
    auto header=metadata.output.find("["+id+"]");if(header==std::string::npos)return {{"success",false},{"error","WinGet did not return the exact inspected package identity."},{"recoverable",true}};
    auto detail=metadata.output.substr(header);detail.erase(std::remove(detail.begin(),detail.end(),'\r'),detail.end());if(detail.size()>16000)return {{"success",false},{"error","Package details exceed the review limit."},{"recoverable",true}};
    if(operation=="packages.inspect")return {{"success",true},{"proposal",randomID()},{"id",id},{"version",version},{"metadata",detail},{"untrusted",true},{"source","https://cdn.winget.microsoft.com/cache"}};
    if(operation!="packages.install"||detail!=args.value("metadata",""))return {{"success",false},{"error","Package details changed since approval. Inspect and approve a new proposal."},{"recoverable",true}};
    auto install=packageArguments("install",id,version);for(auto option:{L"--scope",L"user",L"--silent",L"--no-upgrade"})install.push_back(option);
    // License dialogs are not auto-accepted. WinGet fails clearly if participation is required.
    auto result=agentProcess(executable,install,std::filesystem::temp_directory_path(),cancel,180000);
    if(cancel)return {{"success",false},{"uncertain",true},{"error","Installation interrupted. An external installer may continue; inspect installed state before retrying."}};
    auto verify=agentProcess(executable,packageArguments("list",id),std::filesystem::temp_directory_path(),cancel,30000);
    bool exact=false;std::istringstream installed(verify.output);std::string token;while(installed>>token)if(token==id){std::string current;installed>>current;if(current==version)exact=true;}
    bool confirmed=result.code==0&&verify.code==0&&exact;
    return {{"success",confirmed},{"verified",confirmed},{"exit_code",result.code},{"text",utf8Excerpt(result.output,0,6000)},{"installed_state",utf8Excerpt(verify.output,0,4000)},{"uncertain",!confirmed},{"note","Only a pinned user-scope installation was attempted. No account login or final application task is implied. Do not automatically retry an uncertain installation."}};
}
}
