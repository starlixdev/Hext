#pragma once
std::wstring soundDB(int value){
    wchar_t buffer[32];swprintf(buffer,32,L"%s%.1f dB",value>0?L"+":L"",value/10.0);return buffer;
}
void drawSoundSlider(int id,const std::wstring& label,const std::wstring& value,float y,float position){
    bool enabled=controlEnabled(id);UINT tint=enabled?0xEBEBF0:0x9898A4;
    text(label,204,y,332,13,tint);
    auto valueFont=font(13,false);IDWriteTextLayout* valueLayout=nullptr;
    if(SUCCEEDED(textFactory->CreateTextLayout(value.c_str(),static_cast<UINT32>(value.size()),valueFont,180,24,&valueLayout))){
        valueLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);use(tint);
        canvas->DrawTextLayout(D2D1::Point2F(532,y),valueLayout,brush);valueLayout->Release();
    }
    constexpr float left=211,right=705;
    float x=left+(right-left)*std::clamp(position,0.f,1.f);
    box(left,y+30,right-left,4,0xFFFFFF,2,enabled?.15f:.07f);
    box(left,y+30,std::max(2.f,x-left),4,accent(),2,enabled?.95f:.3f);
    dot(x,y+32,dragSound==id?6:5,0xF4F4F8,enabled?1:.4f);
    hit(id,204,y+19,508,27,label+L", "+value);
    if(keyboardId==id){use(0xFFFFFF,.65f);canvas->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(201,y+17,715,y+48),7,7),brush,1);}
}
void drawSoundDeviceList(){
    text(soundChooser==0?L"Dispositivos de saída":L"Dispositivos de entrada",204,172,350,15,0xE6E6EC,true);
    std::vector<int> filtered;
    for(size_t i=0;i<sound.devices.size()&&i<512;i++)if(sound.devices[i].flow==soundChooser)filtered.push_back(static_cast<int>(i));
    soundDeviceOffset=std::clamp(soundDeviceOffset,0,std::max(0,((static_cast<int>(filtered.size())-1)/4)*4));
    for(int row=0;row<4&&row+soundDeviceOffset<static_cast<int>(filtered.size());row++){
        int index=filtered[row+soundDeviceOffset];auto& item=sound.devices[index];float y=204+row*47.f;
        bool selected=item.id==sound.selected[soundChooser],hovered=hoverId==1200+index||keyboardId==1200+index;
        if(selected||hovered)box(204,y,508,43,selected?accent():0xFFFFFF,8,selected?.13f:.06f);
        symbol(selected?0xE73E:soundChooser==0?0xE767:0xE720,216,y+14,14,selected?accent():0xC7C7D0);
        text(item.name,244,y+5,452,13,0xEAEAF0,true);
        text(item.state!=DEVICE_STATE_ACTIVE?item.state==DEVICE_STATE_DISABLED?L"Desativado":L"Desconectado":item.isDefault?L"Padrão do Windows":L"Disponível",244,y+25,452,11,0xAFAFBC);
        hit(1200+index,204,y,508,43,item.name);
    }
    if(filtered.empty())text(L"Nenhum dispositivo encontrado.",204,224,508,14,0xBDBDC8);
    if(filtered.size()>4){
        if(soundDeviceOffset>0)button(1013,L"Anterior",204,411,100,31);
        if(soundDeviceOffset+4<static_cast<int>(filtered.size()))button(1014,L"Próximos",316,411,100,31);
        alignedText(std::to_wstring((filtered.size()+3)/4)+L"/"+std::to_wstring(soundDeviceOffset/4+1),430,411,80,31,11,0x9A9AA6,false,false);
    }
    button(1012,L"Voltar",610,411,102,31);
}
void drawSoundEndpoint(int flow){
    const auto* item=sound.chosen(flow);const auto& reading=sound.reading[flow];
    text(flow?L"Dispositivo de entrada":L"Dispositivo de saída",204,171,508,12,0xAFAFBC);
    button(1010+flow,L"",204,194,508,38);
    hits.back().name=item?item->name:L"Selecionar dispositivo";
    alignedText(item?item->name:L"Selecionar dispositivo",218,194,448,38,13,0xEBEBF0,false,false);
    symbol(0xE70D,684,208,12,0xBFBFCC);
    drawSoundSlider(1030+flow,flow?L"Volume de entrada":L"Volume de saída",reading.ready?
        std::to_wstring(static_cast<int>(std::lround(reading.volume*100)))+L"%":L"—",251,reading.volume);
    drawToggle(1020+flow,reading.muted,204,319,36,20,reading.ready);
    text(flow?L"Silenciar microfone":L"Silenciar saída",252,322,350,13,0xD5D5DF);
    hit(1020+flow,200,309,252,38,flow?L"Silenciar microfone":L"Silenciar saída");
    if(!flow&&reading.channels==2){
        int balance=static_cast<int>(std::lround(reading.balance*100));
        drawSoundSlider(1032,L"Balanço",balance==0?L"Centralizado":balance<0?L"Esquerda "+std::to_wstring(-balance)+L"%":L"Direita "+std::to_wstring(balance)+L"%",354,(reading.balance+1)/2);
    }else if(!reading.ready)text(item&&item->state==DEVICE_STATE_DISABLED?L"Ative este dispositivo nos ajustes do Windows.":L"Conecte ou selecione um dispositivo disponível.",204,365,508,12,0xBDBDC8);
    if(item&&item->isDefault)text(L"Dispositivo padrão",204,422,250,12,0xB9B9C5);
    else button(1022+flow,L"Definir como padrão",204,411,206,31);
    button(1024,L"Mais ajustes",554,411,158,31);
}
void drawSoundEffects(){
    text(L"Amplificação e equalização",252,173,460,14,0xE7E7EE,true);
    if(!audioApo.installed||!apoEndpointReady){
        auto addon=audioAddon.snapshot();
        bool downloading=addon.state==hext::AddonState::Downloading;
        symbol(0xE767,432,225,42,0xBDBDC9);
        alignedText(!audioApo.installed?L"Add-on de áudio":L"Ativar efeitos nesta saída",204,282,508,29,19,0xF2F2F6,true);
        alignedText(audioApo.installed?L"Selecione a saída no configurador para ativar os efeitos.":L"Equalização e amplificação com Equalizer APO XT.",204,320,508,24,12,0xBDBDC8);
        alignedText(addon.message.empty()?L"Opcional · download pelo GitHub":addon.message,204,346,508,24,11,0xBDBDC8);
        if(downloading){
            float progress=static_cast<float>(addon.received)/std::max(1ULL,addon.total);
            box(284,381,348,4,0xFFFFFF,2,.12f);box(284,381,std::max(2.f,348*progress),4,accent(),2);
            button(1048,L"Cancelar",334,404,248,34);
            alignedText(std::to_wstring(static_cast<int>(progress*100))+L"%",640,370,60,24,11,0xBDBDC8);
        }else button(1046,audioApo.installed?L"Configurar saída":addon.state==hext::AddonState::Ready?L"Instalar add-on":
            addon.state==hext::AddonState::Installing?L"Instalando…":L"Baixar add-on",334,404,248,34,true);
        return;
    }
    drawToggle(1040,audioEffects.enabled,204,170,36,20,controlEnabled(1040));
    hit(1040,200,162,280,35,L"Ativar efeitos de áudio");
    drawSoundSlider(1041,L"Amplificação",soundDB(audioEffects.gain),205,soundSliderPosition(1041));
    drawSoundSlider(1042,L"Graves",soundDB(audioEffects.bass),257,soundSliderPosition(1042));
    drawSoundSlider(1043,L"Médios",soundDB(audioEffects.mid),309,soundSliderPosition(1043));
    drawSoundSlider(1044,L"Agudos",soundDB(audioEffects.treble),361,soundSliderPosition(1044));
    button(1047,L"Redefinir",204,414,110,30);
    button(1046,L"Configurar",326,414,110,30);
    button(1045,audioEffectsProcess?L"Aplicando…":L"Aplicar",590,414,122,30,true);
}
void drawSoundBluetooth(){
    text(L"Dispositivos Bluetooth",204,174,280,14,0xE7E7EE,true);
    button(1050,bluetooth.refreshing()?L"Procurando…":L"Procurar",584,166,128,30);
    int count=static_cast<int>(bluetoothSnapshot.devices.size());
    bluetoothOffset=std::clamp(bluetoothOffset,0,std::max(0,((count-1)/4)*4));
    if(count>4){
        alignedText(std::to_wstring((count+3)/4)+L"/"+std::to_wstring(bluetoothOffset/4+1),464,168,40,28,10,0x9A9AA6,false,false);
        if(bluetoothOffset>0)iconButton(1055,0xE76B,L"Dispositivos anteriores",508,168,28,28);
        if(bluetoothOffset+4<count)iconButton(1056,0xE76C,L"Próximos dispositivos",544,168,28,28);
    }
    if(count==0){
        symbol(0xE702,444,250,32,0xBDBDC8);
        alignedText(bluetooth.refreshing()?L"Buscando dispositivos…":bluetoothSnapshot.radio?L"Nenhum dispositivo encontrado":L"Verifique o Bluetooth do computador",204,302,508,26,15,0xD8D8E2);
        alignedText(L"Conexões e dispositivos LE também estão nos ajustes do Windows.",204,338,508,24,12,0xBDBDC8);
    }
    for(int row=0;row<4&&row+bluetoothOffset<count;row++){
        int index=row+bluetoothOffset;const auto& device=bluetoothSnapshot.devices[index];float y=208+row*46.f;
        bool selected=bluetoothSelected==index;
        if(selected||hoverId==1800+index||keyboardId==1800+index)box(204,y,508,42,selected?accent():0xFFFFFF,8,selected?.13f:.065f);
        symbol(0xE702,216,y+13,15,selected?accent():0xC9C9D4);
        text(device.szName[0]?device.szName:L"Dispositivo Bluetooth",244,y+4,452,13,0xE9E9F0,true);
        text(device.fConnected?L"Conectado":device.fAuthenticated||device.fRemembered?L"Pareado":L"Disponível para parear",244,y+24,452,11,0xB6B6C3);
        hit(1800+index,204,y,508,42,device.szName[0]?device.szName:L"Dispositivo Bluetooth");
    }
    if(bluetoothSelected>=0&&bluetoothSelected<count){
        auto& device=bluetoothSnapshot.devices[bluetoothSelected];bool paired=device.fAuthenticated||device.fRemembered;
        button(paired?1053:1052,paired?L"Opções de conexão":L"Parear",204,405,186,32);
        if(paired)button(1054,L"Esquecer",402,405,112,32);
    }
    button(1051,L"Gerenciar",580,405,132,32);
}
void drawSound(){
    text(L"Som",204,87,508,21,0xF5F5F8,true);
    const wchar_t* tabs[]={L"Saída",L"Entrada",L"Efeitos",L"Bluetooth"};
    box(204,128,508,32,0xFFFFFF,9,.045f);
    box(206+static_cast<float>(soundTabVisual.value)*126.5f,130,124.5f,28,0xFFFFFF,7,.12f);
    for(int i=0;i<4;i++){
        float x=206+i*126.5f;
        if(hoverId==1000+i||keyboardId==1000+i)box(x,130,124.5f,28,0xFFFFFF,7,.045f);
        alignedText(tabs[i],x,130,124.5f,28,12,i==soundTab?0xF3F3F8:0xBFBFCB,true);
        hit(1000+i,x,130,124.5f,28,tabs[i]);
        if(keyboardId==1000+i){use(0xFFFFFF,.7f);canvas->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(x,130,x+124.5f,158),7,7),brush,1);}
    }
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,163,722,470),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),unit(soundPageFade.value)),nullptr);
    if(soundChooser>=0)drawSoundDeviceList();
    else if(soundTab<2)drawSoundEndpoint(soundTab);
    else if(soundTab==2)drawSoundEffects();
    else drawSoundBluetooth();
    if(!soundStatus.empty())text(soundStatus,204,447,508,10,0xD3D3DD);
    else if(soundTab==2&&apoEndpointReady&&audioEffects.gain>0)text(L"Ganhos positivos podem distorcer o áudio.",204,449,508,10,0xBDBDC8);
    canvas->PopLayer();
}
