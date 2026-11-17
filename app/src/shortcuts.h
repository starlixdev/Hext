#pragma once
#include <array>
#include <functional>
#include <string>

namespace hext {
struct Shortcut {
    UINT modifiers=0,key=0;
    bool operator==(const Shortcut& other)const{return modifiers==other.modifiers&&key==other.key;}
    bool valid()const{
        if((modifiers&~(MOD_CONTROL|MOD_ALT|MOD_SHIFT))||!(modifiers&(MOD_CONTROL|MOD_ALT)))return false;
        if(key==VK_F12||key==VK_SNAPSHOT)return false;
        return key==VK_SPACE||key==VK_TAB||key==VK_RETURN||key==VK_BACK||
            (key>=VK_PRIOR&&key<=VK_DELETE)||(key>='0'&&key<='9')||(key>='A'&&key<='Z')||
            (key>=VK_NUMPAD0&&key<=VK_DIVIDE)||(key>=VK_F1&&key<=VK_F24)||(key>=0xBA&&key<=0xE2);
    }
    std::wstring label()const{
        std::wstring result;
        if(modifiers&MOD_CONTROL)result+=L"Ctrl + ";
        if(modifiers&MOD_ALT)result+=L"Alt + ";
        if(modifiers&MOD_SHIFT)result+=L"Shift + ";
        if(modifiers&MOD_WIN)result+=L"Win + ";
        if(key==VK_SPACE)return result+L"Espaço";
        if(key==VK_RETURN)return result+L"Enter";
        if(key==VK_TAB)return result+L"Tab";
        if(key>=VK_F1&&key<=VK_F24)return result+L"F"+std::to_wstring(key-VK_F1+1);
        if((key>='A'&&key<='Z')||(key>='0'&&key<='9'))return result+static_cast<wchar_t>(key);
        wchar_t name[80]{};UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);
        bool extended=key>=VK_PRIOR&&key<=VK_DELETE;
        if(GetKeyNameTextW(static_cast<LONG>((scan<<16)|(extended?1<<24:0)),name,80))return result+name;
        return result+L"Tecla "+std::to_wstring(key);
    }
};
inline const std::array<Shortcut,2> DefaultShortcuts{{{MOD_CONTROL|MOD_ALT,'H'},{MOD_CONTROL|MOD_ALT,VK_SPACE}}};
enum class ShortcutResult {Saved,Invalid,Duplicate,Unavailable};
class Shortcuts {
public:
    std::array<Shortcut,2> bindings=DefaultShortcuts;
    std::array<int,2> registeredIDs{};
    std::function<bool(int,Shortcut)> acquire;
    std::function<void(int)> relinquish;
    DWORD lastError=0;
    ShortcutResult replace(int slot,Shortcut requested){
        if(slot<0||slot>1||!requested.valid())return ShortcutResult::Invalid;
        if(requested==bindings[1-slot])return ShortcutResult::Duplicate;
        if(requested==bindings[slot]&&registeredIDs[slot])return ShortcutResult::Saved;
        int base=slot+1,spare=slot+3;
        int next=registeredIDs[slot]==base?spare:base;
        // Acquire the new binding first. Failure cannot destroy the working one.
        if(!acquire||!acquire(next,requested)){lastError=GetLastError();return ShortcutResult::Unavailable;}
        if(registeredIDs[slot]&&relinquish)relinquish(registeredIDs[slot]);
        registeredIDs[slot]=next;bindings[slot]=requested;lastError=0;
        return ShortcutResult::Saved;
    }
    int slotFor(int id)const{
        for(int slot=0;slot<2;slot++)if(registeredIDs[slot]&&registeredIDs[slot]==id)return slot;
        return -1;
    }
    void releaseAll(){for(auto& id:registeredIDs){if(id&&relinquish)relinquish(id);id=0;}}
};
}
