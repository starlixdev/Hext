#pragma once
void refreshSound(){
    sound.refresh();
}
void soundResult(HRESULT result){
    soundStatus=FAILED(result)?L"Não foi possível ajustar este dispositivo. Abra os ajustes do Windows.":L"";
    invalidate(180);
}
void soundAdvanced(){
    if(testMode)return;
    ShellExecuteW(windowHandle,L"open",L"control.exe",soundTab==1?L"mmsys.cpl,,1":L"mmsys.cpl,,0",nullptr,SW_SHOWNORMAL);
}
void soundSliderValue(int id,float t){
    if(!controlEnabled(id))return;
    t=std::clamp(t,0.f,1.f);
    if(id==1030||id==1031)soundResult(sound.volume(id-1030,t));
    else if(id==1032)soundResult(sound.balance(t*2-1));
}
float soundSliderPosition(int id){
    if(id==1030||id==1031)return sound.reading[id-1030].volume;
    if(id==1032)return (sound.reading[0].balance+1)/2;
    return 0;
}
bool soundSlider(int id){return id>=1030&&id<=1032;}
void soundPointer(int id,float x){
    auto item=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.id==id;});
    if(item!=hits.end())soundSliderValue(id,(x-item->rect.left-7)/std::max(1.f,item->rect.right-item->rect.left-14));
}
void soundAction(int id){
    if(id>=1000&&id<=1002){
        if(soundTab!=id-1000){
            soundTab=id-1000;soundTabVisual.target=soundTab;soundTabVisual.frequency=42;soundTabVisual.damping=1;
            soundPageFade.snap(motionReduced()?1:0);soundPageFade.target=1;soundPageFade.frequency=55;
            if(motionReduced())soundTabVisual.snap(soundTab);
        }
        soundChooser=-1;soundStatus.clear();
        refreshSound();if(soundTab==2){bluetooth.refresh();lastBluetoothRefresh=GetTickCount64();}}
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
