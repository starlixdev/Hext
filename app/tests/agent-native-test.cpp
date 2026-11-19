#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../src/agent-web.h"
#include "../src/agent-desktop.h"
#include "../src/agent-packages.h"
#include "../src/task-engine.h"
#include <cassert>
#include <iostream>
LRESULT CALLBACK fixtureProc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_COMMAND&&LOWORD(wp)==102){wchar_t value[256]{};GetWindowTextW(GetDlgItem(window,101),value,256);SetWindowTextW(GetDlgItem(window,103),(L"Applied: "+std::wstring(value)).c_str());return 0;}
    if(message==WM_CLOSE){DestroyWindow(window);return 0;}if(message==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(window,message,wp,lp);
}
int fixture(const wchar_t* title){
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=fixtureProc;cls.lpszClassName=L"HextAgentNativeFixture";RegisterClassW(&cls);
    auto window=CreateWindowW(cls.lpszClassName,title,WS_OVERLAPPEDWINDOW,40,180,460,240,nullptr,nullptr,cls.hInstance,nullptr);
    CreateWindowW(L"EDIT",L"initial",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,20,25,250,28,window,reinterpret_cast<HMENU>(101),cls.hInstance,nullptr);
    CreateWindowW(L"BUTTON",L"Apply fixture",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,285,25,120,30,window,reinterpret_cast<HMENU>(102),cls.hInstance,nullptr);
    CreateWindowW(L"STATIC",L"Idle",WS_CHILD|WS_VISIBLE,20,80,380,30,window,reinterpret_cast<HMENU>(103),cls.hInstance,nullptr);
    CreateWindowW(L"EDIT",L"must-not-reach-model",WS_CHILD|WS_VISIBLE|ES_PASSWORD|WS_BORDER,20,125,250,25,window,reinterpret_cast<HMENU>(104),cls.hInstance,nullptr);
    ShowWindow(window,SW_SHOWNOACTIVATE);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;
}
int wmain(int argc,wchar_t** argv){
    using namespace hext;
    if(argc>1&&std::wstring(argv[1])==L"--agent-desktop-worker")return agentDesktopWorker();
    if(argc>2&&std::wstring(argv[1])==L"--fixture")return fixture(argv[2]);
    if(argc>1&&std::wstring(argv[1])==L"--linger"){Sleep(10000);return 0;}
    auto executable=std::filesystem::absolute(argv[0]);std::atomic<bool> cancel{false};
    for(auto address:{"127.0.0.1","10.0.0.1","172.16.0.1","192.168.1.1","169.254.169.254","100.64.0.1"})assert(!publicIPv4(inet_addr(address)));
    assert(publicIPv4(inet_addr("1.1.1.1")));for(auto url:{"http://example.com","https://user:password@example.com/","https://example.com:444/","file:///C:/data","https://localhost/","https://example.com/\r\nInjected"})assert(!publicURL(url,false));
    for(auto id:{"--force","x;calc.exe","x&y","../package","a b"})assert(!packageToken(id,128));
    assert(packageToken("Microsoft.PowerShell",128));
    auto options=packageArguments("install","Microsoft.PowerShell","7.4.6");assert(std::find(options.begin(),options.end(),L"--exact")!=options.end()&&std::find(options.begin(),options.end(),L"--source")!=options.end());
    AssistantPreferences legacy;legacy.restore({{"grants",Json::array({0,2,1,1,1,1,0,0})}});assert(legacy.grants[0]==Grant::Deny&&legacy.grants[1]==Grant::Allow&&legacy.grants[8]==Grant::Ask);
    TaskSession task;auto owner=randomID();task.begin(owner,"Retain goal and investigate interrupted effects");auto write=actionRequest({{"name","terminal.run"},{"arguments",{{"script","'hello'"},{"purpose","Fixture"}}}});assert(write&&task.dispatch(*write));auto snapshot=task.checkpoint();auto root=std::filesystem::current_path()/L"dist"/(L"agent-restart-"+wide(randomID()));PersonalStore store;store.open(root);store.put("task",owner,{{"task",snapshot}});TaskSession restored;assert(restored.restore(store.get("task",owner)["task"])&&restored.stage==TaskStage::Paused&&!restored.active()&&restored.inFlight);restored.resumePlanning();assert(restored.goal==task.goal&&!restored.inFlight&&restored.outcomes.back()["uncertain"]==true&&!restored.dispatch(*write));
    auto child=agentProcess(executable.wstring(),{L"--linger"},std::filesystem::temp_directory_path(),cancel,100);assert(child.timedOut&&child.code!=0);cancel=true;auto cancelled=agentProcess(executable.wstring(),{L"--linger"},std::filesystem::temp_directory_path(),cancel,100);assert(cancelled.cancelled);cancel=false;
    std::cout<<"PASS: strict public HTTPS/package inputs, legacy permission migration, encrypted task checkpoint, uncertain effects never replayed, bounded owned-child lifecycle.\n";
    if(argc>1&&std::wstring(argv[1])==L"--desktop"){
        auto title=L"Hext verification "+wide(randomID());auto command=quoteArgument(executable.wstring())+L" --fixture "+quoteArgument(title);std::vector<wchar_t> mutableCommand(command.begin(),command.end());mutableCommand.push_back(0);STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};OwnedHandle fixtureJob(CreateJobObjectW(nullptr,nullptr));JOBOBJECT_EXTENDED_LIMIT_INFORMATION fixtureLimits{};fixtureLimits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;assert(fixtureJob.value&&SetInformationJobObject(fixtureJob.value,JobObjectExtendedLimitInformation,&fixtureLimits,sizeof(fixtureLimits)));assert(CreateProcessW(executable.c_str(),mutableCommand.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process));OwnedHandle owned(process.hProcess),thread(process.hThread);assert(AssignProcessToJobObject(fixtureJob.value,owned.value));HWND window=nullptr;auto until=GetTickCount64()+10000;while(!window&&GetTickCount64()<until){window=FindWindowW(L"HextAgentNativeFixture",title.c_str());Sleep(30);}assert(window);
        auto binding=agentWindow(window);assert(binding);auto observed=agentDesktopCall(executable,{{"binding",*binding},{"operation","inspect"}},cancel);assert(observed.value("success",false)&&observed.dump().find("must-not-reach-model")==std::string::npos);
        Json edit=nullptr,button=nullptr;for(auto& element:observed["elements"]){if(element.value("value","")=="initial")edit=element;if(element.value("name","")=="Apply fixture")button=element;}assert(edit.is_object()&&button.is_object());
        auto unsupported=agentDesktopCall(executable,{{"binding",*binding},{"operation","toggle"},{"element",edit["element"]},{"text",""},{"expected",edit}},cancel);assert(!unsupported.value("success",false)&&unsupported.value("recoverable",false)&&!unsupported.value("uncertain",false));
        auto applied=agentDesktopCall(executable,{{"binding",*binding},{"operation","set_value"},{"element",edit["element"]},{"text","verified-native-value"},{"expected",edit}},cancel);assert(applied.value("success",false)&&applied.value("verified",false));
        auto stale=agentDesktopCall(executable,{{"binding",*binding},{"operation","set_value"},{"element",edit["element"]},{"text","must-be-rejected"},{"expected",edit}},cancel);assert(!stale.value("success",false));
        auto invoked=agentDesktopCall(executable,{{"binding",*binding},{"operation","invoke"},{"element",button["element"]},{"text",""},{"expected",button}},cancel);assert(invoked.value("success",false)&&!invoked.value("verified",false));
        observed=agentDesktopCall(executable,{{"binding",*binding},{"operation","inspect"}},cancel);assert(observed.value("success",false)&&observed.dump().find("Applied: verified-native-value")!=std::string::npos);PostMessageW(window,WM_CLOSE,0,0);assert(WaitForSingleObject(owned.value,5000)==WAIT_OBJECT_0);assert(!validAgentWindow(*binding));
        std::cout<<"PASS: actual Windows UIA control observation, password exclusion, verified text entry, stale-state rejection, button invocation and separately observed final result; closed-window scope invalidated.\n";
    }
    if(argc>1&&std::wstring(argv[1])==L"--web"){
        auto blocked=agentReadWeb("https://127.0.0.1/",cancel);assert(!blocked.value("success",false));auto page=agentReadWeb("https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview",cancel);std::cout<<"HTTPS status="<<page.value("http_status",0)<<" success="<<page.value("success",false)<<"\n";assert(page.value("success",false)&&page.contains("text"));
        auto search=agentReadWeb("https://www.bing.com/search?format=rss&q=Microsoft%20UI%20Automation%20documentation",cancel);std::cout<<"Search status="<<search.value("http_status",0)<<" success="<<search.value("success",false)<<" links="<<search.value("links",Json::array()).size()<<"\n";assert(search.value("success",false)&&!search.value("links",Json::array()).empty());
        auto winget=verifiedWinGet();std::cout<<"Signed Microsoft WinGet available="<<!winget.empty()<<"\n";assert(!winget.empty());assert(officialWinGetSource(winget,cancel));auto packages=agentPackages("packages.search",{{"query","Microsoft.PowerShell"}},cancel);std::cout<<"Actual package search="<<packages.value("success",false)<<"\n";assert(packages.value("success",false));
    }
    return 0;
}
