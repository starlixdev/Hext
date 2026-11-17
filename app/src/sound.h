#pragma once
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <bluetoothapis.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iomanip>

namespace hext {
inline constexpr PROPERTYKEY DisableSystemEffects={{0x1da5d803,0xd492,0x4edd,{0x8c,0x23,0xe0,0xc0,0xff,0xee,0x7f,0x0e}},5};
template<class T> struct AudioPtr {
    T* p=nullptr;
    ~AudioPtr(){if(p)p->Release();}
    T* operator->()const{return p;}
};
struct SoundDevice {
    std::wstring id,name;
    EDataFlow flow=eRender;
    DWORD state=0;
    bool isDefault=false;
};
struct EndpointReading {
    bool ready=false,muted=false;
    float volume=0,balance=0;
    UINT channels=0;
};
// The policy interface has no supported Windows SDK equivalent. Keep it isolated;
// callers fall back to the Windows sound panel if this interface is unavailable.
// Vtable verified against EarTrumpet/Interop/MMDeviceAPI/IPolicyConfig.cs.
struct HextPolicyConfig: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Unused1()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused2()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused3()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused4()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused5()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused6()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused7()=0;
    virtual HRESULT STDMETHODCALLTYPE Unused8()=0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(LPCWSTR,const PROPERTYKEY&,PROPVARIANT*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(LPCWSTR,const PROPERTYKEY&,PROPVARIANT*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(LPCWSTR,ERole)=0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(LPCWSTR,BOOL)=0;
};
class SoundEngine {
    template<class F> HRESULT endpoint(const std::wstring& id,F use){
        AudioPtr<IMMDeviceEnumerator> e;
        HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&e.p));
        if(FAILED(hr))return hr;
        AudioPtr<IMMDevice> d;hr=e->GetDevice(id.c_str(),&d.p);if(FAILED(hr))return hr;
        AudioPtr<IAudioEndpointVolume> v;
        hr=d->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_INPROC_SERVER,nullptr,reinterpret_cast<void**>(&v.p));
        return FAILED(hr)?hr:use(v.p);
    }
public:
    std::vector<SoundDevice> devices;
    std::wstring selected[2];
    EndpointReading reading[2];
    HRESULT refresh(){
        AudioPtr<IMMDeviceEnumerator> e;
        HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&e.p));
        if(FAILED(hr)){devices.clear();reading[0]={};reading[1]={};return hr;}
        std::wstring defaults[2];
        for(int flow=0;flow<2;flow++){
            AudioPtr<IMMDevice> d;
            if(SUCCEEDED(e->GetDefaultAudioEndpoint(static_cast<EDataFlow>(flow),eMultimedia,&d.p))){
                LPWSTR id=nullptr;if(SUCCEEDED(d->GetId(&id))){defaults[flow]=id;CoTaskMemFree(id);}
            }
        }
        AudioPtr<IMMDeviceCollection> all;
        hr=e->EnumAudioEndpoints(eAll,DEVICE_STATEMASK_ALL,&all.p);if(FAILED(hr))return hr;
        UINT count=0;all->GetCount(&count);std::vector<SoundDevice> next;
        for(UINT i=0;i<count;i++){
            AudioPtr<IMMDevice> d;if(FAILED(all->Item(i,&d.p)))continue;
            SoundDevice item;LPWSTR id=nullptr;
            if(FAILED(d->GetId(&id)))continue;
            item.id=id;CoTaskMemFree(id);d->GetState(&item.state);
            AudioPtr<IMMEndpoint> ep;
            if(FAILED(d->QueryInterface(__uuidof(IMMEndpoint),reinterpret_cast<void**>(&ep.p)))||FAILED(ep->GetDataFlow(&item.flow)))continue;
            AudioPtr<IPropertyStore> props;
            if(SUCCEEDED(d->OpenPropertyStore(STGM_READ,&props.p))){
                PROPVARIANT value;PropVariantInit(&value);
                if(SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName,&value))&&value.vt==VT_LPWSTR)item.name=value.pwszVal;
                PropVariantClear(&value);
            }
            if(item.name.empty())item.name=L"Dispositivo de áudio";
            item.isDefault=item.id==defaults[item.flow];next.push_back(std::move(item));
        }
        std::stable_sort(next.begin(),next.end(),[](const SoundDevice& a,const SoundDevice& b){
            if((a.state==DEVICE_STATE_ACTIVE)!=(b.state==DEVICE_STATE_ACTIVE))return a.state==DEVICE_STATE_ACTIVE;
            if(a.isDefault!=b.isDefault)return a.isDefault;
            return a.name<b.name;
        });
        devices=std::move(next);
        for(int flow=0;flow<2;flow++){
            if(selected[flow].empty())selected[flow]=defaults[flow];
            read(flow);
        }
        return S_OK;
    }
    const SoundDevice* chosen(int flow)const{
        for(auto& item:devices)if(item.id==selected[flow]&&item.flow==flow)return &item;
        return nullptr;
    }
    void read(int flow){
        EndpointReading result;
        endpoint(selected[flow],[&](IAudioEndpointVolume* v){
            BOOL muted=FALSE;float scalar=0;
            if(FAILED(v->GetMasterVolumeLevelScalar(&scalar))||FAILED(v->GetMute(&muted)))return E_FAIL;
            result.ready=true;result.volume=scalar;result.muted=muted!=FALSE;v->GetChannelCount(&result.channels);
            if(result.channels==2){float l=0,r=0;v->GetChannelVolumeLevelScalar(0,&l);v->GetChannelVolumeLevelScalar(1,&r);
                float max=std::max(l,r);if(max>.0001f)result.balance=(r-l)/max;}
            return S_OK;
        });
        reading[flow]=result;
    }
    HRESULT volume(int flow,float value){
        HRESULT hr=endpoint(selected[flow],[&](IAudioEndpointVolume* v){return v->SetMasterVolumeLevelScalar(std::clamp(value,0.f,1.f),nullptr);});
        read(flow);return hr;
    }
    HRESULT mute(int flow){
        HRESULT hr=endpoint(selected[flow],[&](IAudioEndpointVolume* v){BOOL muted=FALSE;HRESULT h=v->GetMute(&muted);
            return FAILED(h)?h:v->SetMute(!muted,nullptr);});
        read(flow);return hr;
    }
    HRESULT balance(float value){
        value=std::clamp(value,-1.f,1.f);
        HRESULT hr=endpoint(selected[0],[&](IAudioEndpointVolume* v){UINT count=0;v->GetChannelCount(&count);if(count!=2)return E_NOTIMPL;
            float l=1,r=1;v->GetChannelVolumeLevelScalar(0,&l);v->GetChannelVolumeLevelScalar(1,&r);
            float level=std::max(l,r);if(level<.0001f)level=1;
            HRESULT h=v->SetChannelVolumeLevelScalar(0,level*(value>0?1-value:1),nullptr);
            if(FAILED(h))return h;
            return v->SetChannelVolumeLevelScalar(1,level*(value<0?1+value:1),nullptr);
        });read(0);return hr;
    }
    HRESULT makeDefault(int flow){
        auto item=chosen(flow);if(!item||item->state!=DEVICE_STATE_ACTIVE)return E_INVALIDARG;
        const CLSID cls={0x870af99c,0x171d,0x4f9e,{0xaf,0x0d,0xe6,0x3d,0xf4,0x0c,0x2b,0xc9}};
        const IID iid={0xf8679f50,0x850a,0x41cf,{0x9c,0x72,0x43,0x0f,0x29,0x02,0x90,0xc8}};
        AudioPtr<HextPolicyConfig> policy;
        HRESULT hr=CoCreateInstance(cls,nullptr,CLSCTX_INPROC_SERVER,iid,reinterpret_cast<void**>(&policy.p));
        if(FAILED(hr))return hr;
        // Preserve the separate communications endpoint chosen by the user.
        hr=policy->SetDefaultEndpoint(item->id.c_str(),eConsole);
        if(SUCCEEDED(hr))hr=policy->SetDefaultEndpoint(item->id.c_str(),eMultimedia);
        refresh();return hr;
    }
};

struct BluetoothSnapshot {std::vector<BLUETOOTH_DEVICE_INFO> devices;bool radio=false,searching=false;DWORD error=0;};
class BluetoothDevices {
    std::thread worker;
    std::mutex mutex;
    BluetoothSnapshot result;
    std::atomic<bool> busy{false},changed{false};
public:
    ~BluetoothDevices(){if(worker.joinable())worker.join();}
    bool refreshing()const{return busy.load();}
    void refresh(bool discover=false){
        if(busy.exchange(true))return;
        if(worker.joinable())worker.join();
        worker=std::thread([this,discover]{
            BluetoothSnapshot next;
            BLUETOOTH_FIND_RADIO_PARAMS rp{sizeof(rp)};HANDLE radio=nullptr;
            auto rf=BluetoothFindFirstRadio(&rp,&radio);
            if(rf){
                do{
                    next.radio=true;
                    BLUETOOTH_DEVICE_SEARCH_PARAMS search{sizeof(search)};
                    search.fReturnAuthenticated=search.fReturnRemembered=search.fReturnConnected=TRUE;
                    search.fReturnUnknown=discover;search.fIssueInquiry=discover;search.cTimeoutMultiplier=2;search.hRadio=radio;
                    BLUETOOTH_DEVICE_INFO info{sizeof(info)};
                    auto found=BluetoothFindFirstDevice(&search,&info);
                    if(found){do{
                        auto duplicate=std::find_if(next.devices.begin(),next.devices.end(),[&](const auto& d){return d.Address.ullLong==info.Address.ullLong;});
                        if(duplicate==next.devices.end())next.devices.push_back(info);
                        info.dwSize=sizeof(info);
                    }while(BluetoothFindNextDevice(found,&info));BluetoothFindDeviceClose(found);}
                    else if(GetLastError()!=ERROR_NO_MORE_ITEMS)next.error=GetLastError();
                    CloseHandle(radio);radio=nullptr;
                }while(BluetoothFindNextRadio(rf,&radio));
                BluetoothFindRadioClose(rf);
            }else next.error=GetLastError();
            std::stable_sort(next.devices.begin(),next.devices.end(),[](const auto& a,const auto& b){return a.fConnected>b.fConnected;});
            {std::lock_guard<std::mutex> lock(mutex);result=std::move(next);}
            changed=true;busy=false;
        });
    }
    bool poll(BluetoothSnapshot& out){
        if(!changed.exchange(false))return false;
        std::lock_guard<std::mutex> lock(mutex);out=result;return true;
    }
};

struct AudioEffects {
    int gain=0,bass=0,mid=0,treble=0; // tenths of a decibel
    bool enabled=false;
    static bool validID(const std::wstring& id){
        // Use only the endpoint's GUID, never names or arbitrary configuration text.
        size_t start=id.find_last_of(L'{');if(start==std::wstring::npos||id.size()-start!=38)return false;
        GUID guid{};return SUCCEEDED(CLSIDFromString(id.substr(start).c_str(),&guid));
    }
    static std::wstring endpointGUID(const std::wstring& id){return validID(id)?id.substr(id.find_last_of(L'{')):L"";}
    bool valid()const{return gain>=0&&gain<=120&&bass>=-120&&bass<=120&&mid>=-120&&mid<=120&&treble>=-120&&treble<=120;}
    std::string configuration(const std::wstring& id)const{
        if(!validID(id)||!valid())return {};
        auto guid=endpointGUID(id);std::string g(guid.begin(),guid.end());
        std::ostringstream out;out.imbue(std::locale::classic());out<<std::fixed<<std::setprecision(1);
        out<<"# Managed by Hext. Only the selected playback endpoint.\nDevice: "<<g<<"\nStage: post-mix\nChannel: all\n";
        out<<"Preamp: "<<(enabled?gain/10.0:0)<<" dB\n";
        if(enabled){out<<"Filter: ON LS Fc 120 Hz Gain "<<bass/10.0<<" dB\n";
            out<<"Filter: ON PK Fc 1000 Hz Gain "<<mid/10.0<<" dB Q 0.8\n";
            out<<"Filter: ON HS Fc 6000 Hz Gain "<<treble/10.0<<" dB\n";}
        out<<"Device: all\n";return out.str();
    }
};
inline std::wstring audioRegString(HKEY hive,const std::wstring& key,const wchar_t* value){
    wchar_t data[32768]{};DWORD bytes=sizeof(data);
    if(RegGetValueW(hive,key.c_str(),value,RRF_RT_REG_SZ,nullptr,data,&bytes)!=ERROR_SUCCESS)return L"";
    return data;
}
struct ApoInstallation {
    std::filesystem::path root,config,selector;
    bool installed=false;
    void detect(){
        auto install=audioRegString(HKEY_LOCAL_MACHINE,L"SOFTWARE\\EqualizerAPO",L"InstallPath");
        if(install.empty()){wchar_t program[32768]{};GetEnvironmentVariableW(L"ProgramFiles",program,32768);install=std::wstring(program)+L"\\EqualizerAPO";}
        root=install;
        wchar_t local[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
        std::error_code ec;
        auto hasSelector=[&](const std::filesystem::path& path){return std::filesystem::is_regular_file(path/L"DeviceSelector.exe",ec)||std::filesystem::is_regular_file(path/L"Configurator.exe",ec);};
        if(!hasSelector(root)&&local[0]){
            for(auto variant:{L"x64-sse2",L"x64-avx",L"x64-avx2",L"x64-avx512",L"x64-avx10-1"}){
                auto candidate=std::filesystem::path(local)/(std::wstring(L"EqualizerAPO-XT-")+variant)/L"current";
                if(hasSelector(candidate)){root=candidate;break;}
            }
        }
        auto configured=audioRegString(HKEY_LOCAL_MACHINE,L"SOFTWARE\\EqualizerAPO",L"ConfigPath");
        config=configured.empty()?root/L"config":std::filesystem::path(configured);
        selector=std::filesystem::is_regular_file(root/L"DeviceSelector.exe",ec)?root/L"DeviceSelector.exe":root/L"Configurator.exe";
        installed=hasSelector(root);
    }
    bool endpointConfigured(const std::wstring& id)const{
        auto guid=AudioEffects::endpointGUID(id);if(!installed||guid.empty())return false;
        AudioPtr<IMMDeviceEnumerator> enumerator;
        if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&enumerator.p))))return false;
        AudioPtr<IMMDevice> device;if(FAILED(enumerator->GetDevice(id.c_str(),&device.p)))return false;
        AudioPtr<IPropertyStore> properties;
        if(SUCCEEDED(device->OpenPropertyStore(STGM_READ,&properties.p))){
            PROPVARIANT disabled;PropVariantInit(&disabled);
            bool bypass=SUCCEEDED(properties->GetValue(DisableSystemEffects,&disabled))&&disabled.vt==VT_UI4&&disabled.ulVal!=0;
            PropVariantClear(&disabled);if(bypass)return false;
        }
        HKEY fx=nullptr;auto key=L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render\\"+guid+L"\\FxProperties";
        if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,key.c_str(),0,KEY_READ,&fx)!=ERROR_SUCCESS)return false;
        bool found=false;
        for(DWORD i=0;!found;i++){
            wchar_t name[512];DWORD length=512,type=0,bytes=8192;wchar_t data[4096]{};
            auto status=RegEnumValueW(fx,i,name,&length,nullptr,&type,reinterpret_cast<BYTE*>(data),&bytes);
            if(status==ERROR_NO_MORE_ITEMS)break;
            if(status!=ERROR_SUCCESS||(type!=REG_SZ&&type!=REG_MULTI_SZ))continue;
            std::wstring values(data,std::min<size_t>(bytes/sizeof(wchar_t),4096));
            size_t pos=0;
            while((pos=values.find(L'{',pos))!=std::wstring::npos){
                if(pos+38>values.size())break;
                auto cls=values.substr(pos,38);GUID valid{};
                if(SUCCEEDED(CLSIDFromString(cls.c_str(),&valid))){
                    auto dll=audioRegString(HKEY_CLASSES_ROOT,L"CLSID\\"+cls+L"\\InprocServer32",nullptr);
                    std::transform(dll.begin(),dll.end(),dll.begin(),::towlower);
                    if(dll.find(L"equalizerapo")!=std::wstring::npos)found=true;
                }pos+=38;
            }
        }
        RegCloseKey(fx);return found;
    }
    // Called by the explicit, elevated helper only. No arbitrary file or config input.
    int apply(const std::wstring& id,const AudioEffects& effects){
        if(!effects.valid()||!AudioEffects::validID(id))return 1;
        if(!installed)return 2;
        SoundEngine check;check.selected[0]=id;check.refresh();auto target=check.chosen(0);
        if(!target||target->state!=DEVICE_STATE_ACTIVE||!endpointConfigured(id))return 4;
        return writeConfiguration(config,id,effects);
    }
    static int writeConfiguration(const std::filesystem::path& config,const std::wstring& id,const AudioEffects& effects){
        auto text=effects.configuration(id);if(text.empty())return 1;
        auto main=config/L"config.txt",own=config/L"Hext.txt",temp=config/L"Hext.pending.txt";
        std::error_code sizeError;if(std::filesystem::file_size(main,sizeError)>4*1024*1024||sizeError)return 3;
        std::ifstream existing(main,std::ios::binary);if(!existing)return 3;
        std::string original((std::istreambuf_iterator<char>(existing)),{});existing.close();
        if(original.size()>4*1024*1024)return 3;
        if(original.size()>1&&(static_cast<unsigned char>(original[0])==0xff||static_cast<unsigned char>(original[0])==0xfe))return 3;
        {std::ifstream previous(own,std::ios::binary);if(previous){std::string first;std::getline(previous,first);
            if(first!="# Managed by Hext. Only the selected playback endpoint.")return 5;}}
        bool included=false;std::istringstream lines(original);std::string line;
        while(std::getline(lines,line)){
            auto start=line.find_first_not_of(" \t");if(start==std::string::npos||line[start]=='#')continue;
            auto end=line.find_last_not_of(" \t\r");auto value=line.substr(start,end-start+1);
            std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return std::tolower(c);});
            if(value=="include: hext.txt")included=true;
        }
        {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<text;out.close();if(!out)return 3;}
        if(!MoveFileExW(temp.c_str(),own.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return 3;
        if(!included){
            // CopyFile(TRUE) creates one backup and never replaces an older backup.
            if(!CopyFileW(main.c_str(),(config/L"config.before-hext.txt").c_str(),TRUE)&&GetLastError()!=ERROR_FILE_EXISTS)return 3;
            auto pending=config/L"config.hext.pending.txt";
            std::ofstream out(pending,std::ios::binary|std::ios::trunc);out<<original<<"\n# Hext audio effects\nDevice: all\nInclude: Hext.txt\n";
            out.close();if(!out)return 3;
            if(!MoveFileExW(pending.c_str(),main.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return 3;
        }
        return 0;
    }
};
}
