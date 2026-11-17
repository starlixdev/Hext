#pragma once
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <bluetoothapis.h>
#include <thread>
#include <mutex>
#include <atomic>

namespace hext {
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
}
