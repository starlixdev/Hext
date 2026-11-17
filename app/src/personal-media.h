#pragma once
#include <roapi.h>
#include <winstring.h>
#include <inspectable.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <optional>

namespace hext {
// Narrow Windows.Media.Control ABI declarations. Method order and interface ID
// are checked against Microsoft's windows-rs generated WinRT metadata.
struct MediaAsyncInfo:IInspectable {
    virtual HRESULT STDMETHODCALLTYPE id(UINT32*)=0;
    virtual HRESULT STDMETHODCALLTYPE status(INT32*)=0;
    virtual HRESULT STDMETHODCALLTYPE error(HRESULT*)=0;
    virtual HRESULT STDMETHODCALLTYPE cancel()=0;
    virtual HRESULT STDMETHODCALLTYPE close()=0;
};
template<class T> struct MediaOperation:IInspectable {
    virtual HRESULT STDMETHODCALLTYPE completed(void*)=0;
    virtual HRESULT STDMETHODCALLTYPE getCompleted(void**)=0;
    virtual HRESULT STDMETHODCALLTYPE results(T*)=0;
};
struct MediaPlayback:IInspectable {
    virtual HRESULT STDMETHODCALLTYPE controls(IInspectable**)=0;
    virtual HRESULT STDMETHODCALLTYPE status(INT32*)=0;
};
struct MediaSession:IInspectable {
    virtual HRESULT STDMETHODCALLTYPE appID(HSTRING*)=0;
    virtual HRESULT STDMETHODCALLTYPE properties(IInspectable**)=0;
    virtual HRESULT STDMETHODCALLTYPE timeline(IInspectable**)=0;
    virtual HRESULT STDMETHODCALLTYPE playback(MediaPlayback**)=0;
    virtual HRESULT STDMETHODCALLTYPE play(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE pause(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE stop(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE record(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE forward(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE rewind(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE next(MediaOperation<BYTE>**)=0;
    virtual HRESULT STDMETHODCALLTYPE previous(MediaOperation<BYTE>**)=0;
};
struct MediaManager:IInspectable {virtual HRESULT STDMETHODCALLTYPE current(MediaSession**)=0;};
struct MediaManagerFactory:IInspectable {virtual HRESULT STDMETHODCALLTYPE request(MediaOperation<MediaManager*>**)=0;};
template<class T> struct MediaPtr {T* p=nullptr;~MediaPtr(){if(p)p->Release();}T* operator->()const{return p;}};
template<class T> HRESULT mediaAwait(MediaOperation<T>* op,T* result,const std::atomic<bool>& cancelled){
    if(!op)return E_POINTER;
    const GUID iid={0x00000036,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
    MediaPtr<MediaAsyncInfo> info;auto hr=op->QueryInterface(iid,reinterpret_cast<void**>(&info.p));if(FAILED(hr))return hr;
    auto deadline=GetTickCount64()+5000;INT32 status=0;
    while(SUCCEEDED(hr=info->status(&status))&&status==0&&!cancelled&&GetTickCount64()<deadline)Sleep(20);
    if(cancelled||status==0){info->cancel();return HRESULT_FROM_WIN32(ERROR_CANCELLED);}
    if(FAILED(hr))return hr;
    if(status!=1){HRESULT error=E_FAIL;info->error(&error);return FAILED(error)?error:E_FAIL;}
    return op->results(result);
}
struct MediaResult {HRESULT code=E_PENDING;bool session=false,accepted=false,confirmed=false;INT32 playback=-1;std::string operation;};
inline MediaResult mediaCommand(const std::string& operation,const std::atomic<bool>& cancelled){
    MediaResult out;out.operation=operation;
    auto initialized=RoInitialize(RO_INIT_MULTITHREADED);if(FAILED(initialized)){out.code=initialized;return out;}
    struct Apartment {~Apartment(){RoUninitialize();}} apartment;
    HSTRING name=nullptr;const wchar_t* type=L"Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager";
    HRESULT hr=WindowsCreateString(type,static_cast<UINT32>(wcslen(type)),&name);
    if(FAILED(hr)){out.code=hr;return out;}
    const GUID iid={0x2050c4ee,0x11a0,0x57de,{0xae,0xd7,0xc9,0x7c,0x70,0x33,0x82,0x45}};
    MediaPtr<MediaManagerFactory> factory;hr=RoGetActivationFactory(name,iid,reinterpret_cast<void**>(&factory.p));WindowsDeleteString(name);
    if(FAILED(hr)){out.code=hr;return out;}
    MediaPtr<MediaOperation<MediaManager*>> request;hr=factory->request(&request.p);MediaPtr<MediaManager> manager;
    if(SUCCEEDED(hr))hr=mediaAwait(request.p,&manager.p,cancelled);
    if(FAILED(hr)||!manager.p){out.code=FAILED(hr)?hr:E_NOINTERFACE;return out;}
    MediaPtr<MediaSession> session;hr=manager->current(&session.p);
    if(FAILED(hr)||!session.p){out.code=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);return out;}out.session=true;
    MediaPtr<MediaPlayback> playback;if(SUCCEEDED(session->playback(&playback.p))&&playback.p)playback->status(&out.playback);
    if(operation=="inspect"){out.code=S_OK;return out;}
    MediaPtr<MediaOperation<BYTE>> action;
    if(cancelled){out.code=HRESULT_FROM_WIN32(ERROR_CANCELLED);return out;}
    if(operation=="play")hr=session->play(&action.p);else if(operation=="pause")hr=session->pause(&action.p);
    else if(operation=="next")hr=session->next(&action.p);else if(operation=="previous")hr=session->previous(&action.p);else{out.code=E_INVALIDARG;return out;}
    BYTE accepted=FALSE;if(SUCCEEDED(hr))hr=mediaAwait(action.p,&accepted,cancelled);out.code=hr;out.accepted=SUCCEEDED(hr)&&accepted;
    if(out.accepted&&(operation=="play"||operation=="pause")){
        for(int i=0;i<10&&!cancelled;i++){
            MediaPtr<MediaPlayback> current;INT32 status=-1;if(SUCCEEDED(session->playback(&current.p))&&current.p&&SUCCEEDED(current->status(&status))){out.playback=status;out.confirmed=operation=="play"?status==4:status==5||status==3;if(out.confirmed)break;}
            Sleep(50);
        }
    }
    return out;
}
class MediaService {
    std::thread worker;std::atomic<bool> cancelled{false},active{false};std::mutex mutex;std::optional<MediaResult> result;
public:
    ~MediaService(){cancelled=true;if(worker.joinable())worker.join();}
    bool busy()const{return active;}
    void cancel(){cancelled=true;std::lock_guard<std::mutex> lock(mutex);result.reset();}
    bool start(const std::string& op){
        if(active.exchange(true))return false;
        if(worker.joinable())worker.join();
        cancelled=false;{std::lock_guard<std::mutex> lock(mutex);result.reset();}
        worker=std::thread([this,op]{auto next=mediaCommand(op,cancelled);if(!cancelled){std::lock_guard<std::mutex> lock(mutex);result=next;}active=false;});return true;
    }
    std::optional<MediaResult> poll(){std::lock_guard<std::mutex> lock(mutex);auto next=result;result.reset();return next;}
};
}
