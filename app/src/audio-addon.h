#pragma once
#include <winhttp.h>
#include <bcrypt.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <array>

namespace hext {
inline constexpr wchar_t AudioAddonURL[]=L"https://github.com/starlixdev/Hext/releases/download/audio-addon-2.64.0/Hext-Audio-Setup.exe";
inline constexpr char AudioAddonSHA256[]="09cc9cc55e9a721779d0a4f3acaacb7038bb20d1b150fac365300c9ac10a22b9";
inline constexpr unsigned long long AudioAddonSize=74386700;
inline constexpr char AudioAddonVersion[]="2.64.0";
enum class AddonState {Idle,Downloading,Ready,Installing,Error,Cancelled};
struct AddonStatus {AddonState state=AddonState::Idle;unsigned long long received=0,total=AudioAddonSize;std::wstring message;};
struct HttpHandle {HINTERNET handle=nullptr;~HttpHandle(){if(handle)WinHttpCloseHandle(handle);}};
class SHA256 {
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
public:
    SHA256(){if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0)
        BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0);}
    ~SHA256(){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
    bool update(const void* bytes,size_t count){return hash&&count<=ULONG_MAX&&BCryptHashData(hash,
        reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),static_cast<ULONG>(count),0)>=0;}
    std::string finish(){
        std::array<unsigned char,32> digest{};if(!hash||BCryptFinishHash(hash,digest.data(),32,0)<0)return {};
        constexpr char hex[]="0123456789abcdef";std::string out;
        for(auto byte:digest){out+=hex[byte>>4];out+=hex[byte&15];}return out;
    }
};
inline bool verifiedAddon(const std::filesystem::path& file){
    std::error_code error;if(std::filesystem::file_size(file,error)!=AudioAddonSize||error)return false;
    std::ifstream input(file,std::ios::binary);if(!input)return false;
    SHA256 hash;std::array<char,65536> data{};
    while(input){input.read(data.data(),data.size());auto n=input.gcount();if(n>0&&!hash.update(data.data(),static_cast<size_t>(n)))return false;}
    return input.eof()&&hash.finish()==AudioAddonSHA256;
}
class AudioAddon {
    mutable std::mutex mutex;
    AddonStatus status;
    std::atomic<bool> cancelled{false},running{false};
    std::thread worker;
    std::filesystem::path installer;
    HANDLE installation=nullptr;
    void state(AddonState value,const std::wstring& message){std::lock_guard<std::mutex> lock(mutex);status.state=value;status.message=message;}
    void download(){
        auto partial=installer;partial+=L".download";
        auto fail=[&](const wchar_t* message){
            std::error_code error;std::filesystem::remove(partial,error);
            state(cancelled?AddonState::Cancelled:AddonState::Error,cancelled?L"Download cancelado.":message);running=false;
        };
        if(verifiedAddon(installer)){state(AddonState::Ready,L"Download verificado. Pronto para instalar.");running=false;return;}
        HttpHandle session{WinHttpOpen(L"Hext/0.7.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
        if(!session.handle){fail(L"Não foi possível iniciar o download.");return;}
        WinHttpSetTimeouts(session.handle,5000,5000,5000,5000);
        DWORD tls=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;WinHttpSetOption(session.handle,WINHTTP_OPTION_SECURE_PROTOCOLS,&tls,sizeof(tls));
        URL_COMPONENTS url{};url.dwStructSize=sizeof(url);url.dwHostNameLength=url.dwUrlPathLength=static_cast<DWORD>(-1);
        if(!WinHttpCrackUrl(AudioAddonURL,0,0,&url)||url.nScheme!=INTERNET_SCHEME_HTTPS){fail(L"Endereço do add-on inválido.");return;}
        std::wstring host(url.lpszHostName,url.dwHostNameLength),path(url.lpszUrlPath,url.dwUrlPathLength);
        HttpHandle connection{WinHttpConnect(session.handle,host.c_str(),url.nPort,0)};
        HttpHandle request{connection.handle?WinHttpOpenRequest(connection.handle,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
        if(!request.handle){fail(L"Não foi possível conectar ao GitHub.");return;}
        DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
        WinHttpSetOption(request.handle,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects));
        if(cancelled||!WinHttpSendRequest(request.handle,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.handle,nullptr)){
            fail(L"Falha na conexão com o GitHub. Tente novamente.");return;
        }
        DWORD code=0,bytes=sizeof(code);
        if(!WinHttpQueryHeaders(request.handle,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&bytes,nullptr)||code!=200){
            fail(L"O add-on não está disponível no GitHub. Tente novamente.");return;
        }
        std::ofstream output(partial,std::ios::binary|std::ios::trunc);if(!output){fail(L"Não foi possível salvar o add-on.");return;}
        std::array<char,65536> buffer{};unsigned long long received=0;SHA256 hash;
        for(;;){
            DWORD read=0;if(cancelled||!WinHttpReadData(request.handle,buffer.data(),static_cast<DWORD>(buffer.size()),&read)){
                output.close();fail(L"Download interrompido. Tente novamente.");return;
            }
            if(!read)break;
            received+=read;
            if(received>AudioAddonSize||!hash.update(buffer.data(),read)){output.close();fail(L"O arquivo recebido não corresponde ao add-on.");return;}
            output.write(buffer.data(),read);
            if(!output){output.close();fail(L"Espaço insuficiente ou falha ao salvar o add-on.");return;}
            {std::lock_guard<std::mutex> lock(mutex);status.received=received;}
        }
        output.close();
        if(!output||received!=AudioAddonSize||hash.finish()!=AudioAddonSHA256){fail(L"A verificação do download falhou. O arquivo não será executado.");return;}
        if(cancelled){fail(L"Download cancelado.");return;}
        if(!MoveFileExW(partial.c_str(),installer.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){fail(L"Não foi possível concluir o download.");return;}
        // Preserve Windows' downloaded-file checks when the installer is opened.
        std::ofstream zone(std::filesystem::path(installer.wstring()+L":Zone.Identifier"),std::ios::binary);
        zone<<"[ZoneTransfer]\r\nZoneId=3\r\nHostUrl=https://github.com/starlixdev/Hext\r\n";zone.close();
        state(AddonState::Ready,L"Download verificado. Pronto para instalar.");running=false;
    }
public:
    ~AudioAddon(){stop();if(installation)CloseHandle(installation);}
    AddonStatus snapshot()const{std::lock_guard<std::mutex> lock(mutex);return status;}
    void stop(){cancelled=true;if(worker.joinable())worker.join();}
    void cancel(){cancelled=true;}
    bool start(const std::filesystem::path& directory){
        if(running||installation)return false;
        if(worker.joinable())worker.join();
        std::error_code error;std::filesystem::create_directories(directory,error);
        if(error){state(AddonState::Error,L"Não foi possível criar a pasta do add-on.");return false;}
        installer=directory/L"Hext-Audio-Setup.exe";cancelled=false;running=true;
        {std::lock_guard<std::mutex> lock(mutex);status={AddonState::Downloading,0,AudioAddonSize,L"Baixando do GitHub…"};}
        worker=std::thread([this]{download();});return true;
    }
    bool install(HWND owner){
        if(snapshot().state!=AddonState::Ready||!verifiedAddon(installer)){
            state(AddonState::Error,L"O instalador precisa ser baixado e verificado novamente.");return false;
        }
        SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;
        launch.hwnd=owner;launch.lpVerb=L"open";launch.lpFile=installer.c_str();launch.nShow=SW_SHOWNORMAL;
        if(!ShellExecuteExW(&launch)){state(AddonState::Ready,L"Instalação cancelada ou bloqueada pelo Windows.");return false;}
        installation=launch.hProcess;state(AddonState::Installing,L"Conclua a instalação na janela do add-on.");return true;
    }
    void poll(){
        if(installation&&WaitForSingleObject(installation,0)==WAIT_OBJECT_0){
            DWORD code=1;GetExitCodeProcess(installation,&code);CloseHandle(installation);installation=nullptr;
            state(AddonState::Ready,code==0?L"Selecione a saída em Configurar para ativar os efeitos.":L"Instalação não concluída. Você pode tentar novamente.");
        }
    }
};
}
