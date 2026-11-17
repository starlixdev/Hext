#pragma once
void cancelShortcutCapture(){
    if(shortcutCaptureHook){UnhookWindowsHookEx(shortcutCaptureHook);shortcutCaptureHook=nullptr;}
    captureShortcut=-1;shortcutCaptureUntil=0;shortcutKeys.fill(false);
}
UINT capturedModifiers(){
    return (shortcutKeys[VK_LCONTROL]||shortcutKeys[VK_RCONTROL]||shortcutKeys[VK_CONTROL]?MOD_CONTROL:0)|
        (shortcutKeys[VK_LMENU]||shortcutKeys[VK_RMENU]||shortcutKeys[VK_MENU]?MOD_ALT:0)|
        (shortcutKeys[VK_LSHIFT]||shortcutKeys[VK_RSHIFT]||shortcutKeys[VK_SHIFT]?MOD_SHIFT:0)|
        (shortcutKeys[VK_LWIN]||shortcutKeys[VK_RWIN]?MOD_WIN:0);
}
bool modifierKey(UINT key){return key==VK_CONTROL||key==VK_MENU||key==VK_SHIFT||
    key==VK_LCONTROL||key==VK_RCONTROL||key==VK_LMENU||key==VK_RMENU||key==VK_LSHIFT||key==VK_RSHIFT||key==VK_LWIN||key==VK_RWIN;}
LRESULT CALLBACK captureShortcutKey(int code,WPARAM message,LPARAM data){
    if(code==HC_ACTION&&captureShortcut>=0){
        const auto& event=*reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);UINT key=event.vkCode;
        if(key<256){
            bool down=message==WM_KEYDOWN||message==WM_SYSKEYDOWN,repeat=shortcutKeys[key];
            shortcutKeys[key]=down;
            if(!modifierKey(key)){
                if(down&&!repeat)PostMessageW(windowHandle,WM_APP+8,key,capturedModifiers());
                return 1; // Only during explicit shortcut editing; suppress the trial combination.
            }
            if(key==VK_LWIN||key==VK_RWIN)return 1;
        }
    }
    return CallNextHookEx(shortcutCaptureHook,code,message,data);
}
void beginShortcutCapture(int slot){
    cancelShortcutCapture();captureShortcut=slot;shortcutStatus[slot].clear();
    shortcutCaptureUntil=GetTickCount64()+20000;
    if(!testMode){
        SetForegroundWindow(windowHandle);SetFocus(windowHandle);placeOnDesktop();
        for(UINT key:{VK_LCONTROL,VK_RCONTROL,VK_LMENU,VK_RMENU,VK_LSHIFT,VK_RSHIFT,VK_LWIN,VK_RWIN})shortcutKeys[key]=(GetAsyncKeyState(key)&0x8000)!=0;
        shortcutCaptureHook=SetWindowsHookExW(WH_KEYBOARD_LL,captureShortcutKey,GetModuleHandleW(nullptr),0);
        if(!shortcutCaptureHook){cancelShortcutCapture();shortcutStatus[slot]=L"Não foi possível capturar as teclas. Tente novamente.";}
    }
    inputFocused=assistantFocused=false;resetAssistantCaret();invalidate();
}
void commitShortcut(hext::Shortcut requested){
    if(captureShortcut<0)return;
    int slot=captureShortcut;
    if(requested.key==VK_ESCAPE){cancelShortcutCapture();shortcutStatus[slot]=L"Alteração cancelada.";invalidate();return;}
    auto result=shortcuts.replace(slot,requested);
    if(result==hext::ShortcutResult::Saved){
        cancelShortcutCapture();shortcutIgnoreUntil=GetTickCount64()+350;
        shortcutReleaseGuard=requested;shortcutAwaitRelease=!testMode;
        shortcutStatus[slot]=L"Atalho salvo.";saveConfig();
    }else if(result==hext::ShortcutResult::Duplicate)shortcutStatus[slot]=L"Já usado pela outra ação do Hext. Escolha outra combinação.";
    else if(result==hext::ShortcutResult::Invalid)shortcutStatus[slot]=(requested.modifiers&MOD_WIN)?
        L"Use Ctrl ou Alt. Combinações com Windows são reservadas.":requested.key==VK_F12?
        L"F12 é reservada pelo Windows. Escolha outra tecla.":L"Combine Ctrl ou Alt com uma tecla. Esc cancela.";
    else shortcutStatus[slot]=shortcuts.lastError==ERROR_HOTKEY_ALREADY_REGISTERED?
        L"Atalho já usado por outro aplicativo. Escolha outra combinação.":L"O Windows não permitiu este atalho. Escolha outra combinação.";
    invalidate();
}
void finishShortcutOverlay(){
    bool wasOverlay=shortcutOverlay;
    shortcutOverlay=shortcutClosing=shortcutHold=false;
    if(!testMode&&wasOverlay){
        SetWindowLongPtrW(windowHandle,GWL_EXSTYLE,GetWindowLongPtrW(windowHandle,GWL_EXSTYLE)|WS_EX_NOACTIVATE);
        SetWindowPos(windowHandle,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        if(GetForegroundWindow()==windowHandle&&IsWindow(shortcutReturnWindow))SetForegroundWindow(shortcutReturnWindow);
        placeOnDesktop();
    }
    shortcutReturnWindow=nullptr;
}
void toggleShortcutPanel(){
    if(expanded){dismissIsland();return;}
    hidden=false;visibility.awaitLeave=false;
    if(!shortcutOverlay)shortcutReturnWindow=testMode?nullptr:GetForegroundWindow();
    shortcutOverlay=true;shortcutClosing=false;shortcutHold=true;desktopEligible=true;
    if(!testMode)SetWindowLongPtrW(windowHandle,GWL_EXSTYLE,GetWindowLongPtrW(windowHandle,GWL_EXSTYLE)&~static_cast<LONG_PTR>(WS_EX_NOACTIVATE));
    if(!windowShown||!IsWindowVisible(windowHandle)||IsIconic(windowHandle))restoreDesktopCompact();
    visibility.expanded=true;visibility.compactVisible=true;appearance.target=1;
    setExpanded(true);
    if(!testMode){
        SetForegroundWindow(windowHandle);
        if(GetForegroundWindow()!=windowHandle){
            // Explicit keyboard opening is interactive. Attach only to a responsive
            // foreground queue, and detach immediately after the focus handoff.
            HWND foreground=GetForegroundWindow();DWORD_PTR ready=0;
            DWORD thread=foreground?GetWindowThreadProcessId(foreground,nullptr):0;
            DWORD own=GetCurrentThreadId();
            if(thread&&thread!=own&&SendMessageTimeoutW(foreground,WM_NULL,0,0,SMTO_ABORTIFHUNG,100,&ready)&&AttachThreadInput(own,thread,TRUE)){
                SetForegroundWindow(windowHandle);SetFocus(windowHandle);AttachThreadInput(own,thread,FALSE);
            }
        }else SetFocus(windowHandle);
        placeOnDesktop();
    }
    invalidate(450);
}
void toggleShortcutVisibility(){
    cancelShortcutCapture();
    if(!hidden){
        hidden=true;shortcutHold=false;shortcutClosing=false;beginDesktopExit();
        if(testMode)hideImmediately();
    }else{
        hidden=false;visibility.awaitLeave=true;
        if(shortcutOverlay)finishShortcutOverlay();
        if(appearance.target==0){appearance.target=1;setExpanded(false);visibility.expanded=false;}
        if(testMode){appearance.target=1;}
        else updateVisibility();
    }
}
