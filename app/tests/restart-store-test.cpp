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
        hext::MemoryManager memory(store);memory.remember("style","brief",true,"restart",true);hext::AssistantPreferences prefs;prefs.grants[0]=hext::Grant::Deny;prefs.language="en";prefs.serviceGrants["spotify"]=hext::Grant::Deny;prefs.toolGrants["files.read_text"]=hext::Grant::Ask;store.put("settings","assistant",prefs.json());
        hext::TaskSession task;task.begin("restart","Resume original objective after an uncertain script");auto command=hext::actionRequest({{"name","terminal.run"},{"arguments",{{"script","'fixture'"},{"purpose","Restart probe"}}}});if(!command||!task.dispatch(*command))return 5;store.put("task","restart",{{"task",task.checkpoint()}});
        std::cout<<"PASS: encrypted chat, memory, permissions and in-flight task saved before process exit.\n";return 0;
    }
    if(std::wstring(argv[1])!=L"load")return 3;
    auto last=store.get("settings","last-chat");auto chat=store.get("chat",last.at("chat"));auto memories=store.list("memory");hext::AssistantPreferences prefs;prefs.restore(store.get("settings","assistant"));
    if(chat["messages"][0]["text"]!="Remember this after process exit"||memories.size()!=1||memories[0]["value"]!="brief"||prefs.grants[0]!=hext::Grant::Deny||prefs.language!="en"||prefs.serviceGrants.at("spotify")!=hext::Grant::Deny||prefs.toolGrants.at("files.read_text")!=hext::Grant::Ask)return 4;
    hext::TaskSession task;if(!task.restore(store.get("task","restart")["task"])||task.stage!=hext::TaskStage::Paused||!task.inFlight)return 6;auto request=*task.inFlight;task.resumePlanning();if(task.active()==false||task.goal!="Resume original objective after an uncertain script"||task.inFlight||task.dispatch(request))return 7;
    std::cout<<"PASS: a fresh Windows process restored conversation, memory, permissions, language and original task; no in-flight effect was replayed.\n";return 0;
}
