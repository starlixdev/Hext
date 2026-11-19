#pragma once

const wchar_t* styleName(){return motionStyle==0?L"Smooth":motionStyle==1?L"Snappy":L"Bouncy";}
D2D1_RECT_F volumeTrack=D2D1::RectF(222,238,690,246);
D2D1_RECT_F popoverRect{};

void textScaled(const std::wstring& value,float x,float y,float size,UINT tint,
                float maxWidth=500,bool bold=false,float opacity=1){
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    float s=size/20;
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(s,s,D2D1::Point2F(x,y))*original);
    text(value,x,y,maxWidth/s,20,tint,bold,opacity);
    canvas->SetTransform(original);
}
void number(int id,const std::wstring& value,float x,float y,float size,UINT tint,
            float maxWidth=300,float opacity=1){
    auto& n=numberMotion[id];
    if(n.value.empty()){n.value=n.oldValue=value;n.progress.snap(1);}
    if(n.value!=value){
        n.oldValue=n.value;n.value=value;n.progress.snap(motionReduced()?1:0);
        n.progress.target=1;n.progress.frequency=32;invalidate(220);
    }
    float progress=unit(n.progress.value);
    // Only changed digits move; stable digits retain position and identity.
    float cursor=x;
    for(size_t i=0;i<value.size();i++){
        wchar_t c=value[i];
        float advance=size*(c==L':'?.27f:c==L'.'?.26f:c==L'%'?.78f:.58f);
        if(cursor+advance>x+maxWidth)break;
        bool changed=i<n.oldValue.size()&&n.oldValue[i]!=c&&progress<.999f;
        if(changed){
            textScaled(std::wstring(1,n.oldValue[i]),cursor,y-progress*size*.16f,size,tint,advance+3,false,(1-progress)*opacity);
            textScaled(std::wstring(1,c),cursor,y+(1-progress)*size*.16f,size,tint,advance+3,false,progress*opacity);
        }else textScaled(std::wstring(1,c),cursor,y,size,tint,advance+3,false,opacity);
        cursor+=advance;
    }
}
void contentText(int id,const std::wstring& value,float x,float y,float maxWidth,int size,UINT tint,bool bold){
    if(page==5){text(value,x,y,maxWidth,size,tint,bold);return;}
    auto& n=numberMotion[10000+id];
    if(n.value.empty()){n.value=n.oldValue=value;n.progress.snap(1);}
    if(n.value!=value){
        n.oldValue=n.value;n.value=value;n.progress.snap(motionReduced()?1:0);
        n.progress.target=1;n.progress.frequency=36;invalidate(180);
    }
    float p=unit(n.progress.value);
    if(p<.999f)text(n.oldValue,x,y-2*p,maxWidth,size,tint,bold,1-p);
    text(value,x,y+2*(1-p),maxWidth,size,tint,bold,p);
}
void symbol(wchar_t glyph,float x,float y,float size,UINT tint,float alpha){
    int key=static_cast<int>(std::ceil(size));
    if(!iconFormats.count(key)){
        IDWriteTextFormat* format=nullptr;
        textFactory->CreateTextFormat(iconFont.c_str(),nullptr,DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,static_cast<float>(key),L"pt-BR",&format);
        if(format){format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);}
        iconFormats[key]=format;
    }
    if(auto format=iconFormats[key]){
        IDWriteTextLayout* layout=nullptr;
        if(SUCCEEDED(textFactory->CreateTextLayout(&glyph,1,format,size,size,&layout))){
            DWRITE_OVERHANG_METRICS ink{};layout->GetOverhangMetrics(&ink);
            float dx=(ink.left-ink.right)/2,dy=(ink.top-ink.bottom)/2;
            use(tint,alpha);canvas->DrawTextLayout(D2D1::Point2F(x+dx,y+dy),layout,brush);
            layout->Release();
        }
    }
}
void iconButton(int id,wchar_t glyph,const std::wstring& name,float x,float y,float w,float h,bool primary=false){
    auto& m=controlMotion[id];
    m.hover.target=hoverId==id||keyboardId==id?1:0;
    m.press.target=pressedId==id?.92:1;m.press.frequency=36;m.press.damping=.78;m.hover.frequency=35;
    if(motionReduced()){m.hover.snap(m.hover.target);m.press.snap(m.press.target);}
    float factor=static_cast<float>(m.press.value);
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(factor,factor,D2D1::Point2F(x+w/2,y+h/2))*original);
    if(primary)box(x,y,w,h,0xF4F4F7,h/2);
    else box(x,y,w,h,0xFFFFFF,h/2,.02f+.09f*unit(m.hover.value));
    if(keyboardId==id){use(accent());canvas->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x-2,y-2,x+w+2,y+h+2),h/2,h/2),brush,1.5f);}
    float iconSize=primary?21:17;
    if(glyph)symbol(glyph,x+(w-iconSize)/2,y+(h-iconSize)/2,iconSize,primary?0x18181C:0xE9E9EE);
    canvas->SetTransform(original);hit(id,x,y,w,h,name);
}
void drawSendButton(){
    bool busy=personalBusy();
    if(sendState.target!=(busy?1:0)){sendState.target=busy?1:0;sendState.frequency=45;invalidate(180);}
    if(motionReduced())sendState.snap(sendState.target);
    iconButton(451,0,busy?L"Interromper resposta":L"Enviar mensagem",668,372,38,38,true);
    float p=unit(sendState.value),cx=687,cy=391;
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    float pressed=static_cast<float>(controlMotion[451].press.value);
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(pressed,pressed,D2D1::Point2F(cx,cy))*original);
    ID2D1StrokeStyle* stroke=nullptr;
    factory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,D2D1_CAP_STYLE_ROUND,
        D2D1_CAP_STYLE_ROUND,D2D1_LINE_JOIN_ROUND),nullptr,0,&stroke);
    use(0x18181C,1-p);
    canvas->DrawLine(D2D1::Point2F(cx,cy+6.5f),D2D1::Point2F(cx,cy-6.5f),brush,2.2f,stroke);
    canvas->DrawLine(D2D1::Point2F(cx-5.5f,cy-1),D2D1::Point2F(cx,cy-6.5f),brush,2.2f,stroke);
    canvas->DrawLine(D2D1::Point2F(cx,cy-6.5f),D2D1::Point2F(cx+5.5f,cy-1),brush,2.2f,stroke);
    box(cx-5.5f,cy-5.5f,11,11,0x18181C,2,p);
    release(stroke);canvas->SetTransform(original);
}
void metric(float x,float y,float w,const std::wstring& title,float value,UINT tint,int id,Spring& visual){
    text(title,x,y,w,13,0xBEBEC5);
    number(id,std::to_wstring(static_cast<int>(value))+L"%",x,y+27,32,0xF3F3F7,w);
    box(x,y+81,w,4,0xFFFFFF,2,.1f);
    box(x,y+81,std::max(2.0f,w*unit(visual.value/100)),4,tint,2);
}
void volumeSlider(float x,float y,float w){
    symbol(audioMuted?0xE74F:0xE767,x,y+24,15,0xCDCDD4);
    text(audioReady?L"Volume":L"Áudio indisponível",x,y,w-55,12,0xBEBEC6);
    number(503,audioMuted?L"0%":std::to_wstring(static_cast<int>(volume*100))+L"%",x+w-45,y,12,0xDADAE0,45);
    float start=x+25,trackWidth=w-25;
    box(start,y+31,trackWidth,4,0xFFFFFF,2,.14f);
    float level=unit(volumeVisual.value);
    box(start,y+31,std::max(2.0f,trackWidth*level),4,0xE7E7EE,2);
    dot(start+trackWidth*level,y+33,dragVolume?6:5,0xFFFFFF);
    volumeTrack=D2D1::RectF(start,y+21,start+trackWidth,y+46);
    hit(200,start-6,y+18,trackWidth+12,30,L"Volume do Windows");
}
IDWriteTextLayout* assistantInputLayout(){
    IDWriteTextLayout* layout=nullptr;
    textFactory->CreateTextLayout(assistantInput.c_str(),static_cast<UINT32>(assistantInput.size()),
        font(14,false),100000,30,&layout);
    if(layout){
        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        DWRITE_TRIMMING trimming{};layout->SetTrimming(&trimming,nullptr);
    }
    return layout;
}
void positionAssistantCaret(float pointerX){
    auto item=std::find_if(hits.begin(),hits.end(),[](const Hit& h){return h.id==450;});
    if(item==hits.end())return;
    float baseX=204+(pointerX-item->rect.left)*450/std::max(1.0f,item->rect.right-item->rect.left);
    auto layout=assistantInputLayout();
    if(layout){
        BOOL trailing=FALSE,inside=FALSE;DWRITE_HIT_TEST_METRICS metrics{};
        if(SUCCEEDED(layout->HitTestPoint(baseX-218+assistantTextScroll,8,&trailing,&inside,&metrics)))
            assistantCaret=std::min(assistantInput.size(),static_cast<size_t>(metrics.textPosition+(trailing?metrics.length:0)));
        layout->Release();
    }
    resetAssistantCaret();invalidate(120);
}
void drawAssistantInput(){
    box(204,369,450,44,0xFFFFFF,12,.075f);
    if(assistantFocused){use(0xDADAE4);canvas->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(204,369,654,413),12,12),brush,1);}
    auto layout=assistantInputLayout();
    float caretX=0,caretY=0;DWRITE_HIT_TEST_METRICS caretMetrics{};DWRITE_TEXT_METRICS metrics{};
    if(layout){
        layout->GetMetrics(&metrics);
        layout->HitTestTextPosition(static_cast<UINT32>(std::min(assistantCaret,assistantInput.size())),FALSE,
            &caretX,&caretY,&caretMetrics);
    }
    if(assistantInput.empty())assistantTextScroll=0;
    if(assistantFocused){
        if(caretX<assistantTextScroll)assistantTextScroll=caretX;
        if(caretX-assistantTextScroll>420)assistantTextScroll=caretX-420;
    }
    canvas->PushAxisAlignedClip(D2D1::RectF(217,378,641,405),D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if(selectAll&&assistantFocused&&!assistantInput.empty())
        box(218-assistantTextScroll,379,metrics.widthIncludingTrailingWhitespace,23,0x586B98,2,.7f);
    if(assistantInput.empty())text(personal.view==PersonalView::Editor?L"Digite aqui…":L"Mensagem…",218,381,422,14,0xB7B7C1);
    else if(layout){use(0xEEEEF4);canvas->DrawTextLayout(D2D1::Point2F(218-assistantTextScroll,381),layout,brush);}
    lastAssistantCaretX=218+caretX-assistantTextScroll;
    if(assistantFocused&&assistantCaretVisible&&!selectAll)
        line(lastAssistantCaretX,381,lastAssistantCaretX,399,0xF4F4F8,1.25f);
    canvas->PopAxisAlignedClip();release(layout);
    hit(450,204,369,450,44,L"Mensagem para o assistente");
}
void settingsChoice(int,const std::wstring&,const std::wstring&,float);
void drawToggle(int,bool,float,float,float,float,bool);
#include "personal-ui.h"
void drawHome(){
    if(personal.view!=PersonalView::None){drawPersonalOverlay();return;}
    text(L"Assistant",204,87,380,21,0xF5F5F8,true);
    iconButton(3420,0xE823,L"Tarefas pendentes",550,87,30,30);
    iconButton(3070,0xE81C,L"Conversas salvas",592,87,30,30);
    if(!personalBusy())iconButton(452,0xE710,L"Nova conversa",634,87,30,30);
    iconButton(461,0xE713,L"Configurar assistente",676,87,30,30);
    if(conversation.empty()){
        symbol(0xE8F2,442,181,32,0xC9C9D2);
        alignedText(L"Como posso ajudar?",204,236,508,30,20,0xF0F0F5);
        if(!assistantConfigured())button(460,L"Conectar",388,281,140,32);
    }else{
        std::vector<IDWriteTextLayout*> layouts;std::vector<float> heights;
        float total=0;
        for(auto& message:conversation){
            if(message.modelOnly){layouts.push_back(nullptr);heights.push_back(0);continue;}
            IDWriteTextLayout* layout=nullptr;DWRITE_TEXT_METRICS metrics{};
            textFactory->CreateTextLayout(message.text.c_str(),static_cast<UINT32>(message.text.size()),font(14,false),476,100000,&layout);
            if(layout){layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);layout->GetMetrics(&metrics);}
            float h=metrics.height+44;total+=h;layouts.push_back(layout);heights.push_back(h);
        }
        assistantScrollMax=std::max(0.0f,total-216);assistantScroll=std::clamp(assistantScroll,0.0f,assistantScrollMax);
        float y=133-std::max(0.0f,total-216)+assistantScroll;
        canvas->PushAxisAlignedClip(D2D1::RectF(204,129,712,351),D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        for(size_t i=0;i<conversation.size();i++){
            if(conversation[i].modelOnly)continue;
            text(conversation[i].user?L"Você":conversation[i].tool?L"Hext":L"Assistant",212,y,476,11,0xB7B7C1,true);
            if(layouts[i]){use(0xEEEEF4);canvas->DrawTextLayout(D2D1::Point2F(212,y+21),layouts[i],brush);}
            y+=heights[i];release(layouts[i]);
        }
        canvas->PopAxisAlignedClip();
    }
    drawAssistantInput();
    drawSendButton();
    text(assistantStatus,212,433,personal.task.active()?personal.task.stage==hext::TaskStage::Setup?292:390:492,12,0xB7B7C1);
    if(personal.task.active()&&personal.task.stage==hext::TaskStage::Setup)button(3315,L"Continuar configuração",516,429,196,30,true);
    else if(personal.task.active()){button(3423,L"Pausar",624,429,88,30);}
}
void drawTimer(){
    text(L"Temporizador",204,87,500,21,0xF5F5F8,true);
    contentText(810,timerRunning?L"Em andamento":L"Pausado",401,241,220,13,0xBDBDC5,false);
    float ratio=timerDuration?std::clamp(static_cast<float>(remaining())/timerDuration,0.0f,1.0f):0;
    box(230,286,456,4,0xFFFFFF,2,.12f);box(230,286,std::max(2.0f,456*ratio),4,accent(),2);
    button(221,L"1 minuto",222,315,148,32,false,timerDuration==60000);
    button(222,L"5 minutos",384,315,148,32,false,timerDuration==300000);
    button(223,L"25 minutos",546,315,148,32,false,timerDuration==1500000);
    button(224,L"Reiniciar",222,379,148,34);
    text(L"A contagem continua com o painel recolhido.",204,434,508,12,0xB7B7C1);
}
bool metricShown(int metric){return (metricMask&(1u<<metric))!=0;}
hext::Reading sensorReading(int metric){
    if(metric==hext::CpuLoad)return {cpuPercent,cpuSampleReady,L"Windows"};
    if(!hardwareFresh())return {};
    switch(metric){
        case hext::CpuTemperature:return hardwareSnapshot.cpuTemperature;
        case hext::CpuPower:return hardwareSnapshot.cpuPower;
        case hext::GpuLoad:return hardwareSnapshot.gpuLoad;
        case hext::GpuTemperature:return hardwareSnapshot.gpuTemperature;
        case hext::GpuPower:return hardwareSnapshot.gpuPower;
        default:return {};
    }
}
void systemGroup(float x,float y,float w,float h,bool gpu){
    box(x,y,w,h,0xFFFFFF,14,.035f);
    text(gpu?L"GPU":L"Processador",x+16,y+16,w-32,15,0xEEEEF4,true);
    int base=gpu?hext::GpuLoad:hext::CpuLoad,count=0;
    for(int i=0;i<3;i++)if(metricShown(base+i))count++;
    float step=(h-62)/std::max(1,count),row=y+48;
    for(int i=0;i<3;i++){
        int id=base+i;if(!metricShown(id))continue;
        auto reading=sensorReading(id);
        const wchar_t* label=i==0?L"Uso":i==1?L"Temperatura":L"Consumo";
        std::wstring value=reading.available?hext::decimal(reading.value,i==2?1:0)+(i==0?L"%":i==1?L" °C":L" W"):L"—";
        alignedText(label,x+16,row,w-142,36,12,0xBEBEC8,false,false);
        alignedText(value,x+w-122,row,106,36,20,reading.available?0xF0F0F5:0x9898A4,false);
        if(i==0){
            box(x+16,row+39,w-32,3,0xFFFFFF,1.5f,.1f);
            if(reading.available)box(x+16,row+39,std::max(2.0f,(w-32)*unit(reading.value/100)),3,
                gpu?0x8FACC9:accent(),1.5f);
        }
        hit(520+id,x+12,row,w-24,44,std::wstring(gpu?L"GPU · ":L"CPU · ")+label+L": "+
            (reading.available?value+L" · "+reading.source:L"sensor indisponível"));
        row+=step;
    }
}
void drawSystem(){
    text(L"Meu PC",204,87,420,21,0xF5F5F8,true);
    iconButton(470,0xE713,L"Personalizar monitoramento",676,87,30,30);
    bool ram=metricShown(hext::Ram),cpu=(metricMask&7)!=0,gpu=(metricMask&56)!=0;
    if(ram){
        box(204,130,508,56,0xFFFFFF,13,.045f);
        std::wstring memory=ramSampleReady?hext::decimal(ramTotalGB)+L"/"+hext::decimal(ramFreeGB)+L" GB":L"—";
        alignedText(L"Memória RAM · "+memory,220,139,416,25,14,0xE6E6ED,true,false);
        alignedText(ramSampleReady?hext::decimal(ramPercent,0)+L"%":L"—",646,139,48,25,14,0xBEBEC8,false);
        box(220,174,476,3,0xFFFFFF,1.5f,.1f);
        if(ramSampleReady)box(220,174,std::max(2.0f,476*unit(ramPercent/100)),3,0x8FACC9,1.5f);
        hit(526,216,137,480,32,L"Memória RAM: "+memory+L", total/disponível");
    }
    float top=ram?202:130,h=440-top;
    if(cpu)systemGroup(204,top,gpu?246:508,h,false);
    if(gpu)systemGroup(cpu?466:204,top,cpu?246:508,h,true);
    if(!ram&&!cpu&&!gpu){
        alignedText(L"Escolha os indicadores",204,220,508,30,19,0xEEEEF4);
        button(471,L"Personalizar",378,280,160,32);
    }
}
void drawFiles(){
    text(L"Arquivos",204,87,500,21,0xF5F5F8,true);
    text(L"Até 5 atalhos. Clique no nome para abrir.",204,129,508,13,0xB7B7C1);
    if(files.empty()){
        symbol(0xE8B7,204,196,31,0x9D9DA8);
        text(L"Nenhum arquivo adicionado",204,251,508,16,0xEBEBF0,true);
        text(L"Arraste um arquivo para este painel.",204,287,508,13,0xB7B7C1);
        text(L"Os originais permanecem em suas pastas.",204,321,508,13,0xB7B7C1);
    }else{
        int index=0;
        for(auto& f:files){
            auto p=f.find_last_of(L"\\/");
            button(400+index,f.substr(p==std::wstring::npos?0:p+1),204,176+index*43.0f,508,36);
            if(++index==5)break;
        }
        button(410,L"Limpar atalhos",204,412,160,32);
        text(L"Mantém os arquivos originais.",384,421,328,12,0xB7B7C1);
    }
}
void settingsChoice(int id,const std::wstring& label,const std::wstring& value,float y){
    button(id,value,498,y,198,32);
    // Accessible name includes the setting, rather than only its current value.
    hits.back().name=label+L": "+value;
    symbol(0xE70D,674,y+10,12,controlEnabled(id)?0xD2D2DA:0x9999A4);
}
UINT mixedColor(UINT a,UINT b,float p){
    UINT result=0;
    for(int shift:{0,8,16})result|=static_cast<UINT>(std::lround(blend((a>>shift)&255,(b>>shift)&255,p)))<<shift;
    return result;
}
void drawToggle(int id,bool checked,float x,float y,float w,float h,bool enabled=true){
    auto& state=toggleMotion[id];
    if(!state.initialized){state.position.snap(checked?1:0);state.initialized=true;}
    if(state.position.target!=(checked?1:0)){
        state.position.target=checked?1:0;state.position.frequency=38;state.position.damping=1;invalidate(220);
    }
    if(motionReduced())state.position.snap(state.position.target);
    float p=unit(state.position.value),opacity=enabled?1:.55f;
    box(x,y,w,h,mixedColor(0x62626C,accent(),p),h/2,opacity);
    dot(x+h/2+(w-h)*p,y+h/2,(h-5)/2,0xFFFFFF,opacity);
    if(keyboardId==id){
        use(0xFFFFFF);canvas->DrawRoundedRectangle(D2D1::RoundedRect(
            D2D1::RectF(x-3,y-3,x+w+3,y+h+3),(h+6)/2,(h+6)/2),brush,1);
    }
}
void metricToggle(int metric,const std::wstring& label,float x,float y,float w){
    int id=380+metric;bool checked=metricShown(metric);
    alignedText(label,x+48,y,w-48,32,13,0xE6E6ED,false,false);
    float tx=x;
    drawToggle(id,checked,tx,y+6,36,20);
    hit(id,x,y,w,32,(metric<3?L"Processador · ":metric<6?L"GPU · ":L"")+label);
}
void drawSettingsSection(int section){
    if(section==5){
        text(L"Geral",204,130,508,14,0xC2C2CB,true);
        const wchar_t* titles[]={L"Mostrar ou ocultar a ilha",L"Expandir ou recolher"};
        const wchar_t* details[]={L"Oculta completamente e mostra novamente.",L"Abre o painel também sobre outros aplicativos."};
        for(int slot=0;slot<2;slot++){
            float y=160+slot*132.f;
            box(204,y,508,120,0xFFFFFF,13,.045f);
            text(titles[slot],220,y+14,466,14,0xF0F0F5,true);
            text(details[slot],220,y+37,466,12,0xB7B7C1);
            button(620+slot,captureShortcut==slot?L"Pressione a combinação…":shortcuts.bindings[slot].label(),220,y+65,310,34,false,captureShortcut==slot);
            hits.back().name=L"Atalho: "+std::wstring(titles[slot])+L", "+shortcuts.bindings[slot].label();
            button(622+slot,L"Restaurar padrão",542,y+65,154,34);
            hits.back().name=L"Restaurar padrão: "+std::wstring(titles[slot]);
            auto status=shortcutStatus[slot];
            if(status.empty()&&!shortcuts.registeredIDs[slot]&&!testMode)status=L"Atalho indisponível. Escolha outra combinação.";
            if(!status.empty())text(status,220,y+104,476,10,0xCDCDD8);
        }
        if(captureShortcut>=0){
            text(L"Use Ctrl ou Alt com uma tecla. Esc cancela.",220,433,340,12,0xB7B7C1);
            button(624,L"Cancelar",590,427,106,28);
        }else text(L"Clique no atalho para editar. A disponibilidade é verificada ao salvar.",220,435,476,11,0xB7B7C1);
    }else if(section==0){
        text(L"Aparência",204,130,508,14,0xC2C2CB,true);
        box(204,160,508,202,0xFFFFFF,13,.045f);
        text(L"Material",220,176,258,14,0xF0F0F5,true);
        text(L"Fundo da ilha e dos painéis",220,198,258,12,0xB7B7C1);
        settingsChoice(317,L"Material",systemTransparency&&!highContrast?
            (translucent?L"Translúcido":L"Sólido"):L"Sólido · Windows",178);
        line(220,226,696,226,0xFFFFFF,1,.07f);
        text(L"Formato",220,242,258,14,0xF0F0F5,true);
        text(L"Encaixado no topo ou com margem",220,264,258,12,0xB7B7C1);
        settingsChoice(310,L"Formato",floating?L"Flutuante":L"Encaixado",244);
        line(220,292,696,292,0xFFFFFF,1,.07f);
        text(L"Escala da interface",220,308,258,14,0xF0F0F5,true);
        text(L"Tamanho dos textos e controles",220,330,258,12,0xB7B7C1);
        settingsChoice(314,L"Escala",std::to_wstring(static_cast<int>(zoom*100))+L"%",310);
        box(204,376,508,70,0xFFFFFF,13,.045f);
        text(L"Cor de destaque",220,389,202,14,0xF0F0F5,true);
        text(L"Seleção e ações principais",220,412,202,12,0xB7B7C1);
        for(int i=0;i<4;i++){
            float x=478+i*40.0f;
            auto color=accentPalette.recent[i];
            dot(x,411,12,color);
            if(accent()==color){
                use(0xF6F6FA);canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x,411),17,17),brush,1.5f);
                symbol(0xE73E,x-6,405,12,hext::foreground(color));
            }
            if(keyboardId==300+i){use(accent());canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x,411),20,20),brush,1);}
            hit(300+i,x-19,391,38,40,L"Cor de destaque: "+hext::hex(color));
        }
        iconButton(330,0xE790,L"Escolher outra cor",646,393,36,36);
    }else if(section==1){
        text(L"Movimento",204,130,508,14,0xC2C2CB,true);
        box(204,160,508,116,0xFFFFFF,13,.045f);
        text(L"Animação da ilha",220,176,476,14,0xF0F0F5,true);
        text(L"Suave, ágil ou com mais elasticidade",220,199,476,12,0xB7B7C1);
        const std::wstring presets[]={L"Smooth",L"Snappy",L"Bouncy"};
        for(int i=0;i<3;i++)button(320+i,presets[i],220+i*160.0f,231,156,30,false,motionStyle==i);
        box(204,292,508,88,0xFFFFFF,13,.045f);
        text(L"Reduzir movimento",268,309,420,14,0xF0F0F5,true);
        text(systemReducedMotion?L"Ativado nas preferências do Windows":
            L"Trocas imediatas, sem expansão ou elasticidade",268,334,420,12,0xB7B7C1);
        bool enabled=motionReduced();
        auto& m=controlMotion[311];m.hover.target=hoverId==311||keyboardId==311?1:0;
        if(motionReduced())m.hover.snap(m.hover.target);
        drawToggle(311,enabled,220,316,36,20,controlEnabled(311));
        hit(311,216,307,480,44,L"Reduzir movimento");
        text(L"A entrada no desktop usa um fade discreto.",220,403,476,12,0xB7B7C1);
        text(L"A preferência de acessibilidade do Windows é respeitada.",220,426,476,12,0xB7B7C1);
    }else if(section==2){
        text(L"Tela e áudio",204,130,508,14,0xC2C2CB,true);
        box(204,160,508,112,0xFFFFFF,13,.045f);
        text(L"Monitor",220,176,476,14,0xF0F0F5,true);
        text(L"Selecione para alternar entre as telas conectadas",220,199,476,12,0xB7B7C1);
        auto resolution=std::to_wstring(monitorRect.right-monitorRect.left)+L" × "+
                        std::to_wstring(monitorRect.bottom-monitorRect.top);
        button(313,L"Alternar tela · "+resolution,220,230,476,30);
        symbol(0xE8AB,674,239,12,0xD2D2DA);
        box(204,288,508,110,0xFFFFFF,13,.045f);
        text(L"Saída de áudio",220,305,276,14,0xF0F0F5,true);
        text(audioReady?L"Dispositivo padrão conectado":L"Nenhuma saída disponível",220,330,276,12,0xB7B7C1);
        button(315,L"Reconectar",546,306,150,32);
        text(L"Atualiza a conexão com o dispositivo padrão do Windows.",220,369,476,12,0xB7B7C1);
        text(L"As preferências são salvas automaticamente.",220,425,476,12,0xB7B7C1);
    }else if(section==3){
        drawPersonalSettings();
    }else{
        text(L"Monitoramento",204,130,508,14,0xC2C2CB,true);
        box(204,160,246,172,0xFFFFFF,13,.045f);
        box(466,160,246,172,0xFFFFFF,13,.045f);
        text(L"Processador",220,177,214,14,0xF0F0F5,true);
        text(L"GPU",482,177,214,14,0xF0F0F5,true);
        const std::wstring names[]={L"Uso",L"Temperatura",L"Consumo"};
        for(int i=0;i<3;i++){
            metricToggle(hext::CpuLoad+i,names[i],220,207+i*40.0f,214);
            metricToggle(hext::GpuLoad+i,names[i],482,207+i*40.0f,214);
        }
        box(204,344,508,48,0xFFFFFF,13,.045f);
        metricToggle(hext::Ram,L"Memória RAM",220,352,476);
        button(360,L"Sensores",204,408,160,32);
        alignedText(L"Intervalo",392,408,100,32,13,0xB7B7C1,false,false);
        settingsChoice(362,L"Intervalo",hardwareInterval==1000?L"1 segundo":hardwareInterval==2000?L"2 segundos":L"5 segundos",408);
    }
}
void drawSettings(){
    if(settingsSection==3&&personal.view!=PersonalView::None){drawPersonalOverlay();return;}
    text(L"Configurações",204,87,500,21,0xF5F5F8,true);
    float p=blend(.72f,1,unit(settingsFade.value));
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,125,722,468),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),p),nullptr);
    drawSettingsSection(settingsSection);canvas->PopLayer();
}
void drawAssistant(){
    text(L"Comandos locais",204,87,500,21,0xF5F5F8,true);
    text(L"timer 5  ·  volume 30  ·  pausar timer",204,145,508,15,0xEEEEF3);
    text(L"Também: arquivos, relógio, meu pc e configurações.",204,181,508,12,0xB7B7C1);
    text(L"Comando",204,232,508,13,0xDADAE0,true);
    box(204,262,390,44,selectAll?0x5C6481:0xFFFFFF,9,selectAll?.4f:inputFocused?.12f:.065f);
    if(inputFocused){use(accent(),.9f);canvas->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(204,262,594,306),9,9),brush,1);}
    text(command.empty()?L"Ex.: timer 5":command+(inputFocused?L"│":L""),217,274,366,14,command.empty()?0xC0C0CA:0xF5F5F8);
    hit(240,204,262,390,44,L"Comando local");button(241,L"Executar",606,262,106,44,true);
    text(feedback,204,330,508,12,0xD2D2DC);
    button(230,L"Prévia visual de voz",204,387,230,32);
    text(L"Microfone desligado",450,397,260,12,0xB7B7C1);
}
#include "sound-ui.h"
void drawPage(int index){
    switch(index){case 0:drawHome();break;case 1:drawAssistant();break;case 2:drawTimer();break;
        case 3:drawSystem();break;case 4:drawFiles();break;case 5:drawSettings();break;case 6:drawSound();break;}
}
void drawContent(){
    box(12,72,168,386,0xFFFFFF,16,.035f);
    if(page==5){
        float fade=previousPage==5?1:unit(pageFade.value);
        canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(12,72,180,458),nullptr,
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),fade),nullptr);
        const std::wstring names[]={L"Aparência",L"Movimento",L"Tela e áudio",L"Assistant",L"Monitoramento",L"Geral"};
        const wchar_t icons[]={0xE790,0xE916,0xE770,0xE8F2,0xE9D9,0xE713};
        box(20,164+44.0f*(settingsSection==5?0:settingsSection+1),152,35,accent(),8,.92f);
        for(int row=-1;row<6;row++){
            int i=row==0?5:row-1;
            int id=row<0?610:600+i;float y=row<0?94:164+row*44.0f;
            bool selected=row>=0&&i==settingsSection;
            if(!selected&&(hoverId==id||keyboardId==id))box(20,y,152,35,0xFFFFFF,8,.07f);
            UINT tint=selected?accentForeground():0xD2D2D9;
            symbol(row<0?0xE72B:icons[i],31,y+10,15,tint);
            text(row<0?L"Voltar":names[i],54,y+8,112,13,tint,selected);
            if(keyboardId==id){use(0xFFFFFF,.75f);canvas->DrawRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(19,y-1,173,y+36),9,9),brush,1);}
            hit(id,20,y,152,35,row<0?L"Voltar à página anterior":names[i]);
        }
        canvas->PopLayer();
    }else{
    static const std::wstring names[]={L"Assistant",L"Comandos",L"Temporizador",L"Meu PC",L"Arquivos",L"Personalizar",L"Som"};
    static const wchar_t icons[]={0xE8F2,0xE756,0xE916,0xE770,0xE8B7,0xE713,0xE767};
    if(!(navigationDragging&&navigationCandidate==page))
        box(20,static_cast<float>(navigationDragging?navigationRows[page].value:navPosition.value),152,35,accent(),8,.92f);
    auto drawItem=[&](int i){
        float y=static_cast<float>(navigationRows[i].value);
        bool selected=page==i;
        bool dragged=navigationDragging&&navigationCandidate==i;
        auto& m=controlMotion[100+i];
        m.hover.target=hoverId==100+i||keyboardId==100+i?1:0;m.hover.frequency=38;
        if(motionReduced())m.hover.snap(m.hover.target);
        if(dragged)box(18,y-2,156,39,0x08080C,10,.75f);
        if(selected&&dragged)box(20,y,152,35,accent(),8,.92f);
        else if(!selected)box(20,y,152,35,0xFFFFFF,8,dragged?.12f:.07f*unit(m.hover.value));
        UINT tint=selected?accentForeground():0xD2D2D9;
        symbol(icons[i],31,y+10,15,tint);
        text(names[i],54,y+8,112,13,tint,selected);
        if(keyboardId==100+i){use(0xFFFFFF,.75f);canvas->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(19,y-1,173,y+36),9,9),brush,1);}
        hit(100+i,20,y,152,35,names[i]);
    };
    for(int item:navigation.items)if(!(navigationDragging&&navigationCandidate==item))drawItem(item);
    if(navigationDragging)drawItem(navigationCandidate);
    }
    float p=unit(pageFade.value);
    if(p<.999f&&previousPage!=page){
        size_t before=hits.size();
        canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,72,722,461),nullptr,
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),1-p),nullptr);
        drawPage(previousPage);canvas->PopLayer();
        hits.resize(before);
    }
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,72,722,461),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),p),nullptr);
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    float scale=blend(.985f,1,p),dx=(1-scale)*458,dy=(1-scale)*100;
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale,D2D1::Point2F(458,100))*original);
    size_t before=hits.size();
    drawPage(page);
    for(size_t i=before;i<hits.size();i++){
        auto& r=hits[i].rect;r=D2D1::RectF(r.left*scale+dx,r.top*scale+dy,r.right*scale+dx,r.bottom*scale+dy);
    }
    if(page==0)volumeTrack=D2D1::RectF(volumeTrack.left*scale+dx,volumeTrack.top*scale+dy,
        volumeTrack.right*scale+dx,volumeTrack.bottom*scale+dy);
    canvas->SetTransform(original);canvas->PopLayer();
}
void drawTooltip(float w){
    bool iconOnly=hoverId==10||hoverId==12||hoverId==451||hoverId==452||hoverId==461||hoverId==470||
        (hoverId>=520&&hoverId<520+hext::MetricCount);
    bool show=iconOnly&&!popoverOpen&&GetTickCount64()-hoverSince>550;
    double target=show?1:0;
    if(tooltipMotion.target!=target){tooltipMotion.target=target;tooltipMotion.frequency=34;invalidate(200);}
    if(motionReduced())tooltipMotion.snap(target);
    float p=unit(tooltipMotion.value);
    if(p<.005f)return;
    auto item=std::find_if(hits.begin(),hits.end(),[](const Hit& h){return h.id==hoverId;});
    if(item==hits.end())return;
    float tw=std::min(w-24,static_cast<float>(item->name.size()*7+20));
    float x=std::clamp((item->rect.left+item->rect.right-tw)/2,12.0f,w-tw-12);
    float y=item->rect.bottom+8;
    box(x,y,tw,30,0x353538,7,.98f*p);
    text(item->name,x+10,y+6,tw-20,12,0xF2F2F7,false,p);
}
void closePopover(){
    popoverOpen=false;popoverMotion.target=0;popoverMotion.frequency=settingsMenu?60:36;
    if(motionReduced())popoverMotion.snap(0);
}
void openPopover(){
    settingsMenu=0;popoverOpen=true;popoverMotion.target=1;popoverMotion.frequency=30;
    if(motionReduced())popoverMotion.snap(1);
}
void openSettingsMenu(int kind){
    openPopover();settingsMenu=kind;popoverMotion.frequency=55;keyboardId=-1;
}
void openColorPicker(){
    closePopover();popoverMotion.snap(0);
    draftColor=hext::hsv(accentPalette.current());colorPickerOpen=true;
    colorPickerMotion.target=1;colorPickerMotion.frequency=50;keyboardId=-1;
    if(motionReduced())colorPickerMotion.snap(1);
}
void closeColorPicker(){
    colorPickerOpen=false;colorPickerMotion.target=0;colorPickerMotion.frequency=55;
    if(dragColor){dragColor=0;ReleaseCapture();}
    if(motionReduced())colorPickerMotion.snap(0);
}
void updateColorPointer(float x,float y,int target){
    if(target==331){
        double dx=x-550,dy=y-229;
        draftColor.h=std::fmod(std::atan2(dy,dx)*180/3.141592653589793+360,360.0);
        draftColor.s=std::min(1.0,std::hypot(dx,dy)/88);
    }else if(target==332)draftColor.v=std::clamp((x-468)/220.0,0.0,1.0);
    invalidate();
}
void drawColorPicker(){
    float p=unit(colorPickerMotion.value);if(p<.002f)return;
    if(!colorWheelBitmap){
        constexpr int size=192;std::vector<BYTE> data(size*size*4);
        for(int y=0;y<size;y++)for(int x=0;x<size;x++){
            double dx=x-95.5,dy=y-95.5,r=std::hypot(dx,dy),a=std::clamp(96-r,0.0,1.0);
            auto color=hext::rgb({std::atan2(dy,dx)*180/3.141592653589793,std::min(1.0,r/95),1});
            size_t i=(y*size+x)*4;
            data[i]=static_cast<BYTE>((color&255)*a);data[i+1]=static_cast<BYTE>(((color>>8)&255)*a);
            data[i+2]=static_cast<BYTE>(((color>>16)&255)*a);data[i+3]=static_cast<BYTE>(255*a);
        }
        canvas->CreateBitmap(D2D1::SizeU(size,size),data.data(),size*4,
            D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED)),
            &colorWheelBitmap);
    }
    size_t before=hits.size();
    if(colorPickerOpen){hits.clear();before=0;}
    canvas->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),p),nullptr);
    box(196,72,526,386,0,16,.18f);
    box(388,78,324,370,0x303033,16,.99f);
    use(0xFFFFFF,.14f);canvas->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(388,78,712,448),16,16),brush,.8f);
    text(L"Cor de destaque",410,96,242,17,0xF4F4F8,true);
    iconButton(334,0xE711,L"Fechar seletor de cor",670,90,28,28);
    if(colorWheelBitmap)canvas->DrawBitmap(colorWheelBitmap,D2D1::RectF(462,141,638,317));
    dot(550,229,88,0,static_cast<float>(1-draftColor.v));
    float angle=static_cast<float>(draftColor.h*3.141592653589793/180);
    float px=550+88*static_cast<float>(draftColor.s)*std::cos(angle);
    float py=229+88*static_cast<float>(draftColor.s)*std::sin(angle);
    use(0);canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(px,py),6.5f,6.5f),brush,3);
    use(0xFFFFFF);canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(px,py),6.5f,6.5f),brush,1.5f);
    if(keyboardId==331){use(0xFFFFFF);canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(550,229),92,92),brush,1);}
    hit(331,458,137,184,184,L"Roda de cores: matiz e saturação");
    text(L"Brilho",410,332,55,12,0xD2D2DA);
    for(int i=0;i<110;i++){
        auto c=draftColor;c.v=i/109.0;
        box(468+i*2.0f,337,2.1f,12,hext::rgb(c),0);
    }
    float knob=468+220*static_cast<float>(draftColor.v);
    dot(knob,343,7,hext::rgb(draftColor));use(0xFFFFFF);
    canvas->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(knob,343),7,7),brush,2);
    if(keyboardId==332){use(0xFFFFFF);canvas->DrawRectangle(D2D1::RectF(464,332,692,354),brush,1);}
    hit(332,460,327,236,32,L"Brilho da cor");
    text(L"HEX",410,368,40,11,0xB7B7C1);
    text(hext::hex(hext::rgb(draftColor)),449,363,160,16,0xF4F4F8,true);
    dot(674,374,12,hext::rgb(draftColor));
    button(336,L"Cancelar",410,401,134,32);
    button(335,L"Aplicar",558,401,134,32,true);
    canvas->PopLayer();
    if(!colorPickerOpen)hits.resize(before);
}
void drawSensorSources(float p){
    float x=388,y=128,w=324;
    auto msi=sensorApplication(true),libre=sensorApplication(false);
    int buttons=1+!msi.empty()+!libre.empty();float h=140+buttons*40.0f;
    float scale=blend(.96f,1,p),dx=(1-scale)*(x+w),dy=(1-scale)*y;
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),p),nullptr);
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale,D2D1::Point2F(x+w,y))*original);
    box(x,y,w,h,0x303033,14,.99f);
    use(0xFFFFFF,.14f);canvas->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x,y,x+w,y+h),14,14),brush,.8f);
    if(popoverOpen)hits.clear();
    size_t before=hits.size();
    text(L"Sensores",x+20,y+18,w-72,17,0xF4F4F8,true);
    iconButton(754,0xE711,L"Fechar sensores",x+w-42,y+12,28,28);
    auto cpu=hardwareFresh()?hardwareSnapshot.cpuTemperature.source:L"";
    if(cpu.empty()&&hardwareFresh())cpu=hardwareSnapshot.cpuPower.source;
    text(cpu.empty()?(hardwareFresh()&&(hardwareSnapshot.wmi||hardwareSnapshot.afterburner)?
        L"CPU: leitura não disponível":L"CPU: fonte não conectada"):L"CPU: "+cpu,x+20,y+56,w-40,12,0xCECED8);
    text(!hardwareFresh()?L"GPU: aguardando leitura":hardwareSnapshot.nvidia?L"GPU: driver NVIDIA":
        hardwareSnapshot.afterburner?L"GPU: MSI Afterburner":hardwareSnapshot.wmi?L"GPU: monitor externo":
        hardwareSnapshot.gpuLoad.available?L"GPU: contadores do Windows":L"GPU: fonte não conectada",
        x+20,y+79,w-40,12,0xCECED8);
    text(hardwareSnapshot.gpuName,x+20,y+101,w-40,11,0xB7B7C1);
    float row=y+132;
    button(751,L"Atualizar",x+20,row,w-40,32);row+=40;
    if(!msi.empty()){button(752,L"Abrir MSI Afterburner",x+20,row,w-40,32);row+=40;}
    if(!libre.empty())button(753,L"Abrir LibreHardwareMonitor",x+20,row,w-40,32);
    for(size_t i=before;i<hits.size();i++){
        auto& r=hits[i].rect;r=D2D1::RectF(r.left*scale+dx,r.top*scale+dy,r.right*scale+dx,r.bottom*scale+dy);
    }
    if(!popoverOpen)hits.resize(before);
    canvas->SetTransform(original);canvas->PopLayer();
}
void drawPopover(){
    float p=unit(popoverMotion.value);if(p<.002f)return;
    if(settingsMenu==360){drawSensorSources(p);return;}
    struct MenuItem{int id;std::wstring name;wchar_t glyph;};
    std::vector<MenuItem> items{{901,L"Personalizar",0xE713}};
    if(controlEnabled(902))items.push_back({902,translucent?L"Material: translúcido":L"Material: sólido",0xE790});
    items.push_back({903,std::wstring(L"Animação: ")+styleName(),0xE916});
    items.push_back({904,L"Recolher ilha",0xE70E});items.push_back({905,L"Sair do Hext",0xE8BB});
    float x=PanelW-263,y=58,w=241,h=16+static_cast<float>(items.size())*34+
        static_cast<float>(items.size()-1)*6+7;
    if(settingsMenu){
        items.clear();x=498;w=198;
        if(settingsMenu==317){
            items={{701,L"Translúcido",translucent?wchar_t(0xE73E):wchar_t(0)},
                   {702,L"Sólido",!translucent?wchar_t(0xE73E):wchar_t(0)}};
            y=216;
        }else if(settingsMenu==310){
            items={{711,L"Encaixado",!floating?wchar_t(0xE73E):wchar_t(0)},
                   {712,L"Flutuante",floating?wchar_t(0xE73E):wchar_t(0)}};
            y=282;
        }else if(settingsMenu==314){
            for(int i=0;i<4;i++)items.push_back({721+i,std::to_wstring(85+i*15)+L"%",
                std::abs(zoom-(.85f+i*.15f))<.01f?wchar_t(0xE73E):wchar_t(0)});
            y=348;
        }else if(settingsMenu==362){
            for(int i=0;i<3;i++)items.push_back({741+i,i==0?L"1 segundo":i==1?L"2 segundos":L"5 segundos",
                hardwareInterval==(i==0?1000:i==1?2000:5000)?wchar_t(0xE73E):wchar_t(0)});
            y=408;
        }else{
            for(int i=0;i<4;i++)items.push_back({801+i,i==0?L"Nenhum":i==1?L"Codex":i==2?L"Claude":L"Outro CLI",
                assistantProvider==i?wchar_t(0xE73E):wchar_t(0)});
            y=216;
        }
        h=16+static_cast<float>(items.size())*34+static_cast<float>(items.size()-1)*6;
        y=std::min(y,PanelH-h-16);
    }
    float scale=blend(.94f,1,p),dx=(1-scale)*(x+w),dy=(1-scale)*y;
    popoverRect=D2D1::RectF(x*scale+dx,y*scale+dy,(x+w)*scale+dx,(y+h)*scale+dy);
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(),nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),p),nullptr);
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale,D2D1::Point2F(x+w,y))*original);
    for(int i=4;i>0;i--)box(x-i,y+i,w+2*i,h+i,0x000000,13+i,.045f);
    box(x,y,w,h,0x303033,13,.985f);
    use(0xFFFFFF,.14f);canvas->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(x+.5f,y+.5f,x+w-.5f,y+h-.5f),13,13),brush,.8f);
    if(popoverOpen)hits.clear();
    for(size_t i=0;i<items.size();i++){
        auto& item=items[i];bool exit=item.id==905;
        float row=y+8+i*40.0f+(exit?7:0);
        if(exit)line(x+10,row-5,x+w-10,row-5,0xFFFFFF,1,.12f);
        bool selected=hoverId==item.id||keyboardId==item.id;
        if(selected)box(x+6,row,w-12,34,accent(),6,.95f);
        UINT tint=selected?accentForeground():0xF0F0F5;
        if(item.glyph)symbol(item.glyph,x+15,row+10,14,tint);
        alignedText(item.name,x+42,row,w-52,34,13,tint,false,false);
        if(popoverOpen)hit(item.id,(x+6)*scale+dx,row*scale+dy,(w-12)*scale,34*scale,item.name);
    }
    canvas->SetTransform(original);canvas->PopLayer();
}
void drawSharedHeader(float w,float progress){
    textScaled(L"Hext",blend(18,28,progress),blend(9,21,progress),
        blend(15,20,progress),0xF4F4F8,120,true);
    bool strip=voicePreview||GetTickCount64()<stripUntil||timerRunning;
    float clockAlpha=strip?progress:1;
    number(500,clockLabel(),blend(w-78,w-158,progress),blend(9,24,progress),
           blend(15,13,progress),0xD2D2DA,80,clockAlpha);
    if(progress>.65f){
        float a=smoothRange(.65f,1,progress);
        canvas->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::IdentityMatrix(),a),nullptr);
        iconButton(10,0xE70E,L"Recolher",w-89,17,28,28);
        iconButton(12,0xE712,L"Mais opções",w-51,17,28,28);
        canvas->PopLayer();
    }
    if(progress<.85f&&strip&&!timerRunning){
        float alpha=1-smoothRange(.1f,.85f,progress);
        text(voicePreview?L"Prévia de voz · microfone desligado":stripText,78,12,w-136,12,0xE0E0E8,false,alpha);
        if(voicePreview){
            for(int i=0;i<5;i++){
                float v=static_cast<float>(6+12*(.5+.5*std::sin((motionReduced()?0:GetTickCount64()*.007)+i*1.7)));
                box(w-42+i*5,21-v/2,2.5f,v,accent(),1.2f,alpha);
            }
        }
    }
}
void drawSharedTimer(float w,float progress,float contentVisibility,float contentScale,float offsetX,float offsetY){
    bool large=page==2;
    bool wasLarge=previousPage==2;
    float pageVisibility=large?(wasLarge?1:unit(pageFade.value)):(wasLarge?1-unit(pageFade.value):0);
    float alpha=timerRunning?blend(1,pageVisibility,progress):pageVisibility*contentVisibility;
    if(alpha<.002f)return;
    float expandedX=static_cast<float>(timerX.value)*contentScale+offsetX;
    float expandedY=static_cast<float>(timerY.value)*contentScale+offsetY;
    float matchedProgress=timerRunning?progress:1;
    float x=blend(78,expandedX,matchedProgress),y=blend(10,expandedY,matchedProgress);
    float size=blend(13,static_cast<float>(timerSize.value)*contentScale,matchedProgress);
    number(504,timerLabel(),x,y,size,timerRunning?0xF4DBA2:0xEEEEF6,w-x-20,alpha);
    float buttonAlpha=contentVisibility*pageVisibility;
    if(buttonAlpha>.002f){
        float bx=static_cast<float>(timerButtonX.value)*contentScale+offsetX;
        float by=static_cast<float>(timerButtonY.value)*contentScale+offsetY;
        float bw=static_cast<float>(timerButtonW.value)*contentScale;
        float bh=static_cast<float>(timerButtonH.value)*contentScale;
        canvas->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(),nullptr,
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),buttonAlpha),nullptr);
        size_t before=hits.size();
        button(220,timerRunning?L"Pausar":L"Iniciar",bx,by,bw,bh,true);
        canvas->PopLayer();
        if(buttonAlpha<.3f||bx<2||bx+bw>w-2||by+bh>height.value-2)hits.resize(before);
    }
}
void render(){
    if(!canvas||(!testMode&&!windowShown))return;
    if(!testMode&&appearance.value<.002&&appearance.target==0){hideImmediately();return;}
    float w=static_cast<float>(width.value),h=static_cast<float>(height.value);
    // Content and matched geometry follow the actual shell, not a second clock.
    float progress=unit((h-42)/(PanelH-42));
    int pxW=std::clamp(static_cast<int>(std::ceil(w*dpi)),1,MaxW);
    int pxH=std::clamp(static_cast<int>(std::ceil((h+10)*dpi)),1,MaxH);
    RECT bound{0,0,pxW,pxH};if(FAILED(canvas->BindDC(memoryDC,&bound)))return;
    if(materialDirty){
        if(translucent&&systemTransparency&&!highContrast)wallpaper.refresh(canvas,monitorRect,dpi,PanelW,PanelH);
        materialDirty=false;
    }
    canvas->BeginDraw();canvas->SetTransform(D2D1::Matrix3x2F::Identity());canvas->Clear(color(0,0));
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(dpi,dpi));hits.clear();
    float radius=blend(floating?21:15,28,progress);
    for(int i=4;i>0;i--)box(2+i*.25f,1+i*.6f,w-4-i*.5f,h+i*.8f,0x000000,radius,.045f);
    D2D1_RECT_F surface=D2D1::RectF(2,floating?1:-24,w-2,h);
    ID2D1RoundedRectangleGeometry* shape=nullptr;
    factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(surface,radius,radius),&shape);
    if(shape){
        canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(0,0,w,h),shape),nullptr);
        box(0,-30,w,h+30,0x19191D,0);
        if(translucent&&systemTransparency&&!highContrast&&wallpaper.ready){
            float x=(w-wallpaper.logicalWidth)/2,y=floating?-12:0;
            canvas->DrawBitmap(wallpaper.bitmap,D2D1::RectF(x,y,x+wallpaper.logicalWidth,y+wallpaper.logicalHeight),1);
            box(0,-30,w,h+30,0x101014,0,blend(.91f,.83f,progress));
        }
        float content=smoothRange(.55f,.95f,progress);
        // Keep the complete layout inside the growing shell. Centering a
        // full-sized page here exposed clipped labels before expansion ended.
        float scale=std::min({blend(.965f,1,content),w/PanelW,h/PanelH});
        float offsetX=(w-PanelW*scale)/2;
        float offsetY=(1-content)*4;
        if(content>.002f){
            canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(0,0,w,h),nullptr,
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),content),nullptr);
            canvas->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale)*
                D2D1::Matrix3x2F::Translation(offsetX,offsetY)*D2D1::Matrix3x2F::Scale(dpi,dpi));
            drawContent();
            canvas->PopLayer();
            for(auto& item:hits){
                auto& r=item.rect;
                r=D2D1::RectF(r.left*scale+offsetX,r.top*scale+offsetY,r.right*scale+offsetX,r.bottom*scale+offsetY);
            }
            volumeTrack=D2D1::RectF(volumeTrack.left*scale+offsetX,volumeTrack.top*scale+offsetY,
                volumeTrack.right*scale+offsetX,volumeTrack.bottom*scale+offsetY);
            hits.erase(std::remove_if(hits.begin(),hits.end(),[&](const Hit& item){
                return item.rect.left<2||item.rect.right>w-2||item.rect.bottom>h-2||content<.3f;
            }),hits.end());
        }
        canvas->SetTransform(D2D1::Matrix3x2F::Scale(dpi,dpi));
        drawSharedTimer(w,progress,content,scale,offsetX,offsetY);
        drawSharedHeader(w,progress);
        if(progress>.95f)drawPopover();
        if(progress>.95f)drawColorPicker();
        if(progress>.99f&&!popoverOpen&&!colorPickerOpen)drawTooltip(w);
        canvas->PopLayer();shape->Release();
    }
    use(0xFFFFFF,.09f+.035f*progress);
    canvas->DrawRoundedRectangle(D2D1::RoundedRect(surface,radius,radius),brush,.7f);
    lastRenderResult=canvas->EndDraw();if(FAILED(lastRenderResult))return;
    POINT pos{monitorRect.left+(monitorRect.right-monitorRect.left-pxW)/2,monitorRect.top+(floating?static_cast<int>(12*dpi):0)};
    SIZE size{pxW,pxH};POINT src{0,0};
    BLENDFUNCTION blendFunction{AC_SRC_OVER,0,static_cast<BYTE>(unit(appearance.value)*255),AC_SRC_ALPHA};
    lastPresentedAlpha=blendFunction.SourceConstantAlpha;
    lastUpdateSucceeded=UpdateLayeredWindow(windowHandle,nullptr,&pos,&size,memoryDC,&src,0,&blendFunction,ULW_ALPHA);
    notifyAccessibility();
}
void stepMotion(double seconds){
    personal.transition.step(seconds);
    for(auto* s:{&width,&height,&reveal,&pageFade,&appearance,&navPosition,&settingsFade,&colorPickerMotion,&sendState,&popoverMotion,&tooltipMotion,
                 &timerX,&timerY,&timerSize,&timerSymbol,&timerButtonX,&timerButtonY,&timerButtonW,&timerButtonH,
                 &volumeVisual,&cpuVisual,&ramVisual,&soundTabVisual,&soundPageFade})s->step(seconds);
    for(auto& item:controlMotion){item.second.hover.step(seconds);item.second.press.step(seconds);}
    for(auto& item:toggleMotion)item.second.position.step(seconds);
    for(int i=0;i<7;i++)if(!(navigationDragging&&navigationCandidate==i))navigationRows[i].step(seconds);
    for(auto& item:numberMotion)item.second.progress.step(seconds);
}
bool motionBusy(){
    if(personal.transition.moving())return true;
    for(auto* s:{&width,&height,&reveal,&pageFade,&appearance,&navPosition,&settingsFade,&colorPickerMotion,&sendState,&popoverMotion,&tooltipMotion,
                 &timerX,&timerY,&timerSize,&timerSymbol,&timerButtonX,&timerButtonY,&timerButtonW,&timerButtonH,
                 &volumeVisual,&cpuVisual,&ramVisual,&soundTabVisual,&soundPageFade})if(s->moving())return true;
    for(auto& item:controlMotion)if(item.second.hover.moving()||item.second.press.moving())return true;
    for(auto& item:toggleMotion)if(item.second.position.moving())return true;
    for(auto& row:navigationRows)if(row.moving())return true;
    for(auto& item:numberMotion)if(item.second.progress.moving())return true;
    return false;
}
void snapMotion(){
    personal.transition.snap(personal.transition.target);
    for(auto* s:{&width,&height,&reveal,&pageFade,&appearance,&navPosition,&settingsFade,&colorPickerMotion,&sendState,&popoverMotion,&tooltipMotion,
                 &timerX,&timerY,&timerSize,&timerSymbol,&timerButtonX,&timerButtonY,&timerButtonW,&timerButtonH,
                 &volumeVisual,&cpuVisual,&ramVisual,&soundTabVisual,&soundPageFade})s->snap(s->target);
    for(auto& item:controlMotion){item.second.hover.snap(item.second.hover.target);item.second.press.snap(item.second.press.target);}
    for(auto& item:toggleMotion)item.second.position.snap(item.second.position.target);
    for(auto& row:navigationRows)row.snap(row.target);
    for(auto& item:numberMotion)item.second.progress.snap(1);
}

