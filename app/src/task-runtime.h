#pragma once
bool personalRegisterAdapter(hext::CapabilityDefinition capability,std::vector<hext::ToolDefinition> tools,NativeCapabilityAdapter adapter){
    if(personal.task.active()||assistantBridge.busy||!hext::skillName(capability.id)||hext::capabilityDefinition(capability.id)||static_cast<int>(capability.category)<0||capability.category>=hext::Category::Count||!adapter.execute||tools.empty())return false;
    auto original=hext::nativeToolRegistry();hext::nativeCapabilities().push_back(capability);
    try{for(auto& tool:tools){if(tool.capability!=capability.id)throw std::runtime_error("Wrong capability binding");hext::nativeToolRegistry().add(std::move(tool));}personal.adapters.emplace(capability.id,std::move(adapter));return true;}
    catch(...){hext::nativeToolRegistry()=std::move(original);hext::nativeCapabilities().pop_back();return false;}
}
void personalSuspendRejected(const hext::ActionRequest& request,const std::string& capability,const std::string& reason){
    if(capability=="spotify"&&reason=="desktop-player")personal.nativeSpotifyReady=false;
    personal.task.observed.erase(request.name+":"+request.args.dump());personal.task.inFlight.reset();personal.inFlight.reset();personal.task.suspend(request,capability);personal.capability=capability;personal.setupReason=reason;personal.preparing=false;personal.requestActive=true;personalShow(PersonalView::Setup);assistantStatus=L"O pedido está aguardando este recurso. Nenhuma repetição é necessária.";
}
hext::Json personalAsset(const std::string& id){
    auto asset=personal.store.get("asset",id);if(asset.is_object())return asset;
    auto transient=personal.resources.find(id);if(transient!=personal.resources.end()){
        auto record=transient->second;auto parent=record.value("parent","");
        for(int depth=0;!parent.empty()&&depth<8;depth++){auto root=personal.store.get("asset",parent);if(root.is_object()){if(root.value("app",false))return nullptr;parent.clear();break;}auto next=personal.resources.find(parent);if(next==personal.resources.end())return nullptr;parent=next->second.value("parent","");}
        return parent.empty()?record:hext::Json(nullptr);
    }
    if(auto discovered=personal.applications.find(id))return *discovered;
    return nullptr;
}
hext::Json personalCapabilityState(const std::string& id){
    using J=hext::Json;auto definition=hext::capabilityDefinition(id);J result={{"id",id},{"available",definition!=nullptr},{"ready",definition!=nullptr},{"state","ready"},{"status","Disponível"}};
    if(definition){result["title"]=hext::utf8(definition->title);result["description"]=definition->description;}
    auto native=personal.adapters.find(id);if(native!=personal.adapters.end()&&native->second.inspect){try{auto status=native->second.inspect();if(status.is_object()){result["ready"]=status.value("ready",false);result["state"]=status.value("state","configuration");result["status"]=status.value("status","Preparação necessária");result["details"]=status.value("details","Configuração oficial deste recurso.");}else throw std::runtime_error("Invalid adapter status");}catch(...){result["ready"]=false;result["state"]="error";result["status"]="Não foi possível consultar este recurso.";}return result;}
    if(id=="provider")return {{"id",id},{"available",true},{"ready",assistantConfigured()&&(assistantProvider==1||assistantProvider==2)},{"state",assistantConfigured()?"ready":"configuration"},{"status",hext::utf8(assistantConnectionMessage())}};
    if(id=="desktop"){result["status"]=std::to_string(personal.windowScopes.size())+" janelas autorizadas nesta conversa";return result;}
    if(id=="terminal"){result["status"]="Terminal isolado · sem rede";return result;}
    if(id=="web"){result["ready"]=std::filesystem::exists(hext::systemExecutable(L"curl.exe"));result["status"]=result["ready"]==true?"Pesquisa e texto HTTPS públicos":"curl do Windows indisponível";return result;}
    if(id=="packages"){wchar_t local[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);result["ready"]=personal.packagesVerified||std::filesystem::exists(std::filesystem::path(local)/L"Microsoft/WindowsApps/winget.exe");result["state"]=result["ready"]==true?"ready":"configuration";result["status"]=result["ready"]==true?"App Installer disponível · validação antes do uso":"Instale o App Installer oficial";return result;}
    if(!definition){result["state"]="unsupported";result["status"]="Capacidade não instalada";return result;}
    if(id=="spotify"){
        auto state=personal.spotify.snapshot();result["ready"]=state.connected||personal.nativeSpotifyReady;result["status"]=personal.nativeSpotifyReady&&!state.connected?"Player Spotify verificado no Windows":hext::utf8(state.status);result["routes"]={{"windows_player",personal.nativeSpotifyReady?"verified":"inspect_on_demand"},{"web_api",state.connected?"connected":"requires_setup"}};
        result["state"]=state.busy?"checking":state.connected?"ready":!hext::SpotifyService::validClient(personal.spotify.clientID())?"configuration":state.credentials?"verification":"authentication";
    }else if(id=="files"){bool authorized=personal.store.count("asset")>0||!personal.resources.empty();result["ready"]=authorized;result["state"]=authorized?"ready":"authorization";result["status"]=authorized?"Itens autorizados disponíveis":"Escolha um arquivo ou uma pasta";}
    else if(id=="applications"){result["state"]=personal.applications.busy()?"checking":"ready";result["status"]=personal.applications.busy()?"Verificando aplicativos…":personal.applications.size()==0?"Descoberta disponível":"Aplicativos verificados";}
    else if(id=="windows.audio"){result["status"]=audioReady?"Saída de áudio disponível":"Verifique os dispositivos de áudio";}
    else if(id=="windows.media")result["status"]="Depende de um player compatível";
    else if(id=="hext.skills")result["status"]=std::to_string(personal.skills.list().size())+" skills revisáveis";
    return result;
}
hext::Json personalCapabilitiesContext(){hext::Json result=hext::Json::array();for(auto& definition:hext::capabilityDefinitions())result.push_back(personalCapabilityState(definition.id));return result;}
bool personalRequireSetup(const hext::ActionRequest& request){
    if(personalAgentSetup(request))return true;
    if(request.name.rfind("packages.",0)==0&&!personalCapabilityState("packages").value("ready",false)){personal.task.suspend(request,"packages");personal.capability="packages";personal.requestActive=true;personalShow(PersonalView::Setup);return true;}
    auto definition=hext::toolRegistry().find(request.name);
    if(definition&&personal.adapters.count(definition->capability)&&!personalCapabilityState(definition->capability).value("ready",false)){personal.task.suspend(request,definition->capability);personal.capability=definition->capability;personal.requestActive=true;personalShow(PersonalView::Setup);return true;}
    if(request.name=="capabilities.connect"){
        auto state=personalCapabilityState(request.args["id"]);if(state.value("available",false)&&!state.value("ready",false)){personal.task.suspend(request,request.args["id"]);personal.capability=request.args["id"];personal.requestActive=true;personalShow(PersonalView::Setup);return true;}return false;
    }
    if(request.category!=hext::Category::Spotify||personal.spotify.snapshot().connected)return false;
    if(request.name=="spotify.resume"||request.name=="spotify.pause"||request.name=="spotify.next"||request.name=="spotify.previous")return false;
    personal.task.suspend(request,"spotify");personal.capability="spotify";personal.setupReason="authentication";personal.requestActive=true;personal.task.setupConsent=false;personalShow(PersonalView::Setup);assistantStatus=L"Conecte o recurso para continuar o mesmo pedido.";return true;
}
void personalResumeTask(){
    if(!personal.task.managed||personal.task.chat!=personal.chat||personal.task.stage!=hext::TaskStage::Setup)return;
    auto pending=personal.task.waiting;auto id=personal.task.capability;
    if(pending&&pending->nativeTarget=="spotify"&&!personal.nativeSpotifyReady)return;
    if(id=="spotify"&&pending&&(pending->name=="spotify.play"||pending->name=="spotify.search"||pending->name=="spotify.set_volume")&&!personal.spotify.snapshot().connected)return;
    if(!personalCapabilityState(id).value("ready",false))return;
    personalResolveInline("completed");personal.task.waiting.reset();personal.task.stage=hext::TaskStage::Executing;personal.preparing=false;personal.setupReason.clear();personal.view=PersonalView::None;personal.requestActive=true;
    if(pending){personal.queued.insert(personal.queued.begin(),*pending);personalNext();}else if(id=="provider")personalRunProvider();
    invalidate(180);
}
void personalPrepare(){
    auto id=personal.capability;
    if(testMode){personal.notice=L"Fluxos externos não são iniciados pela verificação visual.";return;}
    if(personal.task.stage==hext::TaskStage::Setup)personal.task.setupConsent=true;
    auto native=personal.adapters.find(id);if(native!=personal.adapters.end()){
        if(native->second.prepare)native->second.prepare();else if(native->second.publicConfiguration&&native->second.configure){auto field=native->second.publicConfiguration();personalEditor("resource-config",field.first,hext::wide(field.second),id);}else personal.notice=personalCapabilityState(id).value("ready",false)?L"Recurso disponível.":L"Este recurso ainda não está configurado.";return;
    }
    if(id=="spotify"){
        if(personal.spotify.busy())return;
        personal.task.setupConsent=personal.task.stage==hext::TaskStage::Setup;
        if(!hext::SpotifyService::validClient(personal.spotify.clientID())){personalEditor("spotify-client",L"Conectar recurso · Client ID");return;}
        if(personal.setupReason=="device"){personal.spotifyPage=2;personal.preparing=personal.task.stage==hext::TaskStage::Setup;personal.spotify.command("devices");return;}
        personal.preparing=personal.task.stage==hext::TaskStage::Setup;personal.setupReason="verification";
        if(personal.spotify.snapshot().credentials)personal.spotify.command("verify");else personal.spotify.connect(windowHandle);
    }else if(id=="files"){auto selected=personalChooseAsset(false,true);if(!selected.empty())personalResumeTask();}
    else if(id=="applications"){auto selected=personalChooseAsset(true);if(!selected.empty()){if(personal.task.waiting&&personal.task.waiting->name=="apps.open")personal.task.waiting->args["id"]=selected;personalResumeTask();}}
    else if(id=="windows.media"){if(personal.task.stage==hext::TaskStage::Setup){personal.preparing=personal.media.start("inspect");}}
    else if(id=="windows.audio"){ShellExecuteW(windowHandle,L"open",L"ms-settings:sound",nullptr,nullptr,SW_SHOWNORMAL);connectAudio();}
    else if(id=="browser")ShellExecuteW(windowHandle,L"open",L"ms-settings:defaultapps",nullptr,nullptr,SW_SHOWNORMAL);
    else if(id=="provider"){
        if(!assistantProvider||assistantProvider==3){assistantProvider=1;detectAssistant(1);}
        if(connectionState[assistantProvider]==hext::ConnectionState::NeedsLogin)loginAssistant();else connectAssistant();
    }else if(id=="packages"){
        ShellExecuteW(windowHandle,L"open",L"ms-windows-store://pdp/?productid=9NBLGGH4NNS1",nullptr,nullptr,SW_SHOWNORMAL);
        if(personal.task.stage==hext::TaskStage::Setup){personal.preparing=true;personal.nativePrepareUntil=GetTickCount64()+180000;personal.nativeRetryAt=0;}
    }
    else personal.notice=L"Este recurso não exige uma conexão externa.";
    invalidate(180);
}
bool personalResourceAction(const hext::ActionRequest& request){
    using J=hext::Json;auto& a=request.args;auto name=request.name;
    try{
        if(name=="skills.import"){
            personal.capability="hext.skills";personalImportSkill();
            if(personal.skillDraft.is_object()){personal.task.stage=hext::TaskStage::Choice;personal.requestActive=true;}
            else personalReceipt(request,false,L"Seleção cancelada ou receita inválida.");
            return true;
        }
        if(name=="connections.disconnect"){
            auto id=a.value("id","");auto adapter=personal.adapters.find(id);
            if(id=="spotify"){personal.spotify.disconnect();personal.nativeSpotifyReady=false;personalReceipt(request,true,L"Conexão local do Spotify removida. Você pode revogar o acesso também na conta do serviço.");}
            else if(adapter!=personal.adapters.end()&&adapter->second.disconnect){adapter->second.disconnect();personalReceipt(request,true,L"Conexão removida.");}
            else personalReceipt(request,false,L"Não há uma conexão removível com esse identificador.");
            return true;
        }
        auto definition=hext::toolRegistry().find(name);auto native=definition?personal.adapters.find(definition->capability):personal.adapters.end();if(native!=personal.adapters.end()){if(!native->second.execute(request))personalReceipt(request,false,L"O adaptador não conseguiu executar esta ferramenta.");return true;}
        if(name=="apps.find"){
            if(testMode&&!personalNativeVerification){personalReceipt(request,false,L"Descoberta real desativada nesta verificação visual.");return true;}
            if(personal.applications.start(a["query"])){personal.applicationsOwned=true;assistantStatus=L"Verificando os aplicativos instalados…";}else personalReceipt(request,false,L"A descoberta de aplicativos está ocupada.");return true;
        }
        if(name=="apps.open"&&!personal.store.get("asset",a["id"]).is_object()){
            auto discovered=personal.applications.find(a["id"]);if(!discovered){personalReceipt(request,false,L"O aplicativo não foi descoberto ou autorizado.");return true;}
            auto path=std::filesystem::path(hext::wide((*discovered)["path"]));if(!hext::discoverableApplication(path)||std::filesystem::last_write_time(path).time_since_epoch().count()!=(*discovered)["stamp"].get<long long>()){personalReceipt(request,false,L"O aplicativo mudou. Verifique-o novamente.");return true;}
            if(personal.store.count("asset")>=100){personalReceipt(request,false,L"Limite de itens autorizados atingido.");return true;}personal.store.put("asset",a["id"],*discovered);return false;
        }
        if(name=="windows.open_settings"){
            if(testMode){personalReceipt(request,false,L"Abertura externa desativada nesta verificação.");return true;}
            const std::map<std::string,std::wstring> pages={{"sound",L"ms-settings:sound"},{"bluetooth",L"ms-settings:bluetooth"},{"display",L"ms-settings:display"},{"apps",L"ms-settings:appsfeatures"},{"privacy",L"ms-settings:privacy"},{"windows_update",L"ms-settings:windowsupdate"}};
            auto code=reinterpret_cast<INT_PTR>(ShellExecuteW(windowHandle,L"open",pages.at(a["page"]).c_str(),nullptr,nullptr,SW_SHOWNORMAL));personalReceipt(request,code>32,code>32?L"Página oficial de configurações aberta. Nenhum ajuste foi alterado automaticamente.":L"O Windows não abriu esta página.");return true;
        }
        if(name=="capabilities.inspect"){auto state=personalCapabilityState(a["id"]);personalReceipt(request,state.value("available",false),hext::wide(state.value("status","")),state);return true;}
        if(name=="capabilities.connect"){
            auto state=personalCapabilityState(a["id"]);if(!state.value("available",false)){personalReceipt(request,false,L"Não há um adaptador confiável instalado para este recurso. Nenhum componente foi baixado.");return true;}
            if(state.value("ready",false)){personalReceipt(request,true,L"O recurso já está disponível.",state);return true;}
            personal.task.suspend(request,a["id"]);personal.capability=a["id"];personalShow(PersonalView::Setup);personal.requestActive=true;return true;
        }
        if(name=="files.choose"){auto id=personalChooseAsset(false,a["kind"]=="folder");personalReceipt(request,!id.empty(),id.empty()?L"Seleção cancelada; nenhum novo item foi autorizado.":L"Item autorizado para a tarefa.",id.empty()?J():J{{"id",id},{"name",personalAsset(id).value("name","")}});return true;}
        if(name=="files.list"||name=="files.read_text"||name=="files.create_text"){
            auto id=a[name=="files.create_text"?"folder":"id"].get<std::string>();auto asset=personalAsset(id);
            if(!asset.is_object()||asset.value("app",false)||!hext::textField(asset,"path",32768))throw std::runtime_error("Unauthorized item");
            auto path=std::filesystem::path(hext::wide(asset["path"]));if(!hext::safeAsset(path,false)||std::filesystem::canonical(path)!=path)throw std::runtime_error("Changed item");
            if(name=="files.read_text"){auto text=hext::readAuthorizedText(path);personalReceipt(request,text.has_value(),text?(text->size()>12000?L"Trecho do texto autorizado lido; o retorno foi limitado a 12 KB.":L"Texto autorizado lido para esta tarefa."):L"O arquivo precisa ser UTF-8, de um tipo de texto permitido e ter até 32 KB.",text?J{{"id",id},{"text",hext::utf8Excerpt(*text,0,12000)},{"truncated",text->size()>12000},{"bytes",text->size()}}:J());}
            else if(name=="files.create_text"){
                if(personal.resources.size()>=100){personalReceipt(request,false,L"Limite de itens temporários atingido. Inicie uma nova conversa para continuar.");return true;}
                auto created=hext::createAuthorizedText(path,a["name"],a["text"]);if(!created){personalReceipt(request,false,L"Não foi possível confirmar a criação. Verifique o destino; arquivos existentes nunca são sobrescritos.");return true;}
                auto child=hext::randomID();personal.resources[child]={{"path",hext::utf8(created->wstring())},{"name",a["name"]},{"app",false},{"parent",id}};personalReceipt(request,true,L"Novo arquivo criado e conteúdo verificado.",J{{"id",child},{"name",a["name"]},{"verified",true}});
            }else{
                if(!std::filesystem::is_directory(path))throw std::runtime_error("Folder required");
                J items=J::array();std::error_code error;int visited=0;bool limited=false;
                for(auto it=std::filesystem::directory_iterator(path,std::filesystem::directory_options::skip_permission_denied,error);it!=std::filesystem::directory_iterator()&&!error&&visited++<500;it.increment(error)){
                    if(items.size()>=40||personal.resources.size()>=100){limited=true;break;}
                    auto childPath=it->path();if(!hext::safeAsset(childPath,false)||std::filesystem::canonical(childPath)!=childPath)continue;
                    auto child=hext::randomID();personal.resources[child]={{"path",hext::utf8(childPath.wstring())},{"name",hext::utf8(childPath.filename().wstring())},{"app",false},{"parent",id}};
                    items.push_back({{"id",child},{"name",hext::utf8(childPath.filename().wstring())},{"folder",it->is_directory()}});
                }if(error)personalReceipt(request,false,L"O Windows não permitiu concluir a consulta à pasta.");else personalReceipt(request,true,limited||visited>=500?L"Lista parcial da pasta autorizada consultada.":L"Itens da pasta autorizada consultados.",J{{"items",items},{"truncated",limited||visited>=500},{"limit",40}});
            }return true;
        }
    }catch(...){personalReceipt(request,false,L"O recurso não está autorizado, mudou ou não pôde concluir esta etapa.");return true;}
    return false;
}
void personalImportSkill(){
    if(testMode)return;
    wchar_t selected[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};dialog.hwndOwner=windowHandle;dialog.lpstrFile=selected;dialog.nMaxFile=32768;dialog.lpstrFilter=L"Hext Skill (*.json)\0*.json\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;menuOpen=true;auto chosen=GetOpenFileNameW(&dialog);menuOpen=false;if(!chosen)return;
    if(std::filesystem::file_size(selected)>32768){personal.notice=L"O arquivo de skill deve ter até 32 KB.";return;}
    std::ifstream file(selected,std::ios::binary);std::string contents((std::istreambuf_iterator<char>(file)),{});auto recipe=hext::parseJSON(contents,32768);
    if(!recipe||!hext::SkillRegistry::valid(*recipe)){personal.notice=L"Skill inválida. Somente receitas declarativas de ferramentas conhecidas são aceitas.";return;}
    personal.skillDraft=*recipe;personalShow(PersonalView::SkillReview);
}
void personalTaskAction(int id){
    if(id>=3400){personalAgentUIAction(id);return;}
    if(id==3310)personalPrepare();
    else if(id==3313)personalImportSkill();
    else if(id==3314){if(personal.skillDraft.is_object()&&personal.skills.add(personal.skillDraft)){personal.store.put("skill",personal.skillDraft["id"],{{"recipe",personal.skillDraft}});personalResolveInline("completed");personal.skillDraft=nullptr;personal.view=PersonalView::None;personal.notice=L"Receita adicionada.";if(personal.inFlight&&personal.inFlight->name=="skills.import"){auto request=*personal.inFlight;personal.task.stage=hext::TaskStage::Executing;personalReceipt(request,true,L"Receita verificada e adicionada.");personalNext();}}else personal.notice=L"Não foi possível adicionar esta skill.";}
    else if(id==3315){personal.capability=personal.task.capability;personalShow(PersonalView::Setup);personalResumeTask();}
    else if(id==3316){if(!testMode&&personal.capability=="spotify")ShellExecuteW(windowHandle,L"open",L"https://developer.spotify.com/dashboard",nullptr,nullptr,SW_SHOWNORMAL);}
    else if(id==3318){auto native=personal.adapters.find(personal.capability);if(native!=personal.adapters.end()&&native->second.publicConfiguration&&native->second.configure){auto field=native->second.publicConfiguration();personalEditor("resource-config",field.first,hext::wide(field.second),personal.capability);}}
    else if(id==3319){auto native=personal.adapters.find(personal.capability);if(native!=personal.adapters.end()&&native->second.disconnect){personalRevoke();native->second.disconnect();personal.notice=L"Recurso desconectado.";}}
    else if(id==3317){if(!testMode&&personal.capability=="spotify"){
        auto opened=reinterpret_cast<INT_PTR>(ShellExecuteW(windowHandle,L"open",L"spotify:",nullptr,nullptr,SW_SHOWNORMAL));if(opened<=32)personal.notice=L"O Spotify não está registrado para abertura. Abra-o manualmente ou use a conexão oficial.";
        else if(personal.task.stage==hext::TaskStage::Setup&&personal.setupReason=="desktop-player"){personal.preparing=true;personal.nativePrepareUntil=GetTickCount64()+8000;personal.nativeRetryAt=0;}
    }}
    else if(id==3340||id==3341){assistantProvider=id==3340?1:2;detectAssistant(assistantProvider);personalPrepare();}
    invalidate(180);
}
