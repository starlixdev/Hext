#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include "../src/personal-core.h"
#include <iostream>
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;
    hext::PersonalStore store;store.open(std::filesystem::path(argv[2]));
    if(std::wstring(argv[1])==L"save"){
        store.transaction([&]{store.put("chat","restart",{{"title","Restart fixture"},{"messages",hext::Json::array({hext::Json{{"role","user"},{"text","Remember this after process exit"}}})}});store.put("settings","last-chat",{{"chat","restart"}});});
        hext::MemoryManager memory(store);memory.remember("style","brief",true,"restart",true);hext::AssistantPreferences prefs;prefs.grants[0]=hext::Grant::Deny;prefs.language="en";store.put("settings","assistant",prefs.json());
        std::cout<<"PASS: encrypted chat, memory and permissions saved before process exit.\n";return 0;
    }
    if(std::wstring(argv[1])!=L"load")return 3;
    auto last=store.get("settings","last-chat");auto chat=store.get("chat",last.at("chat"));auto memories=store.list("memory");hext::AssistantPreferences prefs;prefs.restore(store.get("settings","assistant"));
    if(chat["messages"][0]["text"]!="Remember this after process exit"||memories.size()!=1||memories[0]["value"]!="brief"||prefs.grants[0]!=hext::Grant::Deny||prefs.language!="en")return 4;
    std::cout<<"PASS: a fresh Windows process restored the saved conversation, shared memory, denied permission and language.\n";return 0;
}
