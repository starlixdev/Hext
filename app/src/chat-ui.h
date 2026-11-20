#pragma once
struct ChatCardContent {
    std::wstring title,body,state;bool live=false;
    std::vector<std::pair<int,std::wstring>> buttons;
    std::map<int,std::string> targets;
};
ChatCardContent personalChatContent(const ChatMessage& message){
    ChatCardContent out{hext::wide(message.card.value("title","")),message.text,hext::chatCardState(message.card.value("state","paused")),false,{}};
    out.live=message.card.value("id","")==personal.inlineID&&message.card.value("chat","")==personal.chat;
    if(!out.live)return out;
    auto add=[&](int id,const wchar_t* label){out.buttons.emplace_back(id,label);};
    if(message.card.value("kind","")=="resume"){add(3490,L"Retomar tarefa");add(3491,L"Cancelar tarefa");return out;}
    if(personal.inlineEditor){out.body+=L"\n\n"+personal.editorTitle+L"\nPreencha o campo abaixo. A conversa continua aqui.";add(3094,L"Salvar e continuar");add(3492,L"Voltar");add(3090,L"Cancelar tarefa");return out;}
    auto view=personal.inlineView;
    if(view==PersonalView::Action){add(3095,L"Permitir");add(3493,L"Negar");add(3090,L"Cancelar");if(personal.approval&&!personal.disclosure&&(personal.approval->request.name=="files.create_text"||!personalAgentLabel(personal.approval->request).empty()))add(3440,personal.inlineDetails?L"Ocultar detalhes":L"Ver detalhes");
        if(personal.inlineDetails&&personal.approval){auto& request=personal.approval->request;out.body+=L"\n\n"+(request.name=="files.create_text"?hext::wide(request.args.value("text","")):personalOperationDetails());}
    }else if(view==PersonalView::WindowConsent){add(3450,L"Permitir leitura");add(3493,L"Negar");add(3090,L"Cancelar");}
    else if(view==PersonalView::Intervention){add(3423,L"Pausar e guardar");add(3090,L"Cancelar tarefa");}
    else if(view==PersonalView::Remember){add(3096,L"Guardar");add(3090,L"Agora não");}
    else if(view==PersonalView::SkillReview){
        if(personal.skillDraft.contains("steps")){int i=0;for(auto& step:personal.skillDraft["steps"]){out.body+=L"\n"+std::to_wstring(++i)+L". "+personalToolTitle(step.value("name",""));if(personal.inlineDetails)out.body+=L"\n"+hext::wide(step.value("arguments",hext::Json::object()).dump());}}
        add(3314,L"Adicionar receita");add(3440,personal.inlineDetails?L"Ocultar detalhes":L"Ver detalhes");add(3090,L"Cancelar");
    }else{
        auto id=personal.capability;
        if(id=="provider"){
            out.body+=std::wstring(L"\n")+assistantName()+L" · "+assistantConnectionMessage();add(3340,L"Codex");add(3341,L"Claude");add(3310,connectionState[assistantProvider]==hext::ConnectionState::NeedsLogin?L"Abrir login":L"Conectar e verificar");
            auto state=connectionState[assistantProvider];if(state==hext::ConnectionState::Missing||state==hext::ConnectionState::Error||state==hext::ConnectionState::Incompatible)add(466,L"Localizar executável");
        }else if(id=="spotify"){
            auto state=personal.spotify.snapshot();out.body+=L"\nSpotify · "+state.status;
            if(personal.spotifyPage==1){for(int i=0;i<static_cast<int>(state.tracks.size())&&i<5;i++){auto& track=state.tracks[i];out.buttons.emplace_back(3140+i,hext::wide(track.value("name","")+" · "+track.value("artist","")));out.buttons.emplace_back(3180+i,L"Abrir no Spotify");out.targets[3140+i]=out.targets[3180+i]=track.value("uri","");}}
            else if(personal.spotifyPage==2){for(int i=0;i<4&&personal.listPage*4+i<static_cast<int>(state.devices.size());i++){auto& device=state.devices[personal.listPage*4+i];out.buttons.emplace_back(3150+i,hext::wide(device.value("name","")+(device.value("restricted",true)?" · Restrito":"")));out.targets[3150+i]=device.value("id","");}if(state.devices.size()>4){add(3098,L"Anterior");add(3099,L"Próximos");out.body+=L"\nPágina "+std::to_wstring(personal.listPage+1)+L" de "+std::to_wstring((state.devices.size()+3)/4);}}
            else{
                if(personal.setupReason=="desktop-player")add(3317,L"Abrir Spotify");
                add(3310,hext::SpotifyService::validClient(personal.spotify.clientID())?state.credentials?L"Verificar conexão":L"Abrir login":L"Configurar conexão");
                add(3080,L"Editar Client ID");add(3316,L"Configuração oficial");
            }
        }else{
            out.body+=L"\n"+hext::wide(personalCapabilityState(id).value("status",""));
            auto adapter=personal.adapters.find(id);if(adapter!=personal.adapters.end()&&adapter->second.publicConfiguration&&adapter->second.configure)add(3318,L"Configurar");
            add(3310,id=="files"?L"Escolher pasta":id=="applications"?L"Localizar aplicativo":id=="packages"?L"Abrir Microsoft Store":L"Verificar e continuar");
        }
        add(3090,L"Cancelar tarefa");
    }
    if(!personal.notice.empty())out.body+=L"\n"+personal.notice.substr(0,400);
    return out;
}
float drawChatCard(const ChatMessage& message,float y,bool measure){
    auto content=personalChatContent(message);IDWriteTextLayout* body=nullptr;DWRITE_TEXT_METRICS metrics{};
    if(SUCCEEDED(textFactory->CreateTextLayout(content.body.c_str(),static_cast<UINT32>(content.body.size()),font(12,false),464,100000,&body))){body->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);body->GetMetrics(&metrics);}
    int columns=content.buttons.size()==3?3:2,rows=static_cast<int>((content.buttons.size()+columns-1)/columns);
    float cardHeight=18+22+metrics.height+(rows?12+rows*30+(rows-1)*7:25)+12,total=cardHeight+30;
    if(!measure){
        float visibility=content.live?unit(personal.inlineReveal.value):1;
        D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);canvas->SetTransform(D2D1::Matrix3x2F::Translation(0,5*(1-visibility))*original);
        canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(204,129,712,351),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),visibility),nullptr);
        text(L"Hext",212,y,476,11,0xB7B7C1,true);float top=y+20;
        box(212,top,492,cardHeight,0xFFFFFF,12,content.live?.065f:.04f);
        text(content.title,226,top+12,464,13,0xEEEEF4,true);
        if(body){use(0xC6C6D0);canvas->DrawTextLayout(D2D1::Point2F(226,top+38),body,brush);}
        float buttonY=top+38+metrics.height+12;size_t firstHit=hits.size();
        for(size_t i=0;i<content.buttons.size();i++){
            auto& item=content.buttons[i];float w=(464-(columns-1)*8.f)/columns;
            bool providerChoice=item.first==3340||item.first==3341;
            bool selected=providerChoice&&assistantProvider==(item.first==3340?1:2);
            int id=personalChatButton(item.first,content.targets[item.first]);if(id>=0)button(id,item.second,226+(i%columns)*(w+8),buttonY+(i/columns)*37,w,30,!providerChoice&&(i==0||item.first==3310),selected);
        }
        // Draw clipping does not clip hit testing. Partially hidden buttons must not receive input.
        hits.erase(std::remove_if(hits.begin()+firstHit,hits.end(),[](const Hit& hit){return hit.rect.top<129||hit.rect.bottom>351;}),hits.end());
        if(content.buttons.empty())text(content.state,226,buttonY-3,464,11,0xB7B7C1);
        canvas->PopLayer();canvas->SetTransform(original);
    }
    release(body);return total;
}
