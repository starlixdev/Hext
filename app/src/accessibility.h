#pragma once
#include <oleacc.h>
// Names, roles, values, focus and actions for the custom Direct2D controls.
// Microsoft Active Accessibility is available on both supported Windows versions.
struct HextAccessible final:IAccessible {
    LONG references=1;
    static const Hit* child(VARIANT id){
        return id.vt==VT_I4&&id.lVal>0&&id.lVal<=static_cast<LONG>(hits.size())?&hits[id.lVal-1]:nullptr;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out){return E_POINTER;}
        
        *out=nullptr;
        if(iid!=IID_IUnknown&&iid!=IID_IDispatch&&iid!=IID_IAccessible)return E_NOINTERFACE;
        *out=static_cast<IAccessible*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&references);}
    ULONG STDMETHODCALLTYPE Release() override{ULONG n=InterlockedDecrement(&references);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* n) override{if(!n)return E_POINTER;*n=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT,LCID,ITypeInfo**) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID,LPOLESTR*,UINT,LCID,DISPID*) override{return DISP_E_UNKNOWNNAME;}
    HRESULT STDMETHODCALLTYPE Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*) override{return DISP_E_MEMBERNOTFOUND;}
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;
        return CreateStdAccessibleObject(windowHandle,OBJID_WINDOW,IID_IAccessible,reinterpret_cast<void**>(out));
    }
    HRESULT STDMETHODCALLTYPE get_accChildCount(LONG* count) override{
        if(!count){return E_POINTER;}
        *count=static_cast<LONG>(hits.size());return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT,IDispatch** out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;return S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT id,BSTR* out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;
        if(id.vt==VT_I4&&id.lVal==CHILDID_SELF){*out=SysAllocString(L"Hext");return S_OK;}
        auto item=child(id);if(!item)return E_INVALIDARG;
        *out=SysAllocString(item->name.c_str());return *out?S_OK:E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT id,BSTR* out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;
        auto item=child(id);if(!item)return S_FALSE;
        if(item->id==240)*out=SysAllocString(command.c_str());
        else if(item->id==450)*out=SysAllocString(assistantInput.c_str());
        else if(item->id==200)*out=SysAllocString(std::to_wstring(static_cast<int>(volume*100)).c_str());
        else if(item->id==620||item->id==621)*out=SysAllocString(shortcuts.bindings[item->id-620].label().c_str());
        else if(soundSlider(item->id))*out=SysAllocString(std::to_wstring(static_cast<int>(std::lround(soundSliderPosition(item->id)*100))).c_str());
        else if(item->id==332)*out=SysAllocString(std::to_wstring(static_cast<int>(draftColor.v*100)).c_str());
        else if(item->id==331)*out=SysAllocString((L"Matiz "+std::to_wstring(static_cast<int>(draftColor.h))+
            L", saturação "+std::to_wstring(static_cast<int>(draftColor.s*100))+L"%").c_str());
        else return S_FALSE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT id,BSTR* out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;
        auto item=child(id);
        if(item&&item->id==230)*out=SysAllocString(L"Prévia visual; o microfone permanece desligado.");
        else if(item&&item->id==331)*out=SysAllocString(L"Setas esquerda e direita ajustam matiz; cima e baixo ajustam saturação.");
        else if(id.vt==VT_I4&&id.lVal==0)*out=SysAllocString(L"Ilha compacta na área de trabalho. Passe o mouse para expandir.");
        return *out?S_OK:S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT id,VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);out->vt=VT_I4;
        if(id.vt==VT_I4&&id.lVal==0){out->lVal=ROLE_SYSTEM_PANE;return S_OK;}
        auto item=child(id);if(!item)return E_INVALIDARG;
        out->lVal=item->id==200||item->id==331||item->id==332||soundSlider(item->id)?ROLE_SYSTEM_SLIDER:
            item->id>=520&&item->id<520+hext::MetricCount?ROLE_SYSTEM_STATICTEXT:
            item->id==240||item->id==450?ROLE_SYSTEM_TEXT:
            item->id==3060||item->id==311||item->id==1020||item->id==1021||(item->id>=380&&item->id<380+hext::MetricCount)?ROLE_SYSTEM_CHECKBUTTON:
            item->id>=600&&item->id<=605?ROLE_SYSTEM_PAGETAB:
            (item->id>=100&&item->id<107)||(item->id>=1000&&item->id<=1002)?ROLE_SYSTEM_PAGETAB:
            (item->id>=901&&item->id<=905)||(item->id>=701&&item->id<=724)||(item->id>=741&&item->id<=743)||(item->id>=801&&item->id<=804)?
                ROLE_SYSTEM_MENUITEM:ROLE_SYSTEM_PUSHBUTTON;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT id,VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);out->vt=VT_I4;
        auto control=child(id);
        out->lVal=control&&!controlFocusable(control->id)?STATE_SYSTEM_READONLY:STATE_SYSTEM_FOCUSABLE;
        if(!IsWindowVisible(windowHandle))out->lVal|=STATE_SYSTEM_INVISIBLE;
        auto item=child(id);
        if(item){
            if(!controlEnabled(item->id))out->lVal|=STATE_SYSTEM_UNAVAILABLE;
            if(item->id==keyboardId||(item->id==240&&inputFocused)||(item->id==450&&assistantFocused))out->lVal|=STATE_SYSTEM_FOCUSED;
            if(item->id==pressedId)out->lVal|=STATE_SYSTEM_PRESSED;
            if(item->id==100+page)out->lVal|=STATE_SYSTEM_SELECTED;
            if(page==5&&item->id==600+settingsSection)out->lVal|=STATE_SYSTEM_SELECTED;
            if(item->id==311&&motionReduced())out->lVal|=STATE_SYSTEM_CHECKED;
            if(item->id==3060&&personal.preferences.memory)out->lVal|=STATE_SYSTEM_CHECKED;
            if(item->id==1000+soundTab&&page==6)out->lVal|=STATE_SYSTEM_SELECTED;
            if((item->id==1020&&sound.reading[0].muted)||(item->id==1021&&sound.reading[1].muted))out->lVal|=STATE_SYSTEM_CHECKED;
            if(item->id>=380&&item->id<380+hext::MetricCount&&metricShown(item->id-380))out->lVal|=STATE_SYSTEM_CHECKED;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT,BSTR* out) override{if(!out){return E_POINTER;}
        *out=nullptr;return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* file,VARIANT,LONG* topic) override{
        if(file){*file=nullptr;}
        if(topic){*topic=0;}
        return S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT,BSTR* out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;return S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);
        int target=assistantFocused?450:inputFocused?240:keyboardId;
        for(size_t i=0;i<hits.size();i++)if(hits[i].id==target){
            out->vt=VT_I4;out->lVal=static_cast<LONG>(i+1);return S_OK;
        }
        out->vt=VT_I4;out->lVal=CHILDID_SELF;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);
        for(size_t i=0;i<hits.size();i++)if(hits[i].id==100+page){
            out->vt=VT_I4;out->lVal=static_cast<LONG>(i+1);break;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT id,BSTR* out) override{
        if(!out){return E_POINTER;}
        *out=nullptr;auto item=child(id);if(!item)return S_FALSE;
        if(!controlFocusable(item->id))return S_FALSE;
        *out=SysAllocString(item->id==240||item->id==450?L"Editar":item->id==200||item->id==331||item->id==332||soundSlider(item->id)?L"Ajustar":L"Pressionar");return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accSelect(LONG flags,VARIANT id) override{
        auto item=child(id);if(!item)return E_INVALIDARG;
        if(flags&SELFLAG_TAKEFOCUS){
            if(!desktopEligible||!windowShown)return E_ACCESSDENIED;
            keyboardId=item->id;
            assistantFocused=item->id==450;
            if(assistantFocused)assistantCaret=assistantInput.size();
            resetAssistantCaret();
            SetForegroundWindow(windowHandle);SetFocus(windowHandle);placeOnDesktop();invalidate();
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accLocation(LONG* x,LONG* y,LONG* w,LONG* h,VARIANT id) override{
        if(!x||!y||!w||!h)return E_POINTER;
        RECT bounds{};GetWindowRect(windowHandle,&bounds);
        if(id.vt==VT_I4&&id.lVal==0){*x=bounds.left;*y=bounds.top;*w=bounds.right-bounds.left;*h=bounds.bottom-bounds.top;return S_OK;}
        auto item=child(id);if(!item)return E_INVALIDARG;
        *x=bounds.left+static_cast<LONG>(item->rect.left*dpi);*y=bounds.top+static_cast<LONG>(item->rect.top*dpi);
        *w=static_cast<LONG>((item->rect.right-item->rect.left)*dpi);*h=static_cast<LONG>((item->rect.bottom-item->rect.top)*dpi);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accNavigate(LONG direction,VARIANT from,VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);LONG at=from.vt==VT_I4?from.lVal:0;
        if(direction==NAVDIR_FIRSTCHILD)at=1;else if(direction==NAVDIR_LASTCHILD)at=static_cast<LONG>(hits.size());
        else if(direction==NAVDIR_NEXT||direction==NAVDIR_DOWN)at++;
        else if(direction==NAVDIR_PREVIOUS||direction==NAVDIR_UP)at--;else return S_FALSE;
        if(at<1||at>static_cast<LONG>(hits.size()))return S_FALSE;
        out->vt=VT_I4;out->lVal=at;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accHitTest(LONG x,LONG y,VARIANT* out) override{
        if(!out){return E_POINTER;}
        VariantInit(out);
        RECT bounds{};GetWindowRect(windowHandle,&bounds);
        if(x<bounds.left||x>=bounds.right||y<bounds.top||y>=bounds.bottom)return S_FALSE;
        out->vt=VT_I4;out->lVal=CHILDID_SELF;
        int target=hitAt((x-bounds.left)/dpi,(y-bounds.top)/dpi);
        for(size_t i=0;i<hits.size();i++)if(hits[i].id==target){out->lVal=static_cast<LONG>(i+1);break;}
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT id) override{
        auto item=child(id);if(!item)return E_INVALIDARG;
        if(!controlFocusable(item->id))return S_FALSE;
        if(!controlEnabled(item->id))return E_ACCESSDENIED;
        if(!desktopEligible||!windowShown)return E_ACCESSDENIED;
        PostMessage(windowHandle,WM_APP+5,item->id,0);return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT,BSTR) override{return E_ACCESSDENIED;}
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT id,BSTR value) override{
        auto item=child(id);if(!item)return E_INVALIDARG;
        if(!desktopEligible||!windowShown)return E_ACCESSDENIED;
        if(soundSlider(item->id)){
            if(!value||!controlEnabled(item->id))return E_INVALIDARG;
            wchar_t* end=nullptr;long requested=wcstol(value,&end,10);
            if(end==value||*end||requested<0||requested>100)return E_INVALIDARG;
            soundSliderValue(item->id,requested/100.f);return S_OK;
        }
        if(item->id==200||item->id==332){
            if(!value)return E_INVALIDARG;
            wchar_t* end=nullptr;
            long requested=wcstol(value,&end,10);
            if(end==value||*end||requested<0||requested>100)return E_INVALIDARG;
            PostMessage(windowHandle,item->id==200?WM_APP+6:WM_APP+7,requested,0);return S_OK;
        }
        if(item->id==450){
            assistantInput=value?std::wstring(value,SysStringLen(value)).substr(0,4000):L"";
            assistantCaret=assistantInput.size();resetAssistantCaret();invalidate();return S_OK;
        }
        if(item->id!=240)return E_ACCESSDENIED;
        command=value?std::wstring(value,SysStringLen(value)).substr(0,120):L"";invalidate();return S_OK;
    }
};
IAccessible* accessibleUI=nullptr;
void notifyAccessibility(){
    if(testMode||!accessibleUI)return;
    static size_t previousSignature=0;
    static int previousFocus=-1;
    size_t signature=0;
    for(auto& h:hits)signature=signature*16777619u+std::hash<std::wstring>{}(h.name)+h.id;
    if(signature!=previousSignature){
        previousSignature=signature;
        NotifyWinEvent(EVENT_OBJECT_REORDER,windowHandle,OBJID_CLIENT,CHILDID_SELF);
    }
    int target=assistantFocused?450:inputFocused?240:keyboardId;
    if(target!=previousFocus&&GetFocus()==windowHandle){
        previousFocus=target;
        for(size_t i=0;i<hits.size();i++)if(hits[i].id==target){
            NotifyWinEvent(EVENT_OBJECT_FOCUS,windowHandle,OBJID_CLIENT,static_cast<LONG>(i+1));break;
        }
    }
}
