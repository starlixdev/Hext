#pragma once
void refreshSound(){
    audioAddon.poll();
    sound.refresh();audioApo.detect();apoEndpointReady=audioApo.endpointConfigured(sound.selected[0]);
    if(apoEndpointReady&&!audioEffectsProcess){
        std::error_code sizeError;
        auto bytes=std::filesystem::file_size(audioApo.config/L"Hext.txt",sizeError);
        if(sizeError||bytes>65536){soundEffectsDirty=true;return;}
        std::ifstream config(audioApo.config/L"Hext.txt",std::ios::binary);
        std::string actual((std::istreambuf_iterator<char>(config)),{});
        soundEffectsDirty=actual!=audioEffects.configuration(sound.selected[0]);
    }
}
void soundResult(HRESULT result){
    soundStatus=FAILED(result)?L"Não foi possível ajustar este dispositivo. Abra os ajustes do Windows.":L"";
    invalidate(180);
}
void soundAdvanced(){
    if(testMode)return;
    ShellExecuteW(windowHandle,L"open",L"control.exe",soundTab==1?L"mmsys.cpl,,1":L"mmsys.cpl,,0",nullptr,SW_SHOWNORMAL);
}
void applySoundEffects(){
    if(testMode||audioEffectsProcess)return;
    // Equalizer APO normally grants write access to its config folder. Elevate
    // only when a protected installation actually requires it.
    int direct=audioApo.apply(sound.selected[0],audioEffects);
    if(direct==0){soundEffectsDirty=false;saveConfig();soundStatus=audioEffects.enabled?L"Efeitos aplicados à saída selecionada.":L"Efeitos do Hext desativados.";invalidate();return;}
    if(direct!=3){soundStatus=direct==5?L"O arquivo Hext.txt já está em uso. Preserve-o antes de continuar.":L"Configure esta saída no Equalizer APO antes de aplicar.";invalidate();return;}
    wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
    std::wstring args=L"--apply-audio-effects \""+sound.selected[0]+L"\" "+std::to_wstring(audioEffects.enabled)+L" "+
        std::to_wstring(audioEffects.gain)+L" "+std::to_wstring(audioEffects.bass)+L" "+std::to_wstring(audioEffects.mid)+L" "+std::to_wstring(audioEffects.treble);
    SHELLEXECUTEINFOW info{sizeof(info)};info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;info.hwnd=windowHandle;
    info.lpVerb=L"runas";info.lpFile=executable;info.lpParameters=args.c_str();info.nShow=SW_HIDE;
    if(ShellExecuteExW(&info)){audioEffectsProcess=info.hProcess;soundStatus=L"Aplicando efeitos…";}
    else soundStatus=GetLastError()==ERROR_CANCELLED?L"Aplicação cancelada.":L"Não foi possível aplicar os efeitos.";
    invalidate();
}
void pollSoundEffects(){
    if(!audioEffectsProcess||WaitForSingleObject(audioEffectsProcess,0)!=WAIT_OBJECT_0)return;
    DWORD code=1;GetExitCodeProcess(audioEffectsProcess,&code);CloseHandle(audioEffectsProcess);audioEffectsProcess=nullptr;
    if(code==0){soundEffectsDirty=false;saveConfig();soundStatus=audioEffects.enabled?L"Efeitos aplicados à saída selecionada.":L"Efeitos do Hext desativados.";}
    else soundStatus=code==4?L"Ative esta saída no Configurator e reinicie o áudio.":L"Não foi possível gravar os efeitos. Verifique o Equalizer APO.";
    refreshSound();invalidate();
}
void soundSliderValue(int id,float t){
    if(!controlEnabled(id))return;
    t=std::clamp(t,0.f,1.f);
    if(id==1030||id==1031)soundResult(sound.volume(id-1030,t));
    else if(id==1032)soundResult(sound.balance(t*2-1));
    else if(id>=1041&&id<=1044){
        int value=id==1041?static_cast<int>(std::lround(t*24))*5:static_cast<int>(std::lround((t*48-24)))*5;
        int* fields[]={&audioEffects.gain,&audioEffects.bass,&audioEffects.mid,&audioEffects.treble};
        *fields[id-1041]=value;soundEffectsDirty=true;soundStatus.clear();invalidate(160);
    }
}
float soundSliderPosition(int id){
    if(id==1030||id==1031)return sound.reading[id-1030].volume;
    if(id==1032)return (sound.reading[0].balance+1)/2;
    if(id==1041)return audioEffects.gain/120.f;
    const int values[]={audioEffects.bass,audioEffects.mid,audioEffects.treble};
    return id>=1042&&id<=1044?(values[id-1042]+120)/240.f:0;
}
bool soundSlider(int id){return (id>=1030&&id<=1032)||(id>=1041&&id<=1044);}
void soundPointer(int id,float x){
    auto item=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.id==id;});
    if(item!=hits.end())soundSliderValue(id,(x-item->rect.left-7)/std::max(1.f,item->rect.right-item->rect.left-14));
}
void soundAction(int id){
    if(id>=1000&&id<=1003){
        if(soundTab!=id-1000){
            soundTab=id-1000;soundTabVisual.target=soundTab;soundTabVisual.frequency=42;soundTabVisual.damping=1;
            soundPageFade.snap(motionReduced()?1:0);soundPageFade.target=1;soundPageFade.frequency=55;
            if(motionReduced())soundTabVisual.snap(soundTab);
        }
        soundChooser=-1;soundStatus.clear();
        refreshSound();if(soundTab==3){bluetooth.refresh();lastBluetoothRefresh=GetTickCount64();}}
    else if(id==1010||id==1011){soundChooser=soundChooser==id-1010?-1:id-1010;soundDeviceOffset=0;refreshSound();}
    else if(id==1012){soundChooser=-1;}
    else if(id>=1200&&id<1712){
        int index=id-1200;
        if(index<static_cast<int>(sound.devices.size())&&soundChooser>=0){
            auto& item=sound.devices[index];if(item.flow==soundChooser){sound.selected[soundChooser]=item.id;sound.read(soundChooser);
                soundChooser=-1;soundStatus.clear();refreshSound();saveConfig();}
        }
    }
    else if(id==1013||id==1014)soundDeviceOffset=std::max(0,soundDeviceOffset+(id==1013?-4:4));
    else if(id==1020||id==1021)soundResult(sound.mute(id-1020));
    else if(id==1022||id==1023){if(!testMode){auto hr=sound.makeDefault(id-1022);soundResult(hr);if(FAILED(hr))soundAdvanced();else connectAudio();}}
    else if(id==1024)soundAdvanced();
    else if(id==1040){audioEffects.enabled=!audioEffects.enabled;soundEffectsDirty=true;soundStatus.clear();}
    else if(id==1045)applySoundEffects();
    else if(id==1046){if(!testMode){
        if(audioApo.installed)ShellExecuteW(windowHandle,L"open",audioApo.selector.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        else if(audioAddon.snapshot().state==hext::AddonState::Ready)audioAddon.install(windowHandle);
        else{
            wchar_t local[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
            if(local[0])audioAddon.start(std::filesystem::path(local)/L"Hext/addons/audio");
        }
    }}
    else if(id==1048){if(!testMode)audioAddon.cancel();}
    else if(id==1047){audioEffects.gain=audioEffects.bass=audioEffects.mid=audioEffects.treble=0;soundEffectsDirty=true;soundStatus.clear();}
    else if(id==1050){if(!testMode)bluetooth.refresh(true);}
    else if(id==1051){if(!testMode)ShellExecuteW(windowHandle,L"open",L"ms-settings:bluetooth",nullptr,nullptr,SW_SHOWNORMAL);}
    else if(id>=1800&&id<2312)bluetoothSelected=bluetoothSelected==id-1800?-1:id-1800;
    else if(id==1055||id==1056){bluetoothOffset=std::max(0,bluetoothOffset+(id==1055?-4:4));bluetoothSelected=-1;}
    else if((id==1052||id==1053||id==1054)&&!testMode&&bluetoothSelected>=0&&bluetoothSelected<static_cast<int>(bluetoothSnapshot.devices.size())){
        auto device=bluetoothSnapshot.devices[bluetoothSelected];
        if(id==1052){
            DWORD result=BluetoothAuthenticateDeviceEx(windowHandle,nullptr,&device,nullptr,MITMProtectionNotRequired);
            soundStatus=result==ERROR_SUCCESS?L"Dispositivo pareado.":result==ERROR_CANCELLED?L"Pareamento cancelado.":L"Conclua a conexão nos ajustes Bluetooth do Windows.";
        }else if(id==1053){
            if(!BluetoothDisplayDeviceProperties(windowHandle,&device))ShellExecuteW(windowHandle,L"open",L"ms-settings:bluetooth",nullptr,nullptr,SW_SHOWNORMAL);
        }else if(MessageBoxW(windowHandle,(L"Remover o pareamento de “"+std::wstring(device.szName)+L"”? ").c_str(),L"Bluetooth",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES){
            soundStatus=BluetoothRemoveDevice(&device.Address)==ERROR_SUCCESS?L"Dispositivo removido.":L"Não foi possível remover o dispositivo.";
        }
        bluetooth.refresh();lastBluetoothRefresh=GetTickCount64();
    }
    invalidate(220);
}
