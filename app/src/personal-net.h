#pragma once
#include "personal-core.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <shellapi.h>
#include <thread>

namespace hext {
struct InternetHandle {
    HINTERNET p=nullptr;explicit InternetHandle(HINTERNET value=nullptr):p(value){}~InternetHandle(){if(p)WinHttpCloseHandle(p);}
    InternetHandle(const InternetHandle&)=delete;
};
struct HttpReply {DWORD status=0,error=0;std::string body;int retryAfter=0;};
inline HttpReply spotifyHTTP(const wchar_t* host,const std::wstring& path,const wchar_t* method,const std::string& body,const std::string& token="",bool form=false){
    HttpReply out;
    if((std::wstring(host)!=L"api.spotify.com"&&std::wstring(host)!=L"accounts.spotify.com")||path.empty()||path[0]!=L'/')return out;
    InternetHandle session(WinHttpOpen(L"Hext/0.8",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.p){out.error=GetLastError();return out;}WinHttpSetTimeouts(session.p,5000,5000,10000,10000);
    InternetHandle connection(WinHttpConnect(session.p,host,INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection.p){out.error=GetLastError();return out;}
    InternetHandle request(WinHttpOpenRequest(connection.p,method,path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.p){out.error=GetLastError();return out;}
    DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request.p,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy));
    std::wstring headers=L"Accept: application/json\r\nContent-Type: ";headers+=form?L"application/x-www-form-urlencoded\r\n":L"application/json\r\n";
    if(!token.empty())headers+=L"Authorization: Bearer "+wide(token)+L"\r\n";
    if(!WinHttpSendRequest(request.p,headers.c_str(),static_cast<DWORD>(-1),body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<char*>(body.data()),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0)||!WinHttpReceiveResponse(request.p,nullptr)){
        out.error=GetLastError();return out;
    }
    DWORD bytes=sizeof(out.status);WinHttpQueryHeaders(request.p,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&out.status,&bytes,WINHTTP_NO_HEADER_INDEX);
    wchar_t retry[32]{};bytes=sizeof(retry);if(WinHttpQueryHeaders(request.p,WINHTTP_QUERY_CUSTOM,L"Retry-After",retry,&bytes,WINHTTP_NO_HEADER_INDEX))out.retryAfter=std::clamp(_wtoi(retry),0,86400);
    std::array<char,8192> buffer{};DWORD read=0;
    auto deadline=GetTickCount64()+30000;
    for(;;){
        if(GetTickCount64()>deadline){out.error=ERROR_TIMEOUT;out.body.clear();return out;}
        if(!WinHttpReadData(request.p,buffer.data(),static_cast<DWORD>(buffer.size()),&read)){out.error=GetLastError();out.body.clear();return out;}
        if(!read)break;
        if(out.body.size()+read>2*1024*1024){out.error=ERROR_INSUFFICIENT_BUFFER;out.body.clear();return out;}out.body.append(buffer.data(),read);
    }
    return out;
}
inline std::string base64url(const unsigned char* data,size_t length){
    DWORD size=0;CryptBinaryToStringA(data,static_cast<DWORD>(length),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size);std::string out(size,'\0');
    if(!CryptBinaryToStringA(data,static_cast<DWORD>(length),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,out.data(),&size))throw std::runtime_error("Encoding failed");
    out.resize(size);
    while(!out.empty()&&(out.back()=='='||out.back()=='\0'))out.pop_back();
    std::replace(out.begin(),out.end(),'+','-');std::replace(out.begin(),out.end(),'/','_');return out;
}
inline std::string pkceChallenge(const std::string& verifier){
    std::array<unsigned char,32> digest{};
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD bytes=0,length=0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA-256 unavailable");
    BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<BYTE*>(&length),sizeof(length),&bytes,0);std::vector<BYTE> object(length);
    bool ok=BCryptCreateHash(algorithm,&hash,object.data(),length,nullptr,0,0)>=0&&BCryptHashData(hash,reinterpret_cast<BYTE*>(const_cast<char*>(verifier.data())),static_cast<ULONG>(verifier.size()),0)>=0&&BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)throw std::runtime_error("SHA-256 failed");return base64url(digest.data(),digest.size());
}
inline std::optional<std::string> decodeURL(const std::string& text){
    std::string out;auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;};
    for(size_t i=0;i<text.size();i++){if(text[i]=='%'){if(i+2>=text.size())return {};int a=digit(text[i+1]),b=digit(text[i+2]);if(a<0||b<0)return {};out+=static_cast<char>(a*16+b);i+=2;}else out+=text[i]=='+'?' ':text[i];}if(out.find('\0')!=std::string::npos)return {};return out;
}
inline std::optional<std::string> oauthCode(const std::string& request,const std::string& state){
    if(request.size()>8192||request.rfind("GET /callback?",0)!=0)return {};
    auto end=request.find(" HTTP/1.");if(end==std::string::npos)return {};
    std::map<std::string,std::string> values;auto query=request.substr(14,end-14);size_t start=0;
    while(start<=query.size()){auto stop=query.find('&',start);auto part=query.substr(start,stop==std::string::npos?std::string::npos:stop-start);auto equal=part.find('=');if(equal==std::string::npos)return {};auto key=decodeURL(part.substr(0,equal)),value=decodeURL(part.substr(equal+1));if(!key||!value||values.count(*key))return {};values[*key]=*value;if(stop==std::string::npos)break;start=stop+1;}
    if(values["state"]!=state||!values.count("code")||values["code"].empty()||values["code"].size()>2048||values.count("error"))return {};
    return values["code"];
}
struct SpotifySnapshot {
    DWORD httpStatus=0;
    bool busy=false,credentials=false,connected=false;std::wstring status=L"Conecte sua conta.";
    std::vector<Json> tracks,devices;std::string device;int volume=50;bool supportsVolume=false;
    bool completed=false,success=false;std::string receipt;
};
class SpotifyService {
    PersonalStore& store;mutable std::mutex mutex;std::mutex credentialMutex;std::thread worker;std::atomic<bool> stopRequested{false},active{false};
    SpotifySnapshot state;std::string client,selectedDevice;ULONGLONG retryUntil=0;
    void status(const std::wstring& value){std::lock_guard<std::mutex> lock(mutex);state.status=value;}
    void finish(bool success,const std::wstring& value,const std::string& receipt){std::lock_guard<std::mutex> lock(mutex);if(stopRequested)return;state.busy=false;state.success=success;state.completed=true;state.receipt=receipt;state.status=value;}
    std::optional<std::string> access(){
        auto token=store.get("credential","spotify");if(!token.is_object())return {};
        if(token.value("expires",0LL)>timestamp()+60&&textField(token,"access",4096))return token["access"].get<std::string>();
        if(!textField(token,"refresh",4096))return {};
        auto reply=spotifyHTTP(L"accounts.spotify.com",L"/api/token",L"POST","grant_type=refresh_token&refresh_token="+urlEncode(token["refresh"])+"&client_id="+urlEncode(client),"",true);
        auto parsed=parseJSON(reply.body);if(stopRequested||reply.status!=200||!parsed||!textField(*parsed,"access_token",4096))return {};
        token["access"]=(*parsed)["access_token"];token["expires"]=timestamp()+std::clamp(parsed->value("expires_in",3600),60,86400);
        if(textField(*parsed,"refresh_token",4096))token["refresh"]=(*parsed)["refresh_token"];
        {std::lock_guard<std::mutex> lock(credentialMutex);if(stopRequested)return {};store.put("credential","spotify",token);}return token["access"].get<std::string>();
    }
    HttpReply request(const std::wstring& path,const wchar_t* method=L"GET",const std::string& body=""){
        if(stopRequested)return {};
        if(GetTickCount64()<retryUntil){HttpReply limited;limited.status=429;return limited;}
        auto token=access();if(!token){HttpReply missing;missing.status=401;return missing;}
        auto reply=spotifyHTTP(L"api.spotify.com",path,method,body,*token);
        if(stopRequested)return {};
        if(reply.status==401){
            {std::lock_guard<std::mutex> lock(credentialMutex);if(stopRequested)return {};auto saved=store.get("credential","spotify");if(saved.is_object()){saved["expires"]=0;store.put("credential","spotify",saved);}}
            auto refreshed=access();if(refreshed&&!stopRequested)reply=spotifyHTTP(L"api.spotify.com",path,method,body,*refreshed);
        }
        if(reply.status==429)retryUntil=GetTickCount64()+static_cast<ULONGLONG>(std::max(5,reply.retryAfter))*1000;
        return reply;
    }
    std::wstring failure(const HttpReply& r){
        {std::lock_guard<std::mutex> lock(mutex);state.httpStatus=r.status;}
        if(r.error)return L"Não foi possível confirmar a operação. Verifique a conexão antes de repetir.";
        if(r.status==401){std::lock_guard<std::mutex> lock(mutex);state.connected=false;state.credentials=false;return L"A sessão expirou. Conecte sua conta novamente.";}
        if(r.status==403)return L"Acesso recusado. Confira Premium, permissões e a lista de usuários do aplicativo Spotify.";
        if(r.status==404)return L"Abra o Spotify e selecione um dispositivo disponível.";
        if(r.status==429)return L"Limite do Spotify atingido. Aguarde antes de tentar novamente.";
        return L"O Spotify não concluiu a operação.";
    }
    template<class F> bool launch(F fn){
        if(active.exchange(true))return false;
        if(worker.joinable())worker.join();
        stopRequested=false;
        {std::lock_guard<std::mutex> lock(mutex);state.busy=true;state.completed=false;state.receipt.clear();state.httpStatus=0;}
        worker=std::thread([this,fn]{try{fn();}catch(...){finish(false,L"Não foi possível concluir a conexão com o Spotify.","spotify.error");}{std::lock_guard<std::mutex> lock(mutex);state.busy=false;}active=false;});return true;
    }
public:
    explicit SpotifyService(PersonalStore& s):store(s){}~SpotifyService(){stopRequested=true;if(worker.joinable())worker.join();}
    void initialize(){auto config=store.get("connection","spotify");if(config.is_object())client=config.value("client_id","");auto token=store.get("credential","spotify");std::lock_guard<std::mutex> lock(mutex);state.credentials=token.is_object();if(state.credentials)state.status=L"Sessão salva · verificar conexão";}
    static bool validClient(const std::string& s){return s.size()==32&&std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isxdigit(c)!=0;});}
    std::string clientID()const{return client;}
    bool busy()const{return active;}
    SpotifySnapshot snapshot()const{std::lock_guard<std::mutex> lock(mutex);return state;}
    std::optional<SpotifySnapshot> takeCompletion(){std::lock_guard<std::mutex> lock(mutex);if(!state.completed)return {};state.completed=false;return state;}
    void cancel(){stopRequested=true;std::lock_guard<std::mutex> lock(mutex);state.completed=false;state.busy=active;state.status=active?L"Cancelando operação…":L"Operação cancelada.";}
    void disconnect(){cancel();{std::lock_guard<std::mutex> lock(credentialMutex);store.erase("credential","spotify");}std::lock_guard<std::mutex> lock(mutex);state=SpotifySnapshot{};state.busy=active;state.status=L"Conta desconectada.";if(!active)selectedDevice.clear();}
    void configure(const std::string& value){if(active||!validClient(value))throw std::runtime_error("Invalid Spotify Client ID or active request");disconnect();client=value;store.put("connection","spotify",{{"client_id",client}});}
    void resetConfiguration(){disconnect();store.erase("connection","spotify");if(!active)client.clear();}
    bool connect(HWND owner){
        if(!validClient(client))return false;
        return launch([this,owner]{
            WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Socket unavailable");
            struct NetworkCleanup {~NetworkCleanup(){WSACleanup();}} cleanup;
            SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Socket unavailable");
            struct Socket {SOCKET p;~Socket(){if(p!=INVALID_SOCKET)closesocket(p);}} socketGuard{listener};
            BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
            sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=0;
            if(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(listener,1))throw std::runtime_error("Callback unavailable");
            int size=sizeof(address);getsockname(listener,reinterpret_cast<sockaddr*>(&address),&size);
            auto redirect="http://127.0.0.1:"+std::to_string(ntohs(address.sin_port))+"/callback";
            auto verifier=randomID()+randomID(),stateValue=randomID();
            auto url="https://accounts.spotify.com/authorize?response_type=code&client_id="+client+"&redirect_uri="+urlEncode(redirect)+"&code_challenge_method=S256&code_challenge="+pkceChallenge(verifier)+"&state="+stateValue+"&scope="+urlEncode("user-read-playback-state user-modify-playback-state");
            if(reinterpret_cast<INT_PTR>(ShellExecuteW(owner,L"open",wide(url).c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)throw std::runtime_error("Browser could not open");
            status(L"Autorize no navegador. Redirect cadastrado: http://127.0.0.1/callback");
            auto until=GetTickCount64()+180000;std::optional<std::string> code;
            while(!stopRequested&&GetTickCount64()<until){
                fd_set readable;FD_ZERO(&readable);FD_SET(listener,&readable);timeval timeout{0,200000};if(select(0,&readable,nullptr,nullptr,&timeout)<=0)continue;
                sockaddr_in peer{};int peerSize=sizeof(peer);SOCKET connection=accept(listener,reinterpret_cast<sockaddr*>(&peer),&peerSize);if(connection==INVALID_SOCKET)continue;Socket connectionGuard{connection};
                DWORD wait=250;setsockopt(connection,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));
                std::array<char,2048> buffer{};std::string incoming;auto readUntil=GetTickCount64()+2000;
                while(!stopRequested&&GetTickCount64()<readUntil&&incoming.size()<8192&&incoming.find("\r\n\r\n")==std::string::npos){
                    int count=recv(connection,buffer.data(),static_cast<int>(std::min(buffer.size(),8192-incoming.size())),0);
                    if(count>0)incoming.append(buffer.data(),count);else if(count==0||WSAGetLastError()!=WSAETIMEDOUT)break;
                }
                if(peer.sin_addr.s_addr==htonl(INADDR_LOOPBACK)&&incoming.find("\r\n\r\n")!=std::string::npos)code=oauthCode(incoming,stateValue);
                std::string page=code?"Spotify authorization received. You may return to Hext.":"Invalid or expired authorization. Return to Hext and try again.";
                std::string response="HTTP/1.1 "+std::string(code?"200 OK":"400 Bad Request")+"\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+std::to_string(page.size())+"\r\n\r\n"+page;send(connection,response.data(),static_cast<int>(response.size()),0);if(code)break;
            }
            if(stopRequested){finish(false,L"Conexão cancelada.","spotify.cancelled");return;}if(!code){finish(false,L"Autorização não recebida. Verifique o redirect e tente novamente.","spotify.auth_timeout");return;}
            auto reply=spotifyHTTP(L"accounts.spotify.com",L"/api/token",L"POST","grant_type=authorization_code&code="+urlEncode(*code)+"&redirect_uri="+urlEncode(redirect)+"&client_id="+client+"&code_verifier="+verifier,"",true);
            auto parsed=parseJSON(reply.body);if(stopRequested)return;
            if(reply.status!=200||!parsed||!textField(*parsed,"access_token",4096)||!textField(*parsed,"refresh_token",4096)){finish(false,L"O Spotify recusou a autorização. Confira o Client ID e o redirect.","spotify.auth_failed");return;}
            {std::lock_guard<std::mutex> lock(credentialMutex);if(stopRequested)return;store.put("credential","spotify",{{"access",(*parsed)["access_token"]},{"refresh",(*parsed)["refresh_token"]},{"expires",timestamp()+std::clamp(parsed->value("expires_in",3600),60,86400)}});}
            {std::lock_guard<std::mutex> lock(mutex);if(stopRequested)return;state.credentials=true;state.connected=true;}
            finish(true,L"Conta conectada.","spotify.connected");
        });
    }
    bool command(const std::string& op,const std::string& query="",int percent=0){
        return launch([this,op,query,percent]{
            if(op=="search"){
                auto r=request(L"/v1/search?type=track&limit=5&q="+wide(urlEncode(query)));auto j=parseJSON(r.body);
                if(r.status!=200||!j||!j->contains("tracks")||!(*j)["tracks"].is_object()||!(*j)["tracks"].contains("items")||!(*j)["tracks"]["items"].is_array()){finish(false,failure(r),"spotify.search_failed");return;}
                std::vector<Json> tracks;for(auto& track:(*j)["tracks"]["items"]){
                    if(tracks.size()==5)break;
                    if(!textField(track,"name",512)||!textField(track,"uri",128))continue;
                    auto uri=track["uri"].get<std::string>();if(uri.rfind("spotify:track:",0)!=0)continue;auto id=uri.substr(14);
                    if(id.size()!=22||!std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)!=0;}))continue;
                    std::string artist;if(track.contains("artists")&&track["artists"].is_array()&&!track["artists"].empty()&&textField(track["artists"][0],"name",512))artist=track["artists"][0]["name"];
                    tracks.push_back({{"name",track["name"]},{"artist",artist},{"uri",uri},{"url","https://open.spotify.com/track/"+id}});
                }
                if(stopRequested)return;
                {std::lock_guard<std::mutex> lock(mutex);if(stopRequested)return;state.tracks=std::move(tracks);state.connected=true;}
                finish(true,L"Escolha uma faixa. Resultados fornecidos pelo Spotify.","spotify.results_ready");return;
            }
            if(op=="devices"||op=="verify"){
                auto r=request(L"/v1/me/player/devices");auto j=parseJSON(r.body);if(r.status!=200||!j||!j->contains("devices")||!(*j)["devices"].is_array()){finish(false,failure(r),"spotify.devices_failed");return;}
                std::vector<Json> devices;for(auto& d:(*j)["devices"]){if(devices.size()==30)break;if(textField(d,"id",256)&&textField(d,"name",256))devices.push_back({{"id",d["id"]},{"name",d["name"]},{"restricted",d.value("is_restricted",true)},{"active",d.value("is_active",false)},{"supports_volume",d.value("supports_volume",false)},{"volume",d.value("volume_percent",Json(50)).is_number_integer()?d["volume_percent"].get<int>():50}});}
                if(stopRequested)return;
                {std::lock_guard<std::mutex> lock(mutex);if(stopRequested)return;state.devices=std::move(devices);state.connected=true;}
                finish(true,L"Dispositivos atualizados.","spotify.devices_ready");return;
            }
            std::wstring path=L"/v1/me/player/";const wchar_t* method=L"PUT";std::string body;
            if(op=="play"||op=="resume"){path+=L"play";if(op=="play"){if(query.rfind("spotify:track:",0)!=0){finish(false,L"Selecione uma faixa nos resultados.","spotify.invalid_track");return;}body=Json({{"uris",Json::array({query})}}).dump();}}
            else if(op=="pause")path+=L"pause";
            else if(op=="next"||op=="previous"){path+=wide(op);method=L"POST";}
            else if(op=="volume"){if(percent<0||percent>100){finish(false,L"Volume inválido.","spotify.invalid_volume");return;}path+=L"volume?volume_percent="+std::to_wstring(percent);}
            else if(op=="transfer"){if(query.empty()||query.size()>256){finish(false,L"Dispositivo inválido.","spotify.invalid_device");return;}path=L"/v1/me/player";body=Json({{"device_ids",Json::array({query})},{"play",false}}).dump();}
            else{finish(false,L"Ação de Spotify indisponível.","spotify.unsupported");return;}
            if(!selectedDevice.empty()&&op!="transfer")path+=(path.find(L'?')==std::wstring::npos?L"?":L"&")+std::wstring(L"device_id=")+wide(urlEncode(selectedDevice));
            auto r=request(path,method,body);if(stopRequested)return;if(r.status!=204){finish(false,failure(r),"spotify.operation_failed");return;}
            // A 204 acknowledges the command; inspect player state where the API exposes the requested property.
            bool confirmed=false;if(op=="play"||op=="resume"||op=="pause"||op=="volume"){
                auto check=request(L"/v1/me/player");auto j=parseJSON(check.body);if(check.status==200&&j){
                    if(op=="pause")confirmed=j->value("is_playing",true)==false;
                    else if(op=="play"||op=="resume")confirmed=j->value("is_playing",false);
                    else if(j->contains("device")&&(*j)["device"].is_object())confirmed=(*j)["device"].value("volume_percent",-1)==percent;
                }
            }
            if(op=="transfer")selectedDevice=query;
            finish(true,confirmed?L"Estado confirmado pelo Spotify.":L"Comando aceito pelo Spotify; a mudança de estado ainda não foi confirmada.",confirmed?"spotify.state_confirmed":"spotify.command_accepted");
        });
    }
};
}
