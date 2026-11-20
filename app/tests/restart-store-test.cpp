#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include "../src/personal-core.h"
#include "../src/task-engine.h"
#include <iostream>
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;
    hext::PersonalStore store;store.open(std::filesystem::path(argv[2]));
    if(std::wstring(argv[1])==L"save"){
        store.transaction([&]{store.put("chat","restart",{{"title","Restart fixture"},{"messages",hext::Json::array({hext::Json{{"role","user"},{"text","Remember this after process exit"}}})}});store.put("settings","last-chat",{{"chat","restart"}});});
        hext::MemoryManager memory(store);memory.remember("style","brief",true,"restart",true);hext::AssistantPreferences prefs;prefs.mode=hext::PermissionMode::Delegate;prefs.language="en";store.put("settings","assistant",prefs.json());for(int i=0;i<3;i++){auto mode=prefs;mode.mode=static_cast<hext::PermissionMode>(i);store.put("settings","mode-"+std::to_string(i),mode.json());}
        hext::TaskSession task;task.begin("restart","Resume original objective after an uncertain script");auto command=hext::actionRequest({{"name","terminal.run"},{"arguments",{{"script","'fixture'"},{"purpose","Restart probe"}}}});if(!command||!task.dispatch(*command))return 5;store.put("task","restart",{{"task",task.checkpoint()}});
        std::cout<<"PASS: encrypted chat, memory, all three global modes and in-flight task saved before process exit.\n";return 0;
    }
    if(std::wstring(argv[1])!=L"load")return 3;
    auto last=store.get("settings","last-chat");auto chat=store.get("chat",last.at("chat"));auto memories=store.list("memory");hext::AssistantPreferences prefs;prefs.restore(store.get("settings","assistant"));
    if(chat["messages"][0]["text"]!="Remember this after process exit"||memories.size()!=1||memories[0]["value"]!="brief"||prefs.mode!=hext::PermissionMode::Delegate||prefs.language!="en")return 4;
    for(int i=0;i<3;i++){hext::AssistantPreferences restored;auto value=store.get("settings","mode-"+std::to_string(i));restored.restore(value);if(static_cast<int>(restored.mode)!=i||value.contains("grants")||value.contains("services")||value.contains("tools"))return 8;}
    hext::TaskSession task;if(!task.restore(store.get("task","restart")["task"])||task.stage!=hext::TaskStage::Paused||!task.inFlight)return 6;auto request=*task.inFlight;task.resumePlanning();if(task.active()==false||task.goal!="Resume original objective after an uncertain script"||task.inFlight||task.dispatch(request))return 7;
    std::cout<<"PASS: a fresh Windows process restored conversation, memory, all three global modes, language and original task; no in-flight effect was replayed.\n";return 0;
}
