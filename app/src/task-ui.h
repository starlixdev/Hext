#pragma once
void drawPersonalSpotify();
std::wstring personalToolTitle(const std::string& name){
    const std::map<std::string,std::wstring> names={{"audio.set_volume",L"Alterar volume"},{"audio.set_mute",L"Silenciar áudio"},{"audio.list_devices",L"Consultar dispositivos"},{"audio.set_default_device",L"Trocar saída padrão"},{"timer.start",L"Iniciar temporizador"},{"timer.pause",L"Pausar temporizador"},{"browser.search",L"Pesquisar no navegador"},{"apps.find",L"Encontrar aplicativos"},{"apps.open",L"Abrir aplicativos"},{"windows.open_settings",L"Abrir ajustes do Windows"},{"files.choose",L"Escolher um item"},{"files.open",L"Abrir item autorizado"},{"files.list",L"Consultar pasta"},{"files.read_text",L"Ler texto autorizado"},{"files.create_text",L"Criar novo texto"},{"spotify.search",L"Pesquisar músicas"},{"spotify.play",L"Escolher e reproduzir"},{"spotify.pause",L"Pausar reprodução"},{"spotify.resume",L"Continuar reprodução"},{"spotify.next",L"Próxima faixa"},{"spotify.previous",L"Faixa anterior"},{"spotify.set_volume",L"Alterar volume"},{"media.control",L"Controlar player atual"},{"history.search",L"Consultar histórico"},{"capabilities.inspect",L"Verificar recursos"},{"capabilities.connect",L"Preparar conexão"},{"skills.run",L"Usar skills revisadas"}};
    auto found=names.find(name);if(found!=names.end())return found->second;
    const std::map<std::string,std::wstring> general={{"system.inspect",L"Verificar ambiente"},{"terminal.run",L"Executar no terminal isolado"},{"web.search",L"Pesquisar informações"},{"web.read",L"Consultar página pública"},{"computer.windows",L"Consultar janelas"},{"computer.inspect",L"Observar controles"},{"computer.act",L"Interagir com controles"},{"computer.wait",L"Aguardar configuração"},{"packages.search",L"Encontrar programas"},{"packages.inspect",L"Revisar um pacote"},{"packages.install",L"Instalar versão aprovada"}};auto title=general.find(name);return title==general.end()?hext::wide(name):title->second;
}
void drawConnectionDetail(){
    auto id=personal.capability;auto state=personalCapabilityState(id);auto definition=hext::capabilityDefinition(id);
    if(id=="spotify"){drawPersonalSpotify();return;}
    box(204,168,508,92,0xFFFFFF,13,.045f);text(definition?definition->title:id=="provider"?L"Assistente":L"Configuração",220,184,476,16,0xF0F0F5,true);personalParagraph(hext::wide(state.value("status","")),220,218,476,38,12);
    if(id=="provider"){
        personalParagraph(L"Conecte um provedor para continuar o pedido. A autenticação é feita pelo aplicativo oficial.",220,282,476,58,13);
        button(3340,L"Codex",204,354,244,34,false,assistantProvider==1);button(3341,L"Claude",460,354,252,34,false,assistantProvider==2);button(3310,connectionState[assistantProvider]==hext::ConnectionState::NeedsLogin?L"Entrar e continuar":L"Verificar e continuar",204,414,300,32,true);
        if(connectionState[assistantProvider]==hext::ConnectionState::Missing||connectionState[assistantProvider]==hext::ConnectionState::Error||connectionState[assistantProvider]==hext::ConnectionState::Incompatible)button(466,L"Localizar…",516,414,196,32);
    }else if(id=="hext.skills"){
        personalParagraph(L"Escolha uma receita local. O Hext verifica o arquivo antes de adicioná-lo e aplica o modo de permissão em cada etapa.",220,282,476,90,13);button(3313,L"Escolher receita…",204,414,244,32,true);
    }else{
        personalParagraph(id=="files"?L"Selecione o arquivo ou a pasta necessária para este pedido.":id=="applications"?L"Localize o aplicativo para continuar. O Hext verificará o destino antes de abrir.":id=="packages"?L"Instale ou repare o App Installer pela Microsoft Store. O Hext verifica sua disponibilidade para continuar o pedido.":id=="windows.media"?L"Abra um player compatível e verifique novamente para continuar.":personal.adapters.count(id)?hext::wide(state.value("details","Conclua a configuração oficial para continuar.")):L"Conclua a configuração necessária no Windows para continuar este pedido.",220,282,476,92,13);
        auto adapter=personal.adapters.find(id);if(adapter!=personal.adapters.end()&&adapter->second.publicConfiguration&&adapter->second.configure)button(3318,L"Configurar…",204,374,244,30);
        button(3310,id=="files"?L"Escolher pasta…":id=="applications"?L"Localizar aplicativo…":id=="packages"?L"Abrir Microsoft Store":L"Verificar e continuar",204,414,300,32,true);
    }
}
void drawTaskOverlay(){
    auto view=personal.view;bool setup=view==PersonalView::Setup;auto definition=hext::capabilityDefinition(personal.capability);
    text(view==PersonalView::SkillReview?L"Adicionar receita":setup?L"Continuar seu pedido":definition?definition->title:L"Conectar",204,87,454,21,0xF5F5F8,true);iconButton(3191,0xE711,setup?L"Cancelar tarefa":L"Fechar painel",676,87,30,30);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,125,722,470),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),unit(personal.transition.value)),nullptr);
    if(view==PersonalView::SkillReview){
        text(hext::wide(personal.skillDraft.value("title","")),220,170,476,18,0xEEEEF4,true);personalParagraph(L"Esta receita combina ferramentas conhecidas. Suas etapas continuam sujeitas ao modo de permissão escolhido.",220,214,476,65,13);
        if(personal.skillDraft.contains("steps")){int row=0;for(auto& step:personal.skillDraft["steps"]){if(row>=4)break;text(std::to_wstring(row+1)+L". "+personalToolTitle(step.value("name","")),220,297+row*25.f,476,12,0xC9C9D3);row++;}}
        button(3090,L"Cancelar",204,414,244,35);button(3314,L"Adicionar receita",460,414,252,35,true);
    }else{if(setup)text(hext::wide(personal.task.goal),212,132,492,11,0xB7B7C1);drawConnectionDetail();}
    if(!personal.notice.empty())text(personal.notice,212,450,492,10,0xD9D9E2);
    canvas->PopLayer();
}