#pragma once
void personalParagraph(const std::wstring& value,float x,float y,float w,float h,int size=12,UINT tint=0xB7B7C1){
    IDWriteTextLayout* layout=nullptr;
    if(SUCCEEDED(textFactory->CreateTextLayout(value.c_str(),static_cast<UINT32>(value.size()),font(size,false),w,h,&layout))){
        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);use(tint);canvas->DrawTextLayout(D2D1::Point2F(x,y),layout,brush);layout->Release();
    }
}
void drawPersonalOverlay(){
    auto view=personal.view;
    text(view==PersonalView::Chats?L"Conversas":view==PersonalView::Memory?L"Memória":view==PersonalView::Assets?L"Itens autorizados":view==PersonalView::Editor?personal.editorTitle:L"Assistant",204,87,454,21,0xF5F5F8,true);
    iconButton(3191,0xE711,L"Fechar painel",676,87,30,30);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,125,722,470),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),unit(personal.transition.value)),nullptr);
    if(view==PersonalView::Chats||view==PersonalView::Memory||view==PersonalView::Assets){
        if(personal.rows.empty())alignedText(view==PersonalView::Chats?L"Nenhuma conversa salva":view==PersonalView::Memory?L"Nenhuma preferência guardada":L"Nenhum item autorizado",204,230,508,32,18,0xD8D8E1);
        for(int row=0;row<4;row++){
            int index=personal.listPage*4+row;if(index>=static_cast<int>(personal.rows.size()))break;auto& item=personal.rows[index];float y=142+row*55.f;
            std::wstring title=hext::wide(view==PersonalView::Chats?item.value("title","Conversa"):view==PersonalView::Memory?item.value("key",""):item.value("name",""));
            std::wstring detail=view==PersonalView::Memory?(item.value("explicit",false)?L"Declarada · ":L"Sugerida · ")+hext::wide(item.value("value","")):view==PersonalView::Assets?(item.value("app",false)?L"Aplicativo":L"Arquivo ou pasta"):L"Histórico independente";
            button(3100+row,L"",204,y,416,47);alignedText(title,218,y+3,388,22,13,0xEEEEF4,true,false);alignedText(detail,218,y+25,388,18,11,0xB7B7C1,false,false);hits.back().name=title;
            if(view==PersonalView::Chats)iconButton(3110+row,0xE70F,L"Renomear conversa",626,y+8,32,32);
            iconButton(3120+row,0xE74D,view==PersonalView::Assets?L"Revogar autorização":L"Excluir",672,y+8,32,32);
        }
        if(view==PersonalView::Chats)button(3091,personal.saved?L"Salvar alterações":L"Salvar conversa atual",204,377,244,32,true);
        if(view==PersonalView::Memory)button(3063,L"Nova preferência",204,377,244,32,true);
        if(view==PersonalView::Assets){button(3072,L"Aplicativo",204,377,160,32);button(3073,L"Arquivo",376,377,160,32);button(3074,L"Pasta",548,377,164,32);}
        if(personal.rows.size()>4){
            button(3098,L"Anterior",204,422,108,30);button(3099,L"Próximos",324,422,108,30);
            alignedText(std::to_wstring((personal.rows.size()+3)/4)+L"/"+std::to_wstring(personal.listPage+1),448,422,80,30,11,0x9A9AA6,false,false);
        }
    }else if(view==PersonalView::Editor){
        symbol(0xE70F,440,179,32,0xC9C9D2);
        personalParagraph(personal.editorKind=="memory"?L"Uma preferência curta por item. Exemplo: estilo: respostas objetivas.":personal.editorKind=="spotify-client"?L"Use o Client ID do seu aplicativo Spotify e cadastre o redirect http://127.0.0.1/callback. O navegador cuida do login.":L"Edite o campo e confirme para salvar.",220,240,476,90,13);
        drawAssistantInput();iconButton(3094,0xE73E,L"Salvar",666,369,46,44);
        button(3090,L"Cancelar",554,424,158,30);
    }else if(view==PersonalView::NewChat){
        symbol(0xE8F2,440,176,32,0xC9C9D2);alignedText(personal.pendingChat.empty()?L"Começar uma nova conversa?":L"Abrir outra conversa?",204,240,508,30,19,0xF0F0F5,true);
        personalParagraph(L"Você pode guardar o histórico atual ou descartá-lo. Descartar também remove as preferências derivadas que não foram fixadas.",236,289,444,64,13);
        button(3092,personal.pendingChat.empty()?L"Salvar e criar nova":L"Salvar e abrir",204,377,244,35,true);button(3093,personal.pendingChat.empty()?L"Descartar e criar nova":L"Descartar e abrir",460,377,252,35);button(3090,L"Cancelar",554,424,158,30);
    }else if(view==PersonalView::Action){
        symbol(0xE72E,440,173,32,0xC9C9D2);alignedText(L"Permitir esta ação?",204,230,508,30,20,0xF0F0F5,true);
        if(personal.approval)personalParagraph(personalActionLabel(personal.approval->request),236,284,444,70,16,0xE5E5ED);
        personalParagraph(L"Esta autorização vale somente para esta solicitação e expira em dois minutos.",236,354,444,38,12);
        button(3090,L"Cancelar",204,414,244,35);button(3095,L"Permitir uma vez",460,414,252,35,true);
    }else if(view==PersonalView::Remember){
        symbol(0xE8A5,440,173,32,0xC9C9D2);alignedText(L"Guardar esta preferência?",204,230,508,30,20,0xF0F0F5,true);
        if(personal.memoryDraft.is_object()){
            text(hext::wide(personal.memoryDraft.value("key","")),236,282,444,14,0xF0F0F5,true);
            if(personal.memoryDraft.contains("previous")){text(L"Antes: "+hext::wide(personal.memoryDraft.value("previous","")),236,311,444,12,0xB7B7C1);personalParagraph(L"Agora: "+hext::wide(personal.memoryDraft.value("value","")),236,337,444,44,14,0xDDDEE6);}
            else personalParagraph(hext::wide(personal.memoryDraft.value("value","")),236,310,444,66,14,0xDDDEE6);
        }
        text(L"Sugestão do assistente · pode ser editada ou excluída",236,383,444,11,0xB7B7C1);
        button(3090,L"Agora não",204,414,244,35);button(3096,L"Guardar",460,414,252,35,true);
    }else if(view==PersonalView::Delete){
        symbol(0xE74D,440,176,32,0xC9C9D2);alignedText(personal.editorID.rfind("asset:",0)==0?L"Revogar esta autorização?":L"Excluir estes dados?",204,240,508,30,20,0xF0F0F5,true);
        personalParagraph(personal.editorID=="all-data"?L"Conversas, memória, itens autorizados e a conexão Spotify serão excluídos do Hext. A autenticação dos CLIs não será alterada.":personal.editorID.rfind("chat:",0)==0?L"A conversa e suas memórias derivadas não fixadas serão excluídas do Hext.":L"Esta alteração é permanente. Solicitações pendentes também serão canceladas.",236,295,444,84,13);
        button(3090,L"Cancelar",204,414,244,35);button(3097,personal.editorID.rfind("asset:",0)==0?L"Revogar":L"Excluir",460,414,252,35,true);
    }
    if(!personal.notice.empty())text(personal.notice,212,448,492,10,0xD9D9E2);
    canvas->PopLayer();
}
void drawPersonalSpotify(){
    auto state=personal.spotify.snapshot();
    if(personal.spotifyPage==0){
        box(204,168,508,112,0xFFFFFF,13,.045f);text(L"Spotify",220,183,450,16,0xF0F0F5,true);
        personalParagraph(state.status,220,214,476,54,12);
        button(3080,L"Configurar Client ID",204,298,244,32);
        button(3081,state.busy?L"Conectando…":state.credentials?L"Verificar conexão":L"Conectar",460,298,252,32,true);
        if(state.credentials){button(3083,L"Pesquisar músicas",204,344,244,32);button(3084,L"Dispositivos",460,344,252,32);button(3190,L"Gerenciar acesso",204,414,244,30);button(3082,L"Desconectar",554,414,158,30);}
        else personalParagraph(L"Conexão opcional. O controle de reprodução depende de Spotify Premium e do acesso autorizado ao aplicativo.",220,356,476,62,12);
    }else{
        button(3089,L"Voltar",204,168,100,30);button(3083,L"Pesquisar",316,168,180,30);button(3084,L"Dispositivos",508,168,204,30);
        if(personal.spotifyPage==1){
            for(size_t i=0;i<state.tracks.size()&&i<5;i++){
                float y=209+i*36.f;auto& track=state.tracks[i];
                button(3140+static_cast<int>(i),L"",204,y,466,33);alignedText(hext::wide(track.value("name","")),216,y,290,33,12,0xEEEEF4,true,false);
                alignedText(hext::wide(track.value("artist","")),506,y,148,33,10,0xB7B7C1,false,false);hits.back().name=L"Reproduzir: "+hext::wide(track.value("name",""));
                iconButton(3180+static_cast<int>(i),0xE8A7,L"Abrir no Spotify",678,y,32,32);
            }
            if(state.tracks.empty())alignedText(state.busy?L"Pesquisando…":L"Pesquise uma música para começar",204,278,508,32,16,0xC8C8D3);
        }else{
            for(int row=0;row<4;row++){int index=personal.listPage*4+row;if(index>=static_cast<int>(state.devices.size()))break;auto& d=state.devices[index];float y=209+row*39.f;
                button(3150+row,L"",204,y,508,35,false,d.value("active",false));alignedText(hext::wide(d.value("name","")),218,y,344,35,12,0xEEEEF4,true,false);alignedText(d.value("restricted",true)?L"Restrito":d.value("active",false)?L"Ativo":L"Selecionar",562,y,132,35,11,0xB7B7C1,false,false);hits.back().name=L"Selecionar dispositivo: "+hext::wide(d.value("name",""));
            }
            if(state.devices.empty())alignedText(state.busy?L"Consultando dispositivos…":L"Abra o Spotify em um dispositivo",204,278,508,32,16,0xC8C8D3);
            if(state.devices.size()>4){iconButton(3098,0xE76B,L"Anterior",204,373,28,26);iconButton(3099,0xE76C,L"Próximos",246,373,28,26);text(std::to_wstring((state.devices.size()+3)/4)+L"/"+std::to_wstring(personal.listPage+1),290,379,100,10,0x9A9AA6);}
        }
        button(3085,L"Anterior",204,407,118,30);button(3086,L"Reproduzir",334,407,118,30);button(3087,L"Pausar",464,407,118,30);button(3088,L"Próxima",594,407,118,30);
        if(personal.notice.empty())text(L"Spotify · "+state.status,212,447,492,10,0xB7B7C1);
    }
}
void drawPersonalSettings(){
    const wchar_t* names[]={L"Provedor",L"Ações",L"Memória",L"Dados",L"Spotify"};
    for(int i=0;i<5;i++)button(3000+i,names[i],204+i*102.f,130,100,28,false,personal.settingsTab==i);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,164,722,470),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),unit(personal.transition.value)),nullptr);
    if(personal.settingsTab==0){
        box(204,168,508,66,0xFFFFFF,13,.045f);text(L"Assistente",220,184,250,14,0xF0F0F5,true);settingsChoice(465,L"Assistente",assistantName(),178);
        box(204,246,508,134,0xFFFFFF,13,.045f);text(L"Conexão",220,259,476,14,0xF0F0F5,true);text(assistantConnectionMessage(),220,286,476,12,0xB7B7C1);
        auto state=connectionState[assistantProvider];button(467,state==hext::ConnectionState::Checking?L"Verificando…":state==hext::ConnectionState::Connected?L"Reconectar":state==hext::ConnectionState::NeedsLogin?L"Entrar":L"Conectar",220,326,160,32,true);
        if(assistantProvider==3||state==hext::ConnectionState::Missing||state==hext::ConnectionState::Incompatible||state==hext::ConnectionState::Error)button(466,L"Localizar executável",396,326,300,32);
        else if(state==hext::ConnectionState::Connected)button(3011,L"Desconectar",538,326,158,32);
        text(L"Idioma das respostas",220,389,476,12,0xB7B7C1);
        button(3160,L"Automático",220,412,151,30,false,personal.preferences.language=="auto");button(3161,L"Português",383,412,151,30,false,personal.preferences.language=="pt-BR");button(3162,L"English",546,412,150,30,false,personal.preferences.language=="en");
    }else if(personal.settingsTab==1){
        text(L"Autonomia",220,174,476,13,0xDADAE4,true);
        const wchar_t* levels[]={L"Conversar",L"Pedir autorização",L"Ações permitidas"};for(int i=0;i<3;i++)button(3020+i,levels[i],204+i*172.f,200,164,31,false,static_cast<int>(personal.preferences.autonomy)==i);
        for(int row=0;row<4;row++){int category=personal.permissionPage*4+row;float y=247+row*38.f;box(204,y,508,34,0xFFFFFF,8,.035f);alignedText(hext::categoryName(static_cast<hext::Category>(category)),218,y,304,34,12,0xDEDEE7,false,false);auto grant=personal.preferences.grants[category];button(3040+category,grant==hext::Grant::Deny?L"Negar":grant==hext::Grant::Ask?L"Perguntar":L"Permitir",536,y+2,164,30,false,grant==hext::Grant::Allow);}
        button(3050,personal.permissionPage?L"Anteriores":L"Mais permissões",204,414,200,30);button(3170,L"Revogar todas",536,414,176,30);
        alignedText(L"2/"+std::to_wstring(personal.permissionPage+1),416,414,80,30,10,0x9A9AA6,false,false);
        if(personal.notice.empty())text(L"Ações sensíveis sempre exigem confirmação.",220,450,476,10,0xB7B7C1);
    }else if(personal.settingsTab==2){
        box(204,168,508,92,0xFFFFFF,13,.045f);drawToggle(3060,personal.preferences.memory,220,187,36,20,true);text(L"Memória compartilhada",268,189,420,14,0xEEEEF4,true);hit(3060,216,177,480,43,L"Ativar memória compartilhada");
        text(L"Preferências revisáveis entre suas conversas",220,231,476,12,0xB7B7C1);button(3061,L"Gerenciar memória",204,282,244,34,true);button(3063,L"Nova preferência",460,282,252,34);
        personalParagraph(L"O assistente sugere informações úteis. Você decide o que guardar. Inferências não substituem preferências explícitas.",220,340,476,64,13);button(3062,L"Apagar memória",204,414,244,30);
    }else if(personal.settingsTab==3){
        box(204,168,508,90,0xFFFFFF,13,.045f);text(L"Suas conversas",220,184,476,15,0xF0F0F5,true);text(L"Históricos independentes, salvos neste computador",220,211,476,12,0xB7B7C1);
        button(3070,L"Conversas salvas",204,278,244,34,true);button(3091,L"Salvar conversa atual",460,278,252,34);button(3071,L"Itens autorizados",204,330,508,34);
        button(3075,L"Apagar dados do Assistant",204,414,300,30);personalParagraph(L"Gerencie aplicativos, arquivos e pastas que podem ser abertos.",220,381,476,24,12);
    }else drawPersonalSpotify();
    if(!personal.notice.empty())text(personal.notice,212,448,492,10,0xD9D9E2);
    canvas->PopLayer();
}
