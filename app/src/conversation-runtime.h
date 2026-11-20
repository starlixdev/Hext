#pragma once
hext::Json personalConfigurationInfo(const std::string& id){
    using J=hext::Json;J info={{"id",id},{"state",personalCapabilityState(id)}};auto adapter=personal.adapters.find(id);
    if(adapter!=personal.adapters.end()){
        if(adapter->second.guidance)info["guidance"]=adapter->second.guidance();
        if(adapter->second.publicConfiguration){auto field=adapter->second.publicConfiguration();info["public_field"]={{"label",hext::utf8(field.first)},{"configured",!field.second.empty()},{"secret",false}};}
    }
    if(id=="spotify")info["guidance"]={{"reviewed","2026-11-20"},{"sources",J::array({"https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow","https://developer.spotify.com/documentation/web-api/concepts/apps","https://developer.spotify.com/documentation/web-api/concepts/quota-modes"})},{"public_field",{{"name","client_id"},{"format","32 hexadecimal characters"},{"configured",hext::SpotifyService::validClient(personal.spotify.clientID())},{"secret",false}}},{"facts",J::array({"Basic playback can use an actual Spotify Windows media session without a Web API Client ID; check it before suggesting API setup.","Named catalog search and Web API playback need a registered Spotify application and an eligible account/device. This Hext build has no bundled registered Client ID.","Desktop PKCE requires the application's public Client ID, not its Client Secret. Existing configured IDs can be reused; never search private files or copy another application's credentials.","The developer dashboard exposes the app Client ID in its settings. Hext's registered redirect must be http://127.0.0.1/callback. Account, Premium and development-mode restrictions may apply; consult current official sources."})},{"dashboard","https://developer.spotify.com/dashboard"}};
    return info;
}
hext::Json personalPendingContext(){
    using J=hext::Json;auto id=personal.continuation?personal.continuation->capability:personal.task.capability.empty()?personal.capability:personal.task.capability;
    auto pending=personal.continuation?personal.continuation->pending:personal.approval?std::optional<hext::ActionRequest>(personal.approval->request):personal.task.waiting;
    J result={{"original_goal",personal.task.goal},{"latest_clarification",personal.clarification},{"discussion_only",personal.discussionOnly},{"configuration",personalConfigurationInfo(id)}};
    if(pending)result["waiting_tool"]=pending->name;
    if(personal.inFlight)result["in_progress_tool"]=personal.inFlight->name;
    if(personal.resumeContext.is_object())result["restored_context"]=personal.resumeContext;
    return result;
}
void personalClarify(const std::wstring& value,bool automatic){
    if(!automatic&&personalResumeVisible())personalRestoreTask(personal.chat,false);
    if(!automatic){auto safe=hext::conversationalInput(hext::utf8(value));personalAppend(true,hext::wide(safe));personal.clarification=safe;}
    else personal.clarification="Explain what this pending step needs, why, and how to complete it. Check available simpler routes before asking for technical fields; consult authoritative sources when necessary.";
    personal.autoGuide=automatic;personal.clarificationQueued=true;
    if(assistantBridge.busy)assistantBridge.cancel();
    personalStartClarification();
}
void personalStartClarification(){
    if(!personal.clarificationQueued)return;
    if(!assistantConfigured()||(assistantProvider!=1&&assistantProvider!=2)){personal.clarificationQueued=false;assistantStatus=L"Sua mensagem foi mantida. Conecte o provedor para receber a orientação.";return;}
    bool active=personal.agentOwned||personal.spotifyOwned||personal.mediaOwned||personal.applicationsOwned||personal.preparing;
    personal.discussionOnly=active;personal.discussionTask=personal.task.id;personal.clarificationQueued=false;
    if(!active){
        if(!personal.continuation){PersonalState::WaitingContinuation held;held.stage=personal.task.stage;held.view=personal.inlineView;held.pending=personal.approval?std::optional<hext::ActionRequest>(personal.approval->request):personal.review?std::optional<hext::ActionRequest>(personal.review->request):personal.task.waiting;held.queue=std::move(personal.queued);held.capability=personal.task.capability.empty()?personal.capability:personal.task.capability;if(held.pending&&held.stage!=hext::TaskStage::Setup){auto tool=hext::toolRegistry().find(held.pending->name);if(tool)held.capability=tool->capability;}held.reason=personal.setupReason;held.card=personal.inlineID;held.disclosure=std::move(personal.disclosure);personal.disclosure.reset();personal.continuation=std::move(held);}
        personalCancelReview();personal.gate.revoke();personal.approval.reset();personalResolveInline("paused");personal.queued.clear();personal.task.waiting.reset();
        if(!personal.task.plan()){assistantStatus=L"Limite de planejamento atingido. Pause e retome para verificar a tarefa.";personalFinishClarification();return;}
    }
    personal.requestChat=personal.chat;personal.requestActive=true;personalRunProvider();
}
void personalFinishClarification(){
    personal.discussionOnly=false;
    if(!personal.continuation){personal.requestActive=personal.task.active();return;}
    auto held=std::move(*personal.continuation);personal.continuation.reset();personal.capability=held.capability;personal.task.capability=held.capability;personal.setupReason=held.reason;personal.queued=std::move(held.queue);personal.task.waiting=held.pending;personal.task.stage=held.stage;personal.disclosure=std::move(held.disclosure);personal.reuseCard=held.card;
    if(held.stage==hext::TaskStage::Setup){personalShow(PersonalView::Setup);personalResumeTask();}
    else if(held.stage==hext::TaskStage::Paused){personal.requestActive=false;personalCheckpoint();personalOfferResume();}
    else if(held.view==PersonalView::WindowConsent){personalShow(PersonalView::WindowConsent);}
    else if(held.stage==hext::TaskStage::Approval&&held.pending){personalRequestConsent(*held.pending);}
    else if(!personal.queued.empty()){personal.task.stage=hext::TaskStage::Executing;personalNext();}
    else if(held.stage==hext::TaskStage::Planning){personal.requestActive=true;personalRunProvider();}
    else{personal.task.stage=hext::TaskStage::Completed;personal.requestActive=false;}
    personal.reuseCard.clear();personalCheckpoint();
}
void personalQueueGuidance(){
    if(personal.continuation||personal.discussionOnly||!assistantConfigured()||personal.capability=="provider"||assistantBridge.busy)return;
    auto key=personal.task.id+":"+personal.capability+":"+personal.setupReason;
    if(key==personal.guidedSetup)return;
    personal.guidedSetup=key;personalClarify(L"",true);
}
bool personalConversationError(const std::wstring& message){
    if(personal.discussionOnly){personal.discussionOnly=false;personalAppend(false,message,true);personal.requestActive=personal.task.active();personalNext();return true;}
    if(personal.continuation){personalAppend(false,message,true);personalFinishClarification();return true;}
    return false;
}
bool personalConversationTool(const hext::ActionRequest& request){
    auto name=request.name;auto a=request.args;
    if(name=="capabilities.configure"){
        auto id=a.value("id","");auto value=a.value("value","");
        if(personal.autoGuide||!hext::suppliedPublicValue(personal.clarification,value)){personal.task.observed.erase(request.name+":"+request.args.dump());personalReceipt(request,false,L"O valor público precisa ter sido fornecido por você nesta etapa. Não inventei nem procurei credenciais privadas.");return true;}
        try{if(id=="spotify")personal.spotify.configure(value);else{auto adapter=personal.adapters.find(id);if(adapter==personal.adapters.end()||!adapter->second.publicConfiguration||!adapter->second.configure)throw std::runtime_error("No public configuration");adapter->second.configure(value);}personalReceipt(request,true,L"Configuração pública validada. A autenticação e a prontidão ainda serão verificadas.",{{"id",id},{"ready",personalCapabilityState(id).value("ready",false)}});}catch(...){personalReceipt(request,false,L"O valor não foi aceito pelo recurso. Explique o formato esperado e ajude o usuário a corrigir sem perder o pedido original.",{{"id",id},{"recoverable",true}});}return true;
    }
    if(name=="capabilities.check"){
        auto id=a.value("id","");if(id=="spotify"){
            if(personal.media.start("inspect","spotify")){personal.mediaOwned=true;personal.requestChat=personal.chat;assistantStatus=L"Verificando o player disponível…";}
            else personalReceipt(request,false,L"A verificação do player está ocupada.");
        }else{auto status=personalCapabilityState(id);personalReceipt(request,status.value("available",false),L"Prontidão atual consultada.",personalConfigurationInfo(id));}return true;
    }
    if(name=="browser.open_url"){
        if(testMode&&!personalProviderVerification){personalReceipt(request,false,L"Abertura externa não é executada pela verificação sem conta.");return true;}
        auto url=a.value("url","");if(personal.agent.start([url](std::atomic<bool>& cancel)->hext::Json{return {{"success",!cancel&&hext::publicURL(url).has_value()},{"recoverable",true}};})){personal.agentOwned=true;personal.requestChat=personal.chat;}else personalReceipt(request,false,L"A verificação do endereço está ocupada.");return true;
    }
    if(name=="task.control"){
        auto operation=a.value("operation","");if(operation=="cancel"){
            if(personal.autoGuide){personalReceipt(request,false,L"A orientação automática não pode cancelar seu pedido por conta própria.");return true;}personalCancelChat();return true;
        }
        if(operation=="amend"){
            auto instruction=a.value("instruction","");if(personal.autoGuide||!hext::suppliedPublicValue(personal.clarification,instruction)){personalReceipt(request,false,L"O ajuste precisa corresponder à sua mensagem atual.");return true;}
            personal.continuation.reset();personal.task.goal=(personal.task.goal+"\nUser clarification: "+instruction).substr(0,48000);personal.queued.clear();personal.task.stage=hext::TaskStage::Executing;personalReceipt(request,true,L"Pedido ajustado com sua orientação. As próximas ações ainda passam pelas permissões atuais.");return true;
        }
        personalReceipt(request,true,L"Continuação solicitada; a prontidão e as permissões serão verificadas novamente.");personal.followUp=false;if(personal.continuation&&personal.continuation->stage==hext::TaskStage::Paused){personal.continuation.reset();personal.task.resumePlanning();if(personal.task.plan())personalRunProvider();}else{personalFinishClarification();}return true;
    }
    if(name=="capabilities.connect"&&a.value("id","")=="spotify"&&hext::SpotifyService::validClient(personal.spotify.clientID())&&!personal.spotify.snapshot().connected){
        personal.capability="spotify";bool started=personal.spotify.snapshot().credentials?personal.spotify.command("verify"):personal.spotify.connect(windowHandle);
        if(started){personal.preparing=true;personal.task.stage=hext::TaskStage::Choice;personal.setupReason="authentication";personalShow(PersonalView::Setup);}else personalReceipt(request,false,L"Não foi possível iniciar a autenticação oficial.");return true;
    }
    return false;
}
