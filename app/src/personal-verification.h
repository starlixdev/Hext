#pragma once
int verifyPersonal(const std::filesystem::path& directory,bool provider,bool native){
    std::ofstream report(directory/L"personal-verification.txt");int failures=0;
    auto check=[&](bool ok,const char* description){report<<(ok?"PASS: ":"FAIL: ")<<description<<"\n";report.flush();if(!ok)failures++;};
    personalVerification=true;personalProviderVerification=provider;
    check(personal.ready,"private encrypted database opened in a separate fixture directory");if(!personal.ready)return 1;
    expanded=true;width.snap(736);height.snap(470);reveal.snap(1);appearance.snap(1);pageFade.snap(1);settingsFade.snap(1);
    page=previousPage=0;personal.preferences=hext::AssistantPreferences{};personal.view=PersonalView::None;
    auto timer=hext::actionRequest(hext::Json{{"name","timer.start"},{"arguments",{{"minutes",2}}}});
    timerRunning=false;
    personal.preferences.autonomy=hext::Autonomy::Chat;personal.queued={*timer};personalNext();check(!timerRunning&&!personal.approval,"chat-only denies an otherwise valid timer action");
    personal.preferences.autonomy=hext::Autonomy::Ask;personal.queued={*timer};personalNext();check(personal.approval&&!timerRunning,"ask mode stages a concrete action without executing it");
    personal.transition.snap(1);render();check(exportFrame(directory/L"personal-approval.bmp"),"export native authorization view");
    personalRevoke();personalAction(3095);check(!timerRunning,"revoked approval cannot execute through the UI action handler");
    personal.queued={*timer};personalNext();personalAction(3095);check(timerRunning&&remaining()>119000&&remaining()<=120000,"UI approval executes the real Hext timer service");
    timerRunning=false;timerRemaining=timerDuration=300000;timerSymbol.snap(0);personal.view=PersonalView::None;
    conversation.clear();personalAppend(true,L"Conversa A · fixture privada");personalSaveChat();auto first=personal.chat;check(personal.saved&&personal.store.get("chat",first).is_object(),"save-current action persists the conversation");
    personalNew(false);personalAppend(true,L"Conversa B · contexto separado");personalSaveChat();auto second=personal.chat;
    personalOpenChat(first);check(conversation.size()==1&&conversation[0].text.find(L"Conversa A")!=std::wstring::npos,"reopening chat A does not import chat B context");
    personalEditor("rename",L"Renomear",L"Conversa renomeada",first);personalCommitEditor();check(personal.store.get("chat",first).value("title","")=="Conversa renomeada","rename editor updates saved history");
    personal.memory.remember("response_style","brief-private-marker",true,first,true);personal.memory.remember("inferred_fixture","discard-derived",false,second,false);
    personalOpenChat(second);auto prompt=personalPrompt();check(prompt.find(L"brief-private-marker")!=std::wstring::npos,"shared explicit memory is available in a different conversation");
    personal.preferences.memory=false;check(personalPrompt().find(L"brief-private-marker")==std::wstring::npos,"disabled memory is absent from provider context");personal.preferences.memory=true;
    personal.editorID="chat:"+second;personalAction(3097);check(personal.store.get("chat",second).is_null()&&personal.memory.list().size()==1,"deleting chat removes derived memory and preserves pinned preferences");
    personal.preferences.autonomy=hext::Autonomy::Allowed;personal.preferences.grants[0]=hext::Grant::Deny;personalSavePreferences();auto saved=personal.store.get("settings","assistant");check(saved["grants"][0]==0,"individual permission changes are persisted");
    personal.preferences=hext::AssistantPreferences{};personalSavePreferences();
    page=previousPage=5;settingsSection=3;settingsFade.snap(1);personal.view=PersonalView::None;
    for(int tab=0;tab<5;tab++){
        personal.settingsTab=tab;personal.transition.snap(1);render();std::set<int> ids;bool valid=true;
        for(auto& hit:hits)valid=valid&&!hit.name.empty()&&hit.rect.left>=0&&hit.rect.right<=736&&hit.rect.top>=0&&hit.rect.bottom<=470&&ids.insert(hit.id).second;
        check(valid&&SUCCEEDED(lastRenderResult),"assistant settings controls are named, unique and within island bounds");
        check(exportFrame(directory/(L"personal-settings-"+std::to_wstring(tab)+L".bmp")),"export native assistant settings page");
    }
    personalRefreshRows(PersonalView::Chats);personalShow(PersonalView::Chats);personal.transition.snap(1);render();check(exportFrame(directory/L"personal-chats.bmp"),"export saved chat management");
    personalRefreshRows(PersonalView::Memory);personalShow(PersonalView::Memory);personal.transition.snap(1);render();check(exportFrame(directory/L"personal-memory.bmp"),"export memory provenance and management");
    personalEditor("memory",L"Editar preferência · nome: valor",L"response_style: brief-private-marker",personal.rows.front().value("id",""));personal.transition.snap(1);render();check(exportFrame(directory/L"personal-editor.bmp"),"export inline editor using the existing native text input");personalCloseView();personal.view=PersonalView::None;
    if(provider){
        assistantProvider=1;detectAssistant(1);connectAssistant();auto until=GetTickCount64()+60000;
        while(connectionState[1]==hext::ConnectionState::Checking&&GetTickCount64()<until){pollAssistantConnection();Sleep(20);}
        check(assistantConfigured()&&(connectionChecks[1]&8),"real Codex version, options, authentication and restricted Windows sandbox verified");
        if(assistantConfigured()){
            page=previousPage=0;conversation.clear();personal.saved=false;personal.chat=hext::randomID();personal.preferences=hext::AssistantPreferences{};
            assistantInput=L"Inicie um temporizador de 2 minutos. Use uma ação timer.start e não proponha memória.";personalSend();until=GetTickCount64()+150000;
            while(assistantBridge.busy&&GetTickCount64()<until){if(assistantBridge.poll())personalReply();Sleep(30);}
            std::ofstream debug(directory/L"personal-provider-debug.json",std::ios::binary);
            debug<<hext::Json{{"exit",assistantBridge.exitCode},{"status",hext::utf8(assistantStatus)},{"response",assistantBridge.result.substr(0,24000)},{"diagnostic",assistantBridge.diagnostic.substr(0,8000)}}.dump(2);debug.close();
            check(personal.approval&&personal.approval->request.name=="timer.start"&&personal.approval->request.args.value("minutes",0)==2&&!timerRunning,"natural-language request through real Codex stages the correct action and waits for permission");
            if(personal.approval&&personal.approval->request.name=="timer.start"){personalAction(3095);check(timerRunning,"approving the provider-requested action starts the real timer");timerRunning=false;timerSymbol.snap(0);}
        }
    }
    if(native){
        personalNativeVerification=true;hext::SoundEngine probe;auto hr=probe.refresh();
        if(SUCCEEDED(hr)&&probe.reading[0].ready){
            auto original=probe.reading[0];int percent=static_cast<int>(std::lround(original.volume*100));
            personal.preferences=hext::AssistantPreferences{};personal.preferences.autonomy=hext::Autonomy::Allowed;personal.preferences.grants[0]=hext::Grant::Allow;
            // Set mute to its existing value: verifies the native setter without interrupting sound.
            auto sameMute=hext::actionRequest(hext::Json{{"name","audio.set_mute"},{"arguments",{{"muted",original.muted}}}});personalExecute(*sameMute);probe.read(0);
            check(probe.reading[0].muted==original.muted&&conversation.back().text.find(L"Falhou")==std::wstring::npos,"real action executor writes and reads back the existing Windows mute state");
            if(std::abs(original.volume-percent/100.f)<.00001f){auto sameVolume=hext::actionRequest(hext::Json{{"name","audio.set_volume"},{"arguments",{{"percent",percent}}}});personal.queued={*sameVolume};personalNext();if(personal.approval)personalAction(3095);probe.read(0);check(std::abs(probe.reading[0].volume-original.volume)<.00001f,"real volume action is verified without changing the user's volume");}
            personal.preferences.grants[0]=hext::Grant::Deny;personalExecute(*sameMute);check(conversation.back().text.find(L"Falhou")!=std::wstring::npos,"denied native action is refused before reaching Core Audio");
        }else report<<"UNVERIFIED: no active Core Audio output device\n";
        personal.media.start("inspect");auto until=GetTickCount64()+8000;std::optional<hext::MediaResult> media;
        while(GetTickCount64()<until){media=personal.media.poll();if(media)break;Sleep(20);}
        check(media.has_value(),"real Windows media-session request completes on its worker");
        if(media)report<<"Media session available: "<<media->session<<"; HRESULT: "<<media->code<<". Playback was not changed.\n";
        auto child=directory/L"native-action-child.exe";
        if(std::filesystem::exists(child)){
            auto fixture=directory/(L"native-launch-"+hext::wide(hext::randomID()));std::filesystem::create_directories(fixture);auto executable=fixture/L"authorized.exe",marker=fixture/L"native-action-observed.txt";
            std::filesystem::copy_file(child,executable);auto canonical=std::filesystem::canonical(executable);auto id=hext::randomID();
            personal.store.put("asset",id,{{"path",hext::utf8(canonical.wstring())},{"app",true},{"stamp",std::filesystem::last_write_time(canonical).time_since_epoch().count()}});
            auto launch=hext::actionRequest(hext::Json{{"name","apps.open"},{"arguments",{{"id",id}}}});
            personal.preferences.autonomy=hext::Autonomy::Allowed;personal.preferences.grants[static_cast<int>(hext::Category::Apps)]=hext::Grant::Deny;personalExecute(*launch);
            check(!std::filesystem::exists(marker),"denied application action does not launch the registered fixture executable");
            personal.preferences.grants[static_cast<int>(hext::Category::Apps)]=hext::Grant::Allow;personalExecute(*launch);until=GetTickCount64()+5000;
            while(!std::filesystem::exists(marker)&&GetTickCount64()<until)Sleep(20);
            check(std::filesystem::exists(marker),"allowed application action launches a real registered native executable and observes its marker");
            personal.store.erase("asset",id);personalExecute(*launch);check(conversation.back().text.find(L"Falhou")!=std::wstring::npos,"revoking an authorized path blocks subsequent launches");
        }else report<<"UNVERIFIED: native-action-child.exe not built; real application launch test skipped\n";
        personal.preferences.autonomy=hext::Autonomy::Allowed;personal.preferences.grants[static_cast<int>(hext::Category::Browser)]=hext::Grant::Allow;
        auto browser=hext::actionRequest(hext::Json{{"name","browser.search"},{"arguments",{{"query","Microsoft Windows desktop documentation"}}}});personalExecute(*browser);
        check(conversation.back().text.find(L"Pesquisa enviada")!=std::wstring::npos,"real browser search launch accepted by Windows (page load/content not inspected)");
    }
    personal.editorID="all-data";personalAction(3097);check(personal.store.count("chat")==0&&personal.store.count("memory")==0&&personal.store.count("asset")==0&&personal.store.count("credential")==0&&conversation.empty(),"delete-all handler removes local chats, memory, assets, credentials and visible conversation state");
    personalRevoke();report<<"Failures: "<<failures<<"\n";
    report<<"Spotify playback and OAuth account approval require a configured Spotify application and user account; no simulated account or playback is used.\n";
    return failures?1:0;
}
