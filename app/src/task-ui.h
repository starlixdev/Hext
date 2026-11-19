#pragma once
void drawPersonalSpotify();
std::wstring personalGrantLabel(const std::map<std::string,hext::Grant>& grants,const std::string& key){auto found=grants.find(key);return found==grants.end()?L"Padrão":found->second==hext::Grant::Deny?L"Negar":found->second==hext::Grant::Ask?L"Perguntar":L"Permitir";}
std::wstring personalToolTitle(const std::string& name){
    const std::map<std::string,std::wstring> names={{"audio.set_volume",L"Alterar volume"},{"audio.set_mute",L"Silenciar áudio"},{"audio.list_devices",L"Consultar dispositivos"},{"audio.set_default_device",L"Trocar saída padrão"},{"timer.start",L"Iniciar temporizador"},{"timer.pause",L"Pausar temporizador"},{"browser.search",L"Pesquisar no navegador"},{"apps.find",L"Encontrar aplicativos"},{"apps.open",L"Abrir aplicativos"},{"windows.open_settings",L"Abrir ajustes do Windows"},{"files.choose",L"Escolher um item"},{"files.open",L"Abrir item autorizado"},{"files.list",L"Consultar pasta"},{"files.read_text",L"Ler texto autorizado"},{"files.create_text",L"Criar novo texto"},{"spotify.search",L"Pesquisar músicas"},{"spotify.play",L"Escolher e reproduzir"},{"spotify.pause",L"Pausar reprodução"},{"spotify.resume",L"Continuar reprodução"},{"spotify.next",L"Próxima faixa"},{"spotify.previous",L"Faixa anterior"},{"spotify.set_volume",L"Alterar volume"},{"media.control",L"Controlar player atual"},{"history.search",L"Consultar histórico"},{"capabilities.inspect",L"Verificar recursos"},{"capabilities.connect",L"Preparar conexão"},{"skills.run",L"Usar skills revisadas"}};
    auto found=names.find(name);if(found!=names.end())return found->second;
    const std::map<std::string,std::wstring> general={{"system.inspect",L"Verificar ambiente"},{"terminal.run",L"Executar no terminal isolado"},{"web.search",L"Pesquisar informações"},{"web.read",L"Consultar página pública"},{"computer.windows",L"Consultar janelas"},{"computer.inspect",L"Observar controles"},{"computer.act",L"Interagir com controles"},{"computer.wait",L"Aguardar configuração"},{"packages.search",L"Encontrar programas"},{"packages.inspect",L"Revisar um pacote"},{"packages.install",L"Instalar versão aprovada"}};auto title=general.find(name);return title==general.end()?hext::wide(name):title->second;
}
void drawCapabilityList(float top=168){
    text(L"Integrações e capacidades",220,top+3,476,16,0xF0F0F5,true);
    for(int row=0;row<4;row++){int index=personal.capabilityPage*4+row;if(index>=static_cast<int>(hext::capabilityDefinitions().size()))break;auto& definition=hext::capabilityDefinitions()[index];auto state=personalCapabilityState(definition.id);float y=top+39+row*45.f;
        button(3300+row,L"",204,y,508,40);alignedText(definition.title,220,y+1,302,23,13,0xEEEEF4,true,false);alignedText(hext::wide(state.value("status","")),220,y+22,454,16,10,0xB7B7C1,false,false);symbol(0xE76C,682,y+12,13,0xB7B7C1);hits.back().name=definition.title;
    }
    button(3304,L"Anterior",204,414,120,30);button(3305,L"Próximos",336,414,120,30);alignedText(std::to_wstring((hext::capabilityDefinitions().size()+3)/4)+L"/"+std::to_wstring(personal.capabilityPage+1),472,414,80,30,11,0x9A9AA6,false,false);
}
void drawCapabilityDetail(){
    auto id=personal.capability;auto state=personalCapabilityState(id);auto definition=hext::capabilityDefinition(id);
    if(personal.capabilityPermissions){
        text(L"Permissões deste recurso",220,173,476,15,0xEEEEF4,true);button(3320,L"Recurso: "+personalGrantLabel(personal.preferences.serviceGrants,id),204,211,508,32);
        std::vector<const hext::ToolDefinition*> tools;for(auto& tool:hext::toolRegistry().tools())if(tool.capability==id)tools.push_back(&tool);int maximum=std::max(0,(static_cast<int>(tools.size())-1)/4);personal.capabilityToolPage=std::clamp(personal.capabilityToolPage,0,maximum);
        for(int row=0;row<4;row++){int index=personal.capabilityToolPage*4+row;if(index>=static_cast<int>(tools.size()))break;auto& tool=*tools[index];float y=254+row*35.f;alignedText(personalToolTitle(tool.name),218,y,302,30,12,0xDADAE4,false,false);button(3330+row,personalGrantLabel(personal.preferences.toolGrants,tool.name),540,y,172,30);}
        button(3311,L"Voltar",204,414,120,30);if(maximum){button(3321,L"Anterior",336,414,120,30);button(3322,L"Próximos",468,414,120,30);}return;
    }
    if(id=="spotify"){drawPersonalSpotify();return;}
    box(204,168,508,92,0xFFFFFF,13,.045f);text(definition?definition->title:id=="provider"?L"Assistente":L"Recurso indisponível",220,184,476,16,0xF0F0F5,true);personalParagraph(hext::wide(state.value("status","")),220,218,476,38,12);
    if(id=="provider"){
        personalParagraph(L"Conecte um provedor para continuar este pedido. A entrada é feita pelo aplicativo oficial.",220,282,476,58,13);
        button(3340,L"Codex",204,354,244,34,false,assistantProvider==1);button(3341,L"Claude",460,354,252,34,false,assistantProvider==2);button(3310,connectionState[assistantProvider]==hext::ConnectionState::NeedsLogin?L"Entrar e continuar":L"Verificar e continuar",204,414,300,32,true);
        if(connectionState[assistantProvider]==hext::ConnectionState::Missing||connectionState[assistantProvider]==hext::ConnectionState::Error||connectionState[assistantProvider]==hext::ConnectionState::Incompatible)button(466,L"Localizar…",516,414,196,32);
    }else if(id=="hext.skills"){
        text(L"Receitas com ferramentas do Hext",220,283,476,13,0xDDDDE6,true);int row=0;for(auto& recipe:personal.skills.list()){if(row>=3)break;text(hext::wide(recipe.value("title","")),220,315+row*27.f,476,12,0xB7B7C1);row++;}
        button(3313,L"Adicionar skill…",204,414,244,32,true);button(3311,L"Permissões",552,414,160,32);
    }else if(id=="desktop"||id=="terminal"||id=="web"||id=="packages"){
        personalParagraph(id=="desktop"?L"Autorize cada janela durante a conversa. Campos de senha e janelas protegidas são excluídos. Cada interação exige consentimento próprio.":id=="terminal"?L"Scripts gerais em um AppContainer exclusivo, com tempo, memória e saída limitados. Sem rede, pastas pessoais concedidas ou execução irrestrita. Arquivos permanecem no ambiente da conversa.":id=="web"?L"Pesquisa e leitura de texto público. Consultas e URLs são enviados à origem indicada; não são usados cookies ou credenciais. Páginas não podem conceder permissões.":L"Pacotes da fonte comunitária oficial do WinGet. A instalação exige aprovação para um ID e versão específicos, usa o escopo deste usuário e mantém as verificações de segurança do Windows.",220,282,476,102,13);
        if(id=="desktop")button(3460,L"Revogar janelas",204,414,244,32);
        if(id=="terminal")button(3470,L"Limpar tarefas e ambientes",204,414,310,32);
        if(id=="packages")button(3310,L"App Installer oficial",204,414,244,32);
        button(3311,L"Permissões",552,414,160,32);
    }else{
        personalParagraph(id=="files"?L"Você escolhe quais itens podem ser usados. Leituras e criação de textos exigem confirmação; arquivos existentes não são sobrescritos.":id=="applications"?L"O Hext procura aplicativos registrados e atalhos locais. Descobrir um aplicativo não autoriza sua abertura.":id=="windows.media"?L"Abra um player compatível. Depois, verifique para continuar o pedido que está aguardando.":personal.adapters.count(id)?hext::wide(state.value("details","Configuração oficial deste recurso.")):L"Este recurso usa os controles nativos disponíveis no Windows e continua sujeito às suas permissões.",220,282,476,92,13);
        auto adapter=personal.adapters.find(id);if(adapter!=personal.adapters.end()&&adapter->second.publicConfiguration&&adapter->second.configure)button(3318,L"Configurar…",204,374,244,30);
        bool disconnect=adapter!=personal.adapters.end()&&adapter->second.disconnect;
        if(id=="files"||id=="applications"||id=="windows.media"||id=="windows.audio"||id=="browser"||personal.adapters.count(id))button(3310,id=="files"?L"Escolher pasta…":id=="applications"?L"Autorizar aplicativo…":id=="windows.media"?L"Verificar e continuar":personal.adapters.count(id)?L"Preparar":L"Abrir ajustes",204,414,disconnect?164:244,32,true);
        if(disconnect)button(3319,L"Desconectar",380,414,164,32);
        button(3311,L"Permissões",552,414,160,32);
    }
}
void drawTaskOverlay(){
    auto view=personal.view;auto definition=hext::capabilityDefinition(personal.capability);bool setup=view==PersonalView::Setup;
    std::wstring title=view==PersonalView::Capabilities?L"Recursos":view==PersonalView::SkillReview?L"Adicionar skill":setup?L"Continuar seu pedido":definition?definition->title:L"Assistente";
    text(title,204,87,454,21,0xF5F5F8,true);iconButton(3191,0xE711,setup?L"Cancelar tarefa":L"Fechar painel",676,87,30,30);
    canvas->PushLayer(D2D1::LayerParameters(D2D1::RectF(196,125,722,470),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::IdentityMatrix(),unit(personal.transition.value)),nullptr);
    if(view==PersonalView::Capabilities)drawCapabilityList(148);
    else if(view==PersonalView::SkillReview){
        text(hext::wide(personal.skillDraft.value("title","")),220,170,476,18,0xEEEEF4,true);personalParagraph(L"Esta receita usa apenas ferramentas conhecidas. Adicioná-la não autoriza automaticamente nenhuma de suas ações.",220,214,476,65,13);
        if(personal.skillDraft.contains("steps")){int row=0;for(auto& step:personal.skillDraft["steps"]){if(row>=4)break;text(std::to_wstring(row+1)+L". "+personalToolTitle(step.value("name","")),220,297+row*25.f,476,12,0xC9C9D3);row++;}}
        button(3090,L"Cancelar",204,414,244,35);button(3314,L"Adicionar skill",460,414,252,35,true);
    }else{
        if(setup)text(hext::wide(personal.task.goal),212,132,492,11,0xB7B7C1);else button(3312,L"Todos os recursos",204,129,220,28);
        drawCapabilityDetail();
    }
    if(!personal.notice.empty())text(personal.notice,212,450,492,10,0xD9D9E2);
    canvas->PopLayer();
}
