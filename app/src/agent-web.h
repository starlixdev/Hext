#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include "agent-process.h"
namespace hext {
struct PublicURL {std::wstring url,host,address;};
inline bool publicIPv4(unsigned long address){
    auto n=ntohl(address);unsigned a=n>>24,b=(n>>16)&255;
    if(a==0||a==10||a==127||a>=224||(a==169&&b==254)||(a==172&&b>=16&&b<=31)||(a==192&&(b==168||b==0))||(a==100&&b>=64&&b<=127)||(a==198&&(b==18||b==19))||(a==192&&b==2)||(a==198&&b==51)||(a==203&&b==0))return false;
    return true;
}
inline std::optional<PublicURL> publicURL(const std::string& input,bool resolve=true){
    if(input.size()>2048||input.find_first_of("\r\n\t\\")!=std::string::npos)return {};
    auto url=wide(input);URL_COMPONENTS parts{sizeof(parts)};parts.dwHostNameLength=parts.dwUserNameLength=parts.dwPasswordLength=parts.dwUrlPathLength=parts.dwExtraInfoLength=static_cast<DWORD>(-1);
    if(!WinHttpCrackUrl(url.c_str(),0,0,&parts)||parts.nScheme!=INTERNET_SCHEME_HTTPS||parts.nPort!=443||parts.dwUserNameLength||parts.dwPasswordLength)return {};
    std::wstring host(parts.lpszHostName,parts.dwHostNameLength);if(host.empty()||host.size()>253||host.back()==L'.'||host.find(L'.')==std::wstring::npos||!std::all_of(host.begin(),host.end(),[](wchar_t c){return (c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'.'||c==L'-';}))return {};
    auto service=lower(utf8(host));if(service=="open.spotify.com"||service=="api.spotify.com"||service=="accounts.spotify.com")return {};
    PublicURL result{url,host,L""};if(!resolve)return result;
    WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return {};struct NetworkCleanup{~NetworkCleanup(){WSACleanup();}}cleanup;
    ADDRINFOW hints{};hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;PADDRINFOW addresses=nullptr;
    if(GetAddrInfoW(host.c_str(),L"443",&hints,&addresses))return {};
    bool safe=true;for(auto it=addresses;it;it=it->ai_next){auto ipv4=reinterpret_cast<sockaddr_in*>(it->ai_addr);if(!publicIPv4(ipv4->sin_addr.s_addr)){safe=false;break;}if(result.address.empty()){wchar_t text[64]{};InetNtopW(AF_INET,&ipv4->sin_addr,text,64);result.address=text;}}
    FreeAddrInfoW(addresses);if(!safe||result.address.empty())return {};return result;
}
inline std::string markupText(const std::string& input){
    std::string result;bool tag=false,suppress=false;std::string current;
    for(size_t i=0;i<input.size()&&result.size()<18000;i++){
        auto c=input[i];if(c=='<'){tag=true;current.clear();continue;}
        if(tag){if(c=='>'){tag=false;auto name=lower(current);if(name.rfind("script",0)==0||name.rfind("style",0)==0)suppress=true;else if(name.rfind("/script",0)==0||name.rfind("/style",0)==0)suppress=false;if(!suppress&&!result.empty()&&result.back()!=' ')result+=' ';}else if(current.size()<100)current+=c;continue;}
        if(!suppress){if(static_cast<unsigned char>(c)<32||c==' '){if(!result.empty()&&result.back()!=' ')result+=' ';}else result+=c;}
    }
    for(auto entity:std::vector<std::pair<std::string,std::string>>{{"&amp;","&"},{"&lt;","<"},{"&gt;",">"},{"&quot;","\""},{"&#39;","'"},{"&nbsp;"," "}}){size_t at=0;while((at=result.find(entity.first,at))!=std::string::npos){result.replace(at,entity.first.size(),entity.second);at+=entity.second.size();}}
    return utf8Excerpt(result,0,16000);
}
inline Json agentReadWeb(const std::string& input,std::atomic<bool>& cancel){
    auto parsed=publicURL(input);if(!parsed)return {{"success",false},{"error","Only public IPv4 HTTPS origins on port 443 are supported; local/private addresses and URL credentials are blocked."},{"recoverable",true}};
    auto result=agentProcess(systemExecutable(L"curl.exe"),{L"--disable",L"--silent",L"--show-error",L"--proto",L"=https",L"--noproxy",L"*",L"--max-time",L"20",L"--max-filesize",L"262144",L"--resolve",parsed->host+L":443:"+parsed->address,L"--user-agent",L"Hext/0.10",L"--dump-header",L"-",L"--url",parsed->url},std::filesystem::temp_directory_path(),cancel,25000);
    auto end=result.output.find("\r\n\r\n");if(end==std::string::npos)end=result.output.find("\n\n");std::string headers=end==std::string::npos?"":result.output.substr(0,end);int status=0;auto firstSpace=headers.find(' ');if(firstSpace!=std::string::npos)status=std::atoi(headers.c_str()+firstSpace+1);
    bool success=!result.code&&!result.truncated&&!result.cancelled&&status>=200&&status<300;auto body=end==std::string::npos?"":result.output.substr(end+(result.output.compare(end,4,"\r\n\r\n")==0?4:2));auto content=lower(headers);
    bool text=content.find("text/")!=std::string::npos||content.find("application/json")!=std::string::npos||content.find("application/xml")!=std::string::npos||content.find("application/rss+xml")!=std::string::npos;
    Json out={{"success",success&&text},{"url",input},{"http_status",status},{"untrusted",true},{"redirects_followed",false},{"recoverable",true}};
    if(success&&text)out["text"]=content.find("html")!=std::string::npos||content.find("xml")!=std::string::npos?markupText(body):utf8Excerpt(body,0,16000);
    else out["error"]=status>=300&&status<400?"Redirect not followed. Inspect its destination separately after permission review.":!text&&success?"This response is not a supported text document.":"HTTPS request failed, timed out or exceeded its output limit.";
    if(content.find("application/rss+xml")!=std::string::npos||body.find("<rss")!=std::string::npos){Json links=Json::array();size_t offset=0;while(links.size()<8&&(offset=body.find("<link>",offset))!=std::string::npos){auto finish=body.find("</link>",offset);if(finish==std::string::npos)break;auto link=body.substr(offset+6,finish-offset-6);if(publicURL(link,false))links.push_back(link);offset=finish+7;}out["links"]=links;}
    return out;
}
}
