#pragma once
#include "agent-process.h"
#include <uiautomation.h>
#include <dwmapi.h>
namespace hext {
template<class T>struct AgentCOM{T* value=nullptr;~AgentCOM(){if(value)value->Release();}T** address(){return &value;}T* operator->()const{return value;}};
inline std::string agentBSTR(BSTR value){auto result=value?utf8(std::wstring(value,SysStringLen(value))):"";if(value)SysFreeString(value);return utf8Excerpt(result,0,512);}
inline std::string processIdentity(HANDLE process){FILETIME created{},exit{},kernel{},user{};if(!GetProcessTimes(process,&created,&exit,&kernel,&user))return {};ULARGE_INTEGER time{};time.LowPart=created.dwLowDateTime;time.HighPart=created.dwHighDateTime;return std::to_string(time.QuadPart);}
inline bool processAtMostMedium(HANDLE process){
    OwnedHandle token;if(!OpenProcessToken(process,TOKEN_QUERY,&token.value))return false;DWORD size=0;GetTokenInformation(token.value,TokenIntegrityLevel,nullptr,0,&size);std::vector<BYTE> buffer(size);
    if(!size||!GetTokenInformation(token.value,TokenIntegrityLevel,buffer.data(),size,&size))return false;
    auto label=reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buffer.data());auto count=*GetSidSubAuthorityCount(label->Label.Sid);return count&&*GetSidSubAuthority(label->Label.Sid,count-1)<=SECURITY_MANDATORY_MEDIUM_RID;
}
inline std::optional<Json> agentWindow(HWND window){
    if(!IsWindow(window)||!IsWindowVisible(window)||IsIconic(window)||GetAncestor(window,GA_ROOT)!=window)return {};
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(!pid||pid==GetCurrentProcessId())return {};
    BOOL cloaked=FALSE;DwmGetWindowAttribute(window,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));if(cloaked)return {};DWORD affinity=0;if(GetWindowDisplayAffinity(window,&affinity)&&affinity)return {};
    wchar_t title[513]{},className[256]{};GetWindowTextW(window,title,513);GetClassNameW(window,className,256);if(!*title)return {};auto cls=std::wstring(className);if(cls==L"Progman"||cls==L"WorkerW"||cls==L"Shell_TrayWnd"||cls==L"HextIslandWindow")return {};
    OwnedHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));if(!process.value||!processAtMostMedium(process.value))return {};
    wchar_t path[32768]{};DWORD length=32768;if(!QueryFullProcessImageNameW(process.value,0,path,&length))return {};auto name=lower(utf8(std::filesystem::path(path).filename().wstring()));
    for(auto forbidden:{"consent.exe","credentialuibroker.exe","logonui.exe","lockapp.exe","lsass.exe","winlogon.exe","cmd.exe","powershell.exe","pwsh.exe","conhost.exe","windowsterminal.exe","regedit.exe","taskmgr.exe","mmc.exe","spotify.exe"})if(name==forbidden)return {};
    if(lower(utf8(title)).find("spotify")!=std::string::npos)return {};
    auto created=processIdentity(process.value);if(created.empty())return {};
    return Json{{"hwnd",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(window))},{"pid",pid},{"created",created},{"title",utf8(title)},{"application",name},{"class",utf8(className)}};
}
inline bool validAgentWindow(const Json& binding){
    if(!binding.is_object()||!binding.contains("hwnd")||!binding["hwnd"].is_number_unsigned())return false;
    auto current=agentWindow(reinterpret_cast<HWND>(binding["hwnd"].get<uintptr_t>()));return current&&current->value("pid",0u)==binding.value("pid",0u)&&current->value("created","")==binding.value("created","")&&current->value("application","")==binding.value("application","");
}
inline Json agentWindows(){Json result=Json::array();EnumWindows([](HWND window,LPARAM parameter)->BOOL{auto& rows=*reinterpret_cast<Json*>(parameter);if(rows.size()>=40)return FALSE;if(auto info=agentWindow(window)){(*info)["id"]=randomID();rows.push_back(*info);}return TRUE;},reinterpret_cast<LPARAM>(&result));return result;}
inline std::string runtimeID(IUIAutomationElement* element){SAFEARRAY* array=nullptr;if(FAILED(element->GetRuntimeId(&array))||!array)return {};LONG first=0,last=-1;SafeArrayGetLBound(array,1,&first);SafeArrayGetUBound(array,1,&last);std::string id;for(LONG i=first;i<=last&&i-first<24;i++){int part=0;if(SUCCEEDED(SafeArrayGetElement(array,&i,&part)))id+=std::to_string(part)+".";}SafeArrayDestroy(array);return id;}
inline bool privateUIElement(IUIAutomationElement* element){BOOL password=TRUE;if(FAILED(element->get_CurrentIsPassword(&password))||password)return true;BSTR name=nullptr,id=nullptr;element->get_CurrentName(&name);element->get_CurrentAutomationId(&id);auto label=lower(agentBSTR(name)+" "+agentBSTR(id));for(auto word:{"password","senha","one-time","verification code","código de verificação","creditcard","card number","access token","secret key"})if(label.find(word)!=std::string::npos)return true;return false;}
inline Json agentElement(IUIAutomationElement* element){
    BSTR name=nullptr,id=nullptr;element->get_CurrentName(&name);element->get_CurrentAutomationId(&id);CONTROLTYPEID type=0;BOOL enabled=FALSE,offscreen=TRUE;element->get_CurrentControlType(&type);element->get_CurrentIsEnabled(&enabled);element->get_CurrentIsOffscreen(&offscreen);
    Json out={{"element",runtimeID(element)},{"name",agentBSTR(name)},{"automation_id",agentBSTR(id)},{"type",type},{"enabled",enabled!=FALSE},{"offscreen",offscreen!=FALSE}};
    Json patterns=Json::array();for(auto kind:std::vector<std::pair<PATTERNID,const char*>>{{UIA_InvokePatternId,"invoke"},{UIA_ValuePatternId,"set_value"},{UIA_TogglePatternId,"toggle"},{UIA_SelectionItemPatternId,"select"},{UIA_ExpandCollapsePatternId,"expand"},{UIA_ScrollPatternId,"scroll"}}){AgentCOM<IUnknown> pattern;if(SUCCEEDED(element->GetCurrentPattern(kind.first,pattern.address()))&&pattern.value)patterns.push_back(kind.second);}out["patterns"]=patterns;
    AgentCOM<IUIAutomationValuePattern> value;if(SUCCEEDED(element->GetCurrentPatternAs(UIA_ValuePatternId,IID_IUIAutomationValuePattern,reinterpret_cast<void**>(value.address())))&&value.value){BSTR text=nullptr;if(SUCCEEDED(value->get_CurrentValue(&text)))out["value"]=agentBSTR(text);}
    return out;
}
inline Json agentDesktopOperation(const Json& request){
    auto binding=request.at("binding");if(!validAgentWindow(binding))return {{"success",false},{"error","The authorized window closed, changed owner, became protected or is elevated."},{"recoverable",true}};
    AgentCOM<IUIAutomation> automation;if(FAILED(CoCreateInstance(CLSID_CUIAutomation,nullptr,CLSCTX_INPROC_SERVER,IID_IUIAutomation,reinterpret_cast<void**>(automation.address()))))throw std::runtime_error("UI Automation unavailable");
    AgentCOM<IUIAutomationElement> root;auto window=reinterpret_cast<HWND>(binding["hwnd"].get<uintptr_t>());if(FAILED(automation->ElementFromHandle(window,root.address()))||!root.value)throw std::runtime_error("UI root unavailable");
    AgentCOM<IUIAutomationTreeWalker> walker;if(FAILED(automation->get_ControlViewWalker(walker.address())))throw std::runtime_error("UI walker unavailable");
    Json elements=Json::array();AgentCOM<IUIAutomationElement> target;auto operation=request.value("operation","inspect");auto wanted=request.value("element","");size_t bytes=0;int visited=0;ULONGLONG until=GetTickCount64()+6000;
    std::function<void(IUIAutomationElement*,int)> walk=[&](IUIAutomationElement* current,int depth){
        if(!current||depth>12||visited++>=160||GetTickCount64()>until||bytes>=24000||privateUIElement(current))return;
        int pid=0;BOOL offscreen=TRUE;if(FAILED(current->get_CurrentIsOffscreen(&offscreen))||offscreen||FAILED(current->get_CurrentProcessId(&pid))||static_cast<DWORD>(pid)!=binding["pid"].get<DWORD>())return;
        auto value=agentElement(current);value["depth"]=depth;bytes+=value.dump().size();elements.push_back(value);
        if(!wanted.empty()&&value["element"]==wanted&&!target.value){current->AddRef();target.value=current;}
        AgentCOM<IUIAutomationElement> child;if(SUCCEEDED(walker->GetFirstChildElement(current,child.address()))){while(child.value&&visited<160&&bytes<24000&&GetTickCount64()<until){walk(child.value,depth+1);AgentCOM<IUIAutomationElement> next;if(FAILED(walker->GetNextSiblingElement(child.value,next.address())))break;child.value->Release();child.value=next.value;next.value=nullptr;}}
    };walk(root.value,0);
    for(auto& element:elements){auto exposed=lower(element.value("name","")+" "+element.value("value",""));if(exposed.find("spotify.com")!=std::string::npos||exposed=="spotify")return {{"success",false},{"error","Spotify content is kept out of general automation/provider context. Use the native music capability."}};}
    if(operation=="inspect")return {{"success",true},{"elements",elements},{"truncated",visited>=160||bytes>=24000||GetTickCount64()>=until},{"untrusted",true},{"window_title",agentBSTR([&]{BSTR title=nullptr;root->get_CurrentName(&title);return title;}())}};
    if(!target.value||!validAgentWindow(binding)||privateUIElement(target.value))return {{"success",false},{"error","Element unavailable or protected; inspect again."},{"recoverable",true}};
    auto now=agentElement(target.value);auto expected=request.at("expected");now.erase("depth");expected.erase("depth");if(now!=expected)return {{"success",false},{"error","The control changed after inspection. No action was sent; inspect again."},{"recoverable",true}};
    BOOL enabled=FALSE;target->get_CurrentIsEnabled(&enabled);if(!enabled)return {{"success",false},{"error","Control is disabled."},{"recoverable",true}};
    HRESULT hr=E_NOINTERFACE;bool verified=false,attempted=false;
    if(operation=="invoke"){
        AgentCOM<IUIAutomationInvokePattern> p;
        auto available=target->GetCurrentPatternAs(UIA_InvokePatternId,IID_IUIAutomationInvokePattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){attempted=true;hr=p->Invoke();}
    }else if(operation=="set_value"){
        AgentCOM<IUIAutomationValuePattern> p;
        auto available=target->GetCurrentPatternAs(UIA_ValuePatternId,IID_IUIAutomationValuePattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){
            BOOL readOnly=TRUE;p->get_CurrentIsReadOnly(&readOnly);
            if(!readOnly){auto text=request.value("text","");auto argument=SysAllocString(wide(text).c_str());
                if(argument){attempted=true;hr=p->SetValue(argument);SysFreeString(argument);if(SUCCEEDED(hr)){BSTR value=nullptr;hr=p->get_CurrentValue(&value);verified=SUCCEEDED(hr)&&agentBSTR(value)==text;}}
                else hr=E_OUTOFMEMORY;
            }
        }
    }else if(operation=="toggle"){
        AgentCOM<IUIAutomationTogglePattern> p;
        auto available=target->GetCurrentPatternAs(UIA_TogglePatternId,IID_IUIAutomationTogglePattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){ToggleState before{},after{};p->get_CurrentToggleState(&before);attempted=true;hr=p->Toggle();if(SUCCEEDED(hr))verified=SUCCEEDED(p->get_CurrentToggleState(&after))&&after!=before;}
    }else if(operation=="select"){
        AgentCOM<IUIAutomationSelectionItemPattern> p;
        auto available=target->GetCurrentPatternAs(UIA_SelectionItemPatternId,IID_IUIAutomationSelectionItemPattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){attempted=true;hr=p->Select();BOOL selected=FALSE;verified=SUCCEEDED(hr)&&SUCCEEDED(p->get_CurrentIsSelected(&selected))&&selected;}
    }else if(operation=="expand"||operation=="collapse"){
        AgentCOM<IUIAutomationExpandCollapsePattern> p;
        auto available=target->GetCurrentPatternAs(UIA_ExpandCollapsePatternId,IID_IUIAutomationExpandCollapsePattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){attempted=true;hr=operation=="expand"?p->Expand():p->Collapse();ExpandCollapseState state{};verified=SUCCEEDED(hr)&&SUCCEEDED(p->get_CurrentExpandCollapseState(&state))&&state==(operation=="expand"?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed);}
    }else if(operation=="scroll_down"||operation=="scroll_up"){
        AgentCOM<IUIAutomationScrollPattern> p;
        auto available=target->GetCurrentPatternAs(UIA_ScrollPatternId,IID_IUIAutomationScrollPattern,reinterpret_cast<void**>(p.address()));
        if(SUCCEEDED(available)&&p.value){attempted=true;hr=p->Scroll(ScrollAmount_NoAmount,operation=="scroll_down"?ScrollAmount_LargeIncrement:ScrollAmount_LargeDecrement);}
    }
    return {{"success",SUCCEEDED(hr)},{"accepted",SUCCEEDED(hr)},{"verified",verified},{"hresult",hr},{"operation",operation},{"note",verified?"Control state read back successfully.":"Only API acceptance is known. Inspect the application to verify the actual goal; never infer a completed send/install from a button invocation."},{"uncertain",attempted&&FAILED(hr)},{"recoverable",!attempted&&FAILED(hr)}};
}
inline int agentDesktopWorker(){
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);std::string input;char block[2048];DWORD read=0;
    while(input.size()<8192&&ReadFile(GetStdHandle(STD_INPUT_HANDLE),block,sizeof(block),&read,nullptr)&&read)input.append(block,read);
    Json result;try{auto parsed=parseJSON(input,8192);if(!parsed)throw std::runtime_error("Invalid request");result=agentDesktopOperation(*parsed);}catch(...){result={{"success",false},{"error","The native accessibility provider did not complete this operation."},{"recoverable",true}};}
    auto output=result.dump(-1,' ',false,Json::error_handler_t::replace);DWORD written=0;WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),output.data(),static_cast<DWORD>(output.size()),&written,nullptr);CoUninitialize();return result.value("success",false)?0:1;
}
inline Json agentDesktopCall(const std::filesystem::path& executable,const Json& request,std::atomic<bool>& cancel){
    auto child=agentProcess(executable.wstring(),{L"--agent-desktop-worker"},std::filesystem::temp_directory_path(),cancel,10000,nullptr,request.dump());
    auto result=parseJSON(child.output);if(result&&result->is_object())return *result;
    return {{"success",false},{"error",child.timedOut?"The accessibility provider timed out. No alternative input was sent.":"The accessibility worker failed or was cancelled."},{"uncertain",request.value("operation","inspect")!="inspect"},{"recoverable",request.value("operation","inspect")=="inspect"}};
}
}
