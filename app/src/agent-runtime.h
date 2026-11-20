#pragma once
std::filesystem::path personalExecutable(){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);return path;}
std::wstring personalAgentLabel(const hext::ActionRequest& request){
    auto& a=request.args;auto name=request.name;
    if(name=="terminal.run")return L"Executar PowerShell isolado, sem rede ou pastas pessoais:\n"+hext::wide(a.value("purpose",""));
    if(name=="web.read")return L"Consultar página HTTPS pública:\n"+hext::wide(a.value("url",""));
    if(name=="web.search")return L"Pesquisar na web (consulta enviada ao Bing):\n"+hext::wide(a.value("query",""));
    if(name=="computer.windows")return L"Consultar títulos das janelas disponíveis. Nenhuma janela será autorizada automaticamente.";
    if(name=="computer.inspect"||name=="computer.wait"||name=="computer.act"){
        auto window=personal.windows.find(a.value("window",""));auto title=window==personal.windows.end()?L"janela indisponível":hext::wide(window->second.value("title",""));
        if(name=="computer.act"){
            std::string label=a.value("element","");auto snapshot=personal.snapshots.find(a.value("snapshot",""));if(snapshot!=personal.snapshots.end())for(auto& element:snapshot->second["elements"])if(element.value("element","")==label){label=element.value("name",label);break;}
            return L"Interagir com "+title+L"\n"+hext::wide(a.value("operation",""))+L": "+hext::wide(label)+(a.value("text","").empty()?L"":L"\nTexto: "+hext::wide(a.value("text","")));
        }return (name=="computer.wait"?L"Aguardar sua participação em ":L"Ler controles de ")+title;
    }
    if(name=="packages.install"){auto proposal=personal.proposals.find(a.value("proposal",""));if(proposal==personal.proposals.end())return L"Proposta de instalação indisponível.";return L"Instalar "+hext::wide(proposal->second.value("id",""))+L" · "+hext::wide(proposal->second.value("version",""))+L"\nFonte comunitária oficial do WinGet · somente este usuário. O instalador é código externo; revise os detalhes.";}
    if(name.rfind("packages.",0)==0)return L"Consultar pacote no WinGet: "+hext::wide(a.value("id",a.value("query","")))+L" "+hext::wide(a.value("version",""));
    if(name=="system.inspect")return L"Verificar Windows e dependências disponíveis.";
    return {};
}
bool personalCheckpoint(){
    if(!personal.ready||!personal.task.managed)return true;
    try{
        if(!personal.task.active()&&personal.task.stage!=hext::TaskStage::Paused&&personal.task.stage!=hext::TaskStage::Blocked){if(!personal.checkpointDigest.empty())personal.store.erase("task",personal.task.chat);personal.checkpointDigest.clear();return true;}
        hext::Json messages=hext::Json::array();size_t bytes=0;std::vector<hext::Json> recent;
        for(auto it=conversation.rbegin();it!=conversation.rend()&&recent.size()<30;++it){auto value=hext::utf8(it->text);if(bytes+value.size()>100000)break;bytes+=value.size();recent.push_back({{"user",it->user},{"tool",it->tool},{"model_only",it->modelOnly},{"text",value}});}std::reverse(recent.begin(),recent.end());for(auto& message:recent)messages.push_back(message);
        hext::Json resources=hext::Json::object();for(auto& resource:personal.resources)resources[resource.first]=resource.second;
        hext::Json record={{"task",personal.task.checkpoint()},{"chat",personal.chat},{"title",personal.title},{"messages",messages},{"resources",resources}};auto digest=record.dump();
        if(digest==personal.checkpointDigest)return true;
        if(personal.store.get("task",personal.chat).is_null()&&personal.store.count("task")>=20)throw std::runtime_error("Pending task limit");
        personal.store.put("task",personal.chat,record);personal.checkpointDigest=std::move(digest);personal.checkpointFailed=false;return true;
    }catch(...){personal.checkpointFailed=true;assistantStatus=L"Não foi possível guardar a tarefa. A execução foi interrompida antes de novas ações.";return false;}
}
void personalPauseTask(){
    if(!personal.task.managed||(!personal.task.active()&&personal.task.stage!=hext::TaskStage::Blocked&&personal.task.stage!=hext::TaskStage::Paused))return;
    personal.task.stage=hext::TaskStage::Paused;
    if(personalCheckpoint())personal.preserveCheckpoint=true;
}
void personalRestoreTask(const std::string& id){
    auto record=personal.store.get("task",id);hext::TaskSession restored;
    if(!record.is_object()||!record.contains("task")||!restored.restore(record["task"])||restored.chat!=id||record.value("chat","")!=id||!record.contains("messages")||!record["messages"].is_array()||record["messages"].size()>30)throw std::runtime_error("Invalid task checkpoint");
    std::vector<ChatMessage> messages;for(auto& row:record["messages"]){if(!hext::textField(row,"text",100000,true))throw std::runtime_error("Invalid task context");messages.push_back({row.value("user",false),hext::wide(row["text"]),row.value("tool",false),row.value("model_only",false)});}
    personalPauseTask();personalRevoke();personal.chat=id;personal.title=record.value("title","Tarefa retomada");personal.saved=personal.store.get("chat",id).is_object();personal.dirty=true;conversation=std::move(messages);personal.task=std::move(restored);personal.resources.clear();
    if(record.contains("resources")&&record["resources"].is_object()&&record["resources"].size()<=100)for(auto it=record["resources"].begin();it!=record["resources"].end();++it)if(it.key().size()<=64&&it.value().is_object())personal.resources[it.key()]=it.value();
    // Approval, UI window handles and authentication state never survive restart.
    personal.gate.revoke();personal.approval.reset();personal.queued.clear();personal.windows.clear();personal.windowScopes.clear();personal.snapshots.clear();personal.task.resumePlanning();personal.task.plan();personal.requestChat=personal.chat;personal.requestActive=true;personal.followUp=false;personal.view=PersonalView::None;personal.checkpointDigest.clear();
    personalAppend(false,L"Tarefa retomada. Autorizações antigas foram descartadas; etapas interrompidas serão verificadas antes de continuar.",true);
    if(!assistantConfigured()){personal.task.stage=hext::TaskStage::Setup;personal.task.capability=personal.capability="provider";personalShow(PersonalView::Setup);}else personalRunProvider();
}
void personalAgentCleanup(){
    personal.agent.cancel();personal.agentOwned=false;personal.windows.clear();personal.snapshots.clear();personal.windowScopes.clear();personal.proposals.clear();personal.pendingWindow.clear();
}
bool personalDeleteAgentWorkspaces(){
    if(personal.agent.busy())return false;
    for(auto& row:personal.store.list("workspace",200)){auto id=row.value("container","");if(!hext::agentID(id)||FAILED(DeleteAppContainerProfile((L"Hext.Task."+hext::wide(id)).c_str())))return false;personal.store.erase("workspace",row.at("id"));}return true;
}
bool personalAgentSetup(const hext::ActionRequest& request){
    if(request.name!="computer.inspect"&&request.name!="computer.act"&&request.name!="computer.wait")return false;
    auto id=request.args.value("window","");auto found=personal.windows.find(id);
    if(found==personal.windows.end()||!hext::validAgentWindow(found->second))return false; // Executor reports a recoverable stale ID.
    if(personal.windowScopes.count(id))return false;
    if(personal.preferences.mode!=hext::PermissionMode::Ask&&!hext::protectedUIContext(found->second.value("title","")))return false;
    personal.pendingWindow=id;personal.task.suspend(request,"desktop");personal.capability="desktop";personal.requestActive=true;personalShow(PersonalView::WindowConsent);return true;
}
bool personalAgentAction(const hext::ActionRequest& request){
    using J=hext::Json;auto name=request.name;auto args=request.args;
    bool ours=name=="system.inspect"||name.rfind("terminal.",0)==0||name.rfind("web.",0)==0||name.rfind("computer.",0)==0||name.rfind("packages.",0)==0;if(!ours)return false;
    if(testMode&&!personalNativeVerification){personalReceipt(request,false,L"Esta verificação não executa ferramentas gerais no computador.");return true;}
    if(personal.agent.busy()){personalReceipt(request,false,L"O executor está finalizando uma operação anterior.");return true;}
    if(name=="system.inspect"){
        SYSTEM_INFO system{};GetNativeSystemInfo(&system);personalReceipt(request,true,L"Ambiente consultado sem ler documentos pessoais.",J{{"architecture",system.wProcessorArchitecture},{"processors",system.dwNumberOfProcessors},{"powershell",std::filesystem::exists(hext::systemExecutable(L"WindowsPowerShell/v1.0/powershell.exe"))},{"curl",std::filesystem::exists(hext::systemExecutable(L"curl.exe"))},{"ui_automation","available_for_authorized_unelevated_windows"},{"terminal","AppContainer prepared on first permitted use; no network capabilities"},{"packages","Requires signed Microsoft App Installer and official WinGet source"}});return true;
    }
    if(name=="computer.windows"){
        auto rows=hext::agentWindows();J output=J::array();for(auto it=personal.windows.begin();it!=personal.windows.end();)if(!personal.windowScopes.count(it->first))it=personal.windows.erase(it);else ++it;
        for(auto& row:rows){auto id=row["id"].get<std::string>();personal.windows[id]=row;output.push_back({{"id",id},{"title",row["title"]},{"application",row["application"]}});}personalReceipt(request,true,L"Janelas disponíveis consultadas. As próximas ações seguem o modo de permissão escolhido.",output);return true;
    }
    J desktop=nullptr;std::filesystem::path executable=personalExecutable();
    if(name.rfind("computer.",0)==0){
        auto id=args.value("window","");auto window=personal.windows.find(id);if(window!=personal.windows.end()&&!personal.windowScopes.count(id)&&personal.preferences.mode!=hext::PermissionMode::Ask&&!hext::protectedUIContext(window->second.value("title","")))personal.windowScopes.insert(id);
        if(window==personal.windows.end()||!personal.windowScopes.count(id)||!hext::validAgentWindow(window->second)){personalReceipt(request,false,L"Janela não autorizada, alterada ou encerrada. Consulte as janelas novamente.",J{{"recoverable",true}});return true;}
        desktop={{"binding",window->second},{"operation","inspect"}};
        if(name=="computer.act"){
            auto snapshot=personal.snapshots.find(args.value("snapshot",""));
            if(snapshot==personal.snapshots.end()||snapshot->second.value("window","")!=id||GetTickCount64()>snapshot->second.value("expires",0ULL)){personalReceipt(request,false,L"A observação expirou. Leia os controles novamente antes de agir.",J{{"recoverable",true}});return true;}
            J expected=nullptr;for(auto& element:snapshot->second["elements"])if(element.value("element","")==args.value("element",""))expected=element;
            if(!expected.is_object()){personalReceipt(request,false,L"O controle não existe na observação autorizada.",J{{"recoverable",true}});return true;}
            desktop["operation"]=args["operation"];desktop["text"]=args["text"];desktop["element"]=args["element"];desktop["expected"]=expected;
        }
    }
    if(name=="packages.install"){
        auto found=personal.proposals.find(args.value("proposal",""));if(found==personal.proposals.end()){personalReceipt(request,false,L"A proposta expirou. Consulte o pacote novamente.");return true;}args=found->second;
    }
    if(name=="terminal.run"){
        if(!hext::agentID(personal.chat)){personalReceipt(request,false,L"Conversa incompatível com o ambiente isolado.");return true;}
        if(personal.store.get("workspace",personal.chat).is_null()&&personal.store.count("workspace")>=20){personalReceipt(request,false,L"O limite de 20 ambientes foi atingido. Limpe os ambientes anteriores nas configurações.");return true;}
        personal.store.put("workspace",personal.chat,{{"container",personal.chat}});
    }
    auto chat=personal.chat;
    auto operation=[name,args,desktop,executable,chat](std::atomic<bool>& cancel)->J{
        if(name=="terminal.run"){auto value=hext::sandboxPowerShell(chat,args.at("script"),cancel);return {{"success",value.code==0&&!value.truncated&&!value.timedOut&&!value.cancelled},{"exit_code",value.code},{"stdout",hext::utf8Excerpt(value.output,0,20000)},{"stderr",hext::utf8Excerpt(value.error,0,6000)},{"isolated",true},{"network_capabilities",false},{"timed_out",value.timedOut},{"truncated",value.truncated},{"untrusted",true},{"uncertain",value.timedOut||value.cancelled},{"recoverable",true}};}
        if(name=="web.read")return hext::agentReadWeb(args.at("url"),cancel);
        if(name=="web.search")return hext::agentReadWeb("https://www.bing.com/search?format=rss&q="+hext::urlEncode(args.at("query")),cancel);
        if(name.rfind("packages.",0)==0)return hext::agentPackages(name,args,cancel);
        if(name=="computer.wait"){
            auto until=GetTickCount64()+120000;auto wanted=hext::lower(args.value("expected_text",""));
            while(!cancel&&GetTickCount64()<until){auto observed=hext::agentDesktopCall(executable,desktop,cancel);if(!observed.value("success",false))return observed;
                bool match=hext::lower(observed.value("window_title","")).find(wanted)!=std::string::npos;
                for(auto& element:observed["elements"])if(hext::lower(element.value("name","")+" "+element.value("value","")).find(wanted)!=std::string::npos)match=true;
                if(match)return J{{"success",true},{"condition_observed",true},{"note","Expected accessibility text is visible; this does not grant broader permissions or prove account/API access."}};
                for(int i=0;i<10&&!cancel;i++)Sleep(100);
            }return J{{"success",false},{"error","The expected visible state was not observed. The task can be paused and resumed without repeating the original request."},{"recoverable",true}};
        }
        return hext::agentDesktopCall(executable,desktop,cancel);
    };
    if(personal.agent.start(operation)){personal.agentOwned=true;personal.requestActive=true;assistantStatus=L"Em execução: "+personalAgentLabel(request);if(name=="computer.wait")personalShow(PersonalView::Intervention);}
    else personalReceipt(request,false,L"Não foi possível iniciar o executor nativo.");
    return true;
}
void personalAgentPoll(){
    if(personal.preparing&&personal.task.stage==hext::TaskStage::Setup&&personal.task.capability=="packages"&&!personal.agent.busy()&&GetTickCount64()>=personal.nativeRetryAt&&GetTickCount64()<personal.nativePrepareUntil){personal.agent.start([](std::atomic<bool>& cancel)->hext::Json{return {{"setup_probe","packages"},{"ready",!cancel&&!hext::verifiedWinGet().empty()}};});personal.nativeRetryAt=GetTickCount64()+5000;}
    if(personal.preparing&&personal.task.stage==hext::TaskStage::Setup&&personal.task.capability=="packages"&&GetTickCount64()>=personal.nativePrepareUntil){personal.preparing=false;personal.notice=L"Conclua a instalação pelo aplicativo oficial e verifique novamente. O pedido continua guardado.";}
    if(auto completed=personal.agent.poll()){
        if(completed->value("setup_probe","")=="packages"){
            if(personal.preparing&&personal.task.stage==hext::TaskStage::Setup&&personal.task.capability=="packages"&&completed->value("ready",false)){personal.packagesVerified=true;personal.preparing=false;personalResumeTask();}
            return;
        }
        if(personal.agentOwned&&personal.inFlight&&personal.requestChat==personal.chat){auto request=*personal.inFlight;personal.agentOwned=false;auto result=*completed;bool success=result.value("success",false);
            if(!success&&result.value("requires_setup","")=="packages"){personal.packagesVerified=false;personalSuspendRejected(request,"packages","dependency");return;}
            if(success&&request.name.rfind("packages.",0)==0)personal.packagesVerified=true;
            if(request.name=="computer.inspect"&&success){if(personal.snapshots.size()>=8)personal.snapshots.erase(personal.snapshots.begin());auto id=hext::randomID();personal.snapshots[id]={{"window",request.args["window"]},{"elements",result["elements"]},{"expires",GetTickCount64()+180000}};result["snapshot"]=id;}
            if(request.name=="packages.inspect"&&success){if(personal.proposals.size()>=8)personal.proposals.erase(personal.proposals.begin());personal.proposals[result["proposal"]]=result;}
            bool recoverable=!success&&result.value("recoverable",false)&&!result.value("uncertain",false);
            if(personal.view==PersonalView::Intervention)personal.view=PersonalView::None;
            personalReceipt(request,success,success?request.name=="computer.act"&&!result.value("verified",false)?L"A interface aceitou a ação. O resultado final ainda precisa ser observado.":L"Etapa nativa concluída; resultado disponível para verificação.":hext::wide(result.value("error","A etapa falhou. Consulte o resultado antes de tentar outra abordagem.")),result);
            if(recoverable&&personal.task.managed&&!personal.preferences.blocked){personal.task.stage=hext::TaskStage::Executing;personal.followUp=true;}
            personalNext();
        }
    }
    auto now=GetTickCount64();if(now-personal.lastCheckpoint>500){personal.lastCheckpoint=now;if(!personalCheckpoint()){personalPauseTask();personal.agent.cancel();personal.agentOwned=false;assistantBridge.cancel();personal.requestActive=false;personal.task.stage=hext::TaskStage::Blocked;}}
}
std::wstring personalOperationDetails(){
    if(!personal.approval)return {};
    auto& request=personal.approval->request;auto text=personalAgentLabel(request);
    if(request.name=="terminal.run")text+=L"\n\n"+hext::wide(request.args.value("script",""));
    if(request.name=="packages.install"){auto found=personal.proposals.find(request.args.value("proposal",""));if(found!=personal.proposals.end())text+=L"\n\n"+hext::wide(found->second.value("metadata",""));}
    return text;
}
void personalAgentUIAction(int id){
    if(id==3420){personal.rows=personal.store.list("task",20);personalShow(PersonalView::Tasks);}
    else if(id>=3430&&id<3434){int index=personal.listPage*4+id-3430;if(index<static_cast<int>(personal.rows.size()))personalRestoreTask(personal.rows[index].value("id",""));}
    else if(id==3423){personalPauseTask();personalRevoke();personal.view=PersonalView::None;assistantStatus=L"Tarefa pausada. Você pode retomá-la em Tarefas pendentes.";}
    else if(id==3440){personal.reviewPage=0;personalShow(PersonalView::OperationReview);}
    else if(id==3441)personal.reviewPage=std::max(0,personal.reviewPage-1);
    else if(id==3442)personal.reviewPage=std::min(static_cast<int>(personalOperationDetails().size()/650),personal.reviewPage+1);
    else if(id==3443)personalShow(PersonalView::Action);
    else if(id==3450){
        auto found=personal.windows.find(personal.pendingWindow);if(found==personal.windows.end()||!hext::validAgentWindow(found->second)){personal.notice=L"A janela mudou ou foi fechada.";return;}
        if(personal.windowScopes.size()>=8){personal.notice=L"Revogue janelas anteriores antes de autorizar outra.";return;}
        personal.windowScopes.insert(personal.pendingWindow);auto pending=personal.task.waiting;personal.task.waiting.reset();personal.task.stage=hext::TaskStage::Executing;personal.view=PersonalView::None;
        if(pending){auto request=personalRefineAction(*pending);auto ticket=personal.gate.propose(personal.preferences,request,personal.chat);auto approved=personal.gate.consume(ticket.nonce,personal.preferences,personal.chat);if(approved)personalExecute(*approved,hext::ExecutionAuthority::User);personalNext();}
    }
    else if(id==3460){personalRevoke();personal.notice=L"Autorizações de janelas revogadas.";}
    else if(id==3470){personal.editorID="agent-data";personalShow(PersonalView::Delete);}
    else if(id>=3480&&id<3484){int index=personal.listPage*4+id-3480;if(index<static_cast<int>(personal.rows.size())){personal.editorID="task:"+personal.rows[index].value("id","");personalShow(PersonalView::Delete);}}
    invalidate(180);
}
