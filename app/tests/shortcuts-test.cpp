#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cassert>
#include <map>
#include <iostream>
#include "../src/shortcuts.h"

int main(int argc,char** argv){
    using namespace hext;
    assert(DefaultShortcuts[0].label()==L"Ctrl + Alt + H");
    assert(DefaultShortcuts[1].label()==L"Ctrl + Alt + Espaço");
    assert(!(Shortcut{0,'A'}.valid()));
    assert(!(Shortcut{MOD_WIN,'D'}.valid()));
    assert(!(Shortcut{MOD_CONTROL,VK_F12}.valid()));
    assert(!(Shortcut{MOD_SHIFT,'A'}.valid()));
    assert((Shortcut{MOD_CONTROL|MOD_SHIFT,'A'}.valid()));
    assert((Shortcut{MOD_ALT,VK_F24}.valid()));
    std::map<int,Shortcut> reserved;
    reserved[99]={MOD_CONTROL|MOD_SHIFT,'X'}; // Another application.
    Shortcuts keys;
    keys.acquire=[&](int id,Shortcut s){
        for(const auto& entry:reserved)if(entry.second==s){SetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);return false;}
        reserved[id]=s;return true;
    };
    keys.relinquish=[&](int id){reserved.erase(id);};
    assert(keys.replace(0,keys.bindings[0])==ShortcutResult::Saved);
    assert(keys.replace(1,keys.bindings[1])==ShortcutResult::Saved);
    auto before=keys.registeredIDs;auto original=keys.bindings;
    assert(keys.replace(0,{MOD_CONTROL|MOD_SHIFT,'X'})==ShortcutResult::Unavailable);
    assert(keys.lastError==ERROR_HOTKEY_ALREADY_REGISTERED);
    assert(keys.registeredIDs==before&&keys.bindings==original&&reserved.size()==3);
    assert(keys.replace(0,keys.bindings[1])==ShortcutResult::Duplicate);
    assert(keys.registeredIDs==before&&reserved.size()==3);
    assert(keys.replace(0,{MOD_CONTROL|MOD_ALT,'J'})==ShortcutResult::Saved);
    assert(keys.registeredIDs[0]!=before[0]&&reserved.count(before[0])==0);
    assert(keys.slotFor(keys.registeredIDs[0])==0&&keys.slotFor(keys.registeredIDs[1])==1);
    assert(keys.slotFor(99)==-1);
    assert(keys.replace(0,DefaultShortcuts[0])==ShortcutResult::Saved);
    before=keys.registeredIDs;
    assert(keys.replace(0,DefaultShortcuts[0])==ShortcutResult::Saved&&keys.registeredIDs==before);
    keys.releaseAll();assert(reserved.size()==1&&reserved.count(99));
    if(argc>1&&std::string(argv[1])=="--skip-native"){
        std::cout<<"PASS: shortcut replacement and conflict model; native desktop registration skipped by request\n";return 0;
    }
    HWND first=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Hext test A",WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HWND second=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"Hext test B",WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    assert(first&&second);
    bool checked=false;
    for(UINT key=VK_F13;key<=VK_F24;key++){
        UINT modifiers=MOD_CONTROL|MOD_ALT|MOD_SHIFT|MOD_NOREPEAT;
        if(!RegisterHotKey(first,1000,modifiers,key))continue;
        SetLastError(0);assert(!RegisterHotKey(second,1001,modifiers,key));
        assert(GetLastError()==ERROR_HOTKEY_ALREADY_REGISTERED);
        UnregisterHotKey(first,1000);
        assert(RegisterHotKey(second,1001,modifiers,key));UnregisterHotKey(second,1001);
        checked=true;break;
    }
    DestroyWindow(first);DestroyWindow(second);
    std::cout<<"PASS: separate shortcuts, validation, conflict detection, duplicate prevention, atomic replacement, restore and cleanup\n";
    std::cout<<(checked?"PASS: Windows RegisterHotKey detects a conflicting reservation and accepts it after release; no keypress injected\n":
        "SKIP: native hotkey reservation unavailable in this Windows desktop session; conflict model passed\n");
}
