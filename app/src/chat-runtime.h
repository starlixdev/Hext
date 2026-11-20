#pragma once
bool personalInlineKind(PersonalView view){return view==PersonalView::Action||view==PersonalView::Setup||view==PersonalView::Connection||view==PersonalView::WindowConsent||view==PersonalView::Intervention||view==PersonalView::SkillReview||view==PersonalView::Remember;}
bool personalInteractionIs(PersonalView view){return personal.inlineView==view||personal.view==view;}
bool personalEditing(){return personal.inlineEditor||personal.view==PersonalView::Editor;}
bool personalResumeVisible(){return !personal.inlineID.empty()&&personal.inlineChat==personal.chat&&personal.inlineView==PersonalView::None&&std::any_of(conversation.begin(),conversation.end(),[](const ChatMessage& message){return message.card.is_object()&&message.card.value("id","")==personal.inlineID&&message.card.value("kind","")=="resume";});}
hext::Json personalMessageRecord(const ChatMessage& message,bool checkpoint){
    hext::Json row={{"text",hext::utf8(message.text)}};
    if(checkpoint){row["user"]=message.user;row["tool"]=message.tool;row["model_only"]=message.modelOnly;}
    else row["role"]=message.user?"user":message.tool?"tool":"assistant";
    if(message.card.is_object()&&!message.modelOnly)row["card"]=message.card;
    return row;
}
ChatMessage personalRestoreMessage(const hext::Json& row,bool checkpoint){
    if(!hext::textField(row,"text",checkpoint?100000:48000,true))throw std::runtime_error("Invalid conversation text");
    auto role=row.value("role","assistant");if(!checkpoint&&role!="user"&&role!="assistant"&&role!="tool")throw std::runtime_error("Invalid conversation role");
    ChatMessage message{checkpoint?row.value("user",false):role=="user",hext::wide(row["text"]),checkpoint?row.value("tool",false):role=="tool",checkpoint&&row.value("model_only",false)};
    // Model replies never use this path. Restored presentation metadata cannot create a live ticket.
    if(!message.user&&!message.modelOnly&&row.contains("card"))message.card=hext::restoreChatCard(row["card"]);
    return message;
}
void personalResolveInline(const std::string& state){
    if(!personal.inlineID.empty())for(auto& message:conversation)if(message.card.is_object()&&message.card.value("id","")==personal.inlineID){message.card["state"]=state;personal.dirty=true;break;}
    for(auto& binding:personal.chatBindings){controlMotion.erase(binding.first);if(keyboardId==binding.first)keyboardId=-1;}
    personal.inlineID.clear();personal.inlineChat.clear();personal.inlineTask.clear();personal.inlineNonce.clear();personal.inlineCommands.clear();personal.chatBindings.clear();personal.inlineView=PersonalView::None;personal.inlineDetails=false;
    if(personal.saved&&personal.dirty)personalSaveChat();
    invalidate(160);
}
void personalPresentInline(PersonalView view){
    std::wstring title,body;std::string kind,nonce;
    if(view==PersonalView::Action){
        kind="approval";title=personal.disclosure?L"Compartilhar este resultado?":L"Posso executar esta ação?";
        if(personal.approval){nonce=personal.approval->nonce;body=personal.disclosure?L"O resultado pode conter informações sensíveis. Preciso da sua autorização para enviá-lo ao provedor de IA.":personalActionLabel(personal.approval->request);}
        if(!personal.reviewReason.empty())body+=L"\n"+hext::wide(personal.reviewReason);
        else body+=L"\nPara continuar seu pedido, preciso da sua autorização para esta etapa.";
    }else if(view==PersonalView::WindowConsent){kind="window";title=L"Autorizar leitura da janela";nonce=personal.pendingWindow;auto found=personal.windows.find(nonce);body=(found==personal.windows.end()?L"Janela indisponível":hext::wide(found->second.value("title","")))+std::wstring(L"\nPreciso ler os controles e compartilhar o conteúdo com a IA para continuar. Campos de senha são excluídos.");}
    else if(view==PersonalView::Intervention){kind="participation";title=L"Sua participação";body=personal.inFlight?hext::wide(personal.inFlight->args.value("instruction","")):L"Conclua a etapa no aplicativo oficial.";body+=L"\nEstou verificando a condição esperada para continuar automaticamente. Faça o login no aplicativo oficial; não envie sua senha aqui.";}
    else if(view==PersonalView::SkillReview){kind="skill";title=L"Adicionar receita";body=hext::wide(personal.skillDraft.value("title",""))+L"\nRevise as etapas antes de adicionar. Cada ação continuará sujeita ao modo de permissão escolhido.";}
    else if(view==PersonalView::Remember){kind="memory";title=L"Guardar esta preferência?";body=hext::wide(personal.memoryDraft.value("key",""))+L": "+hext::wide(personal.memoryDraft.value("value",""));if(personal.memoryDraft.contains("previous"))body=L"Antes: "+hext::wide(personal.memoryDraft.value("previous",""))+L"\nAgora: "+body;body+=L"\nIsso poderá ser usado em outras conversas.";}
    else{
        kind=view==PersonalView::Setup?"setup":"connection";auto definition=hext::capabilityDefinition(personal.capability);title=personal.capability=="provider"?L"Conectar assistente":L"Continuar com "+std::wstring(definition?definition->title:L"este recurso");
        nonce=personal.capability+":"+personal.setupReason+":"+std::to_string(personal.spotifyPage);
        body=personal.capability=="provider"?L"Preciso de um provedor conectado para continuar seu pedido. Escolha Codex ou Claude e abra a entrada oficial.":personal.capability=="spotify"?personal.spotifyPage==1?L"Escolha uma faixa abaixo para continuar no Spotify.":personal.spotifyPage==2?L"Selecione um dispositivo disponível para continuar.":L"Preciso de um player disponível ou de uma conexão oficial com o Spotify para continuar seu pedido.":personal.capability=="files"?L"Escolha o arquivo ou a pasta necessária. Vou verificar o acesso e continuar o mesmo pedido.":personal.capability=="applications"?L"Localize o aplicativo necessário. Vou verificar o destino antes de continuar.":personal.capability=="packages"?L"O App Installer precisa estar disponível. Abra a Microsoft Store; vou verificar a instalação antes de continuar.":L"Este pedido precisa de uma configuração. Conclua a etapa oficial e vou verificar o resultado antes de continuar.";
    }
    auto task=personal.task.managed?personal.task.id:"";
    assistantStatus=view==PersonalView::Action||view==PersonalView::WindowConsent?L"Aguardando sua autorização.":view==PersonalView::Remember||view==PersonalView::SkillReview?L"Aguardando sua revisão.":view==PersonalView::Intervention?L"Verificando a etapa no aplicativo…":L"Aguardando configuração.";
    if(personal.inlineView==view&&personal.inlineChat==personal.chat&&personal.inlineTask==task&&personal.inlineNonce==nonce){personal.view=PersonalView::None;invalidate(160);return;}
    personalResolveInline("completed");if(conversation.size()>=498){personal.requestActive=false;personal.task.stage=hext::TaskStage::Blocked;assistantStatus=L"Limite da conversa. Salve e inicie outra antes de continuar.";return;}
    personal.inlineID=personal.reuseCard.empty()?hext::randomID():personal.reuseCard;personal.inlineChat=personal.chat;personal.inlineTask=task;personal.inlineNonce=nonce;personal.inlineView=view;personal.view=PersonalView::None;personal.notice.clear();
    hext::Json card={{"version",1},{"id",personal.inlineID},{"chat",personal.chat},{"task",task},{"kind",kind},{"state","pending"},{"title",hext::utf8(title)}};
    bool reused=false;if(!personal.reuseCard.empty())for(auto& message:conversation)if(message.card.is_object()&&message.card.value("id","")==personal.reuseCard){message.text=body.substr(0,1600);message.card=card;reused=true;break;}
    if(!reused)conversation.push_back({false,body.substr(0,1600),true,false,card});
    personal.dirty=true;assistantScroll=0;
    personal.inlineReveal.snap(motionReduced()?1:0);personal.inlineReveal.target=1;personal.inlineReveal.frequency=45;personal.inlineReveal.damping=1;
    if(personal.saved)personalSaveChat();
    personalCheckpoint();invalidate(220);
    if(view==PersonalView::Setup&&personal.task.stage==hext::TaskStage::Setup)personalQueueGuidance();
}
void personalCancelChat(const std::string& state){
    personal.preserveCheckpoint=false;
    if(personal.inlineView==PersonalView::Remember){personalResolveInline("skipped");personal.memoryDraft=nullptr;personalAppend(false,L"Preferência não guardada.",true);return;}
    personalResolveInline(state);personalRevoke();personal.view=PersonalView::None;
    personalAppend(false,state=="denied"?L"Você negou esta ação. Nenhuma etapa pendente será executada. Se preferir, posso orientar você a realizar a tarefa manualmente ou adaptar o pedido.":L"Tarefa cancelada. Etapas já concluídas não são desfeitas.",true);
    assistantStatus=state=="denied"?L"Ação negada.":L"Tarefa cancelada.";
}
void personalOfferResume(){
    auto record=personal.store.get("task",personal.chat);if(!record.is_object()||!record.contains("task")||!record["task"].is_object())return;
    hext::TaskSession snapshot;if(!snapshot.restore(record["task"])||snapshot.chat!=personal.chat)return;
    if(!personal.inlineID.empty())return;
    personal.inlineID=hext::randomID();personal.inlineChat=personal.chat;personal.inlineTask=snapshot.id;personal.inlineNonce.clear();personal.inlineView=PersonalView::None;personal.inlineCommands.clear();
    for(auto& message:conversation)if(message.card.is_object()&&message.card.value("kind","")=="resume"&&message.card.value("task","")==snapshot.id){message.card["state"]="paused";personal.inlineID=message.card.value("id",personal.inlineID);return;}
    if(conversation.size()>=499){personal.inlineID.clear();return;}
    conversation.push_back({false,hext::wide(snapshot.goal).substr(0,500)+L"\nO pedido foi guardado. Vou verificar o estado atual e solicitar novas autorizações quando necessário.",true,false,{{"version",1},{"id",personal.inlineID},{"chat",personal.chat},{"task",snapshot.id},{"kind","resume"},{"state","paused"},{"title","Retomar tarefa"}}});
    personal.dirty=true;assistantScroll=0;if(personal.saved)personalSaveChat();invalidate(160);
}
void personalLoadPendingChat(const std::string& id){
    if(personal.store.get("chat",id).is_object()){personalOpenChat(id);return;}
    auto record=personal.store.get("task",id);if(!record.is_object()||!record.contains("messages")||!record["messages"].is_array()||record["messages"].size()>30)throw std::runtime_error("Invalid pending chat");
    std::vector<ChatMessage> messages;for(auto& row:record["messages"]){auto message=personalRestoreMessage(row,true);if(message.card.is_object()&&message.card.value("chat","")!=id)message.card=nullptr;messages.push_back(std::move(message));}
    personalPauseTask();personalRevoke();conversation=std::move(messages);personal.chat=id;personal.title=record.value("title","Tarefa guardada");personal.saved=false;personal.dirty=false;personal.view=PersonalView::None;assistantInput.clear();assistantCaret=0;assistantScroll=0;personal.store.put("settings","last-chat",{{"chat",id}});personalOfferResume();
}
int personalChatButton(int command,const std::string& target){
    auto found=personal.inlineCommands.find(command);if(found!=personal.inlineCommands.end()){
        int old=found->second;if(personal.chatBindings.at(old).target==target)return old;
        personal.chatBindings.erase(old);controlMotion.erase(old);personal.inlineCommands.erase(found);
    }
    if(personal.inlineSerial>=2000000000||personal.inlineID.empty())return -1;
    int id=++personal.inlineSerial;personal.inlineCommands[command]=id;
    personal.chatBindings[id]={personal.inlineID,personal.chat,personal.inlineTask,personal.inlineNonce,personal.preferences.revision,command,target};return id;
}
bool personalChatEnabled(int id){
    if(page!=0||personal.view!=PersonalView::None||!expanded)return false;
    auto found=personal.chatBindings.find(id);if(found==personal.chatBindings.end())return false;auto& binding=found->second;
    if(binding.card!=personal.inlineID||binding.chat!=personal.chat||binding.task!=personal.inlineTask||binding.nonce!=personal.inlineNonce||binding.revision!=personal.preferences.revision)return false;
    int command=binding.command;if(command==3490||command==3491){auto record=personal.store.get("task",personal.chat);return record.is_object()&&record.contains("task")&&record["task"].value("id","")==binding.task;}
    if(personal.inlineEditor)return command==3094||command==3492||command==3090;
    if(command==3094||command==3492)return false;
    if(command==3090||command==3423)return true;
    if(command==3493&&personal.inlineView==PersonalView::WindowConsent)return personal.task.waiting.has_value();
    if(command==3095||command==3493)return personal.inlineView==PersonalView::Action&&personal.approval&&personal.approval->nonce==binding.nonce&&GetTickCount64()<=personal.approval->expires;
    if(command==3450)return personal.inlineView==PersonalView::WindowConsent&&personal.task.waiting.has_value();
    if(command==3096)return personal.inlineView==PersonalView::Remember&&personal.memoryDraft.is_object();
    if(command==3314)return personal.inlineView==PersonalView::SkillReview&&personal.skillDraft.is_object();
    if(command==3440)return personal.approval.has_value()||personal.inlineView==PersonalView::SkillReview;
    if(personal.preparing||personal.spotify.busy())return false;
    if((command>=3140&&command<3145)||(command>=3180&&command<3185)){auto state=personal.spotify.snapshot();int index=command>=3180?command-3180:command-3140;if(index>=static_cast<int>(state.tracks.size())||state.tracks[index].value("uri","")!=binding.target)return false;}
    if(command>=3150&&command<3154){auto state=personal.spotify.snapshot();int index=personal.listPage*4+command-3150;if(index>=static_cast<int>(state.devices.size())||state.devices[index].value("id","")!=binding.target||state.devices[index].value("restricted",true))return false;}
    return (personal.inlineView==PersonalView::Setup||personal.inlineView==PersonalView::Connection)&&controlEnabled(command);
}
void personalChatAction(int id){
    if(!personalChatEnabled(id))return;
    int command=personal.chatBindings.at(id).command;
    if(command==3493){personalCancelChat("denied");return;}
    if(command==3090){personalCancelChat();return;}
    if(command==3490){personalResolveInline("completed");personalRestoreTask(personal.chat);return;}
    if(command==3491){personalResolveInline("cancelled");personal.store.erase("task",personal.chat);personalAppend(false,L"Tarefa guardada cancelada. Nenhuma etapa será retomada.",true);return;}
    if(command==3440){personal.inlineDetails=!personal.inlineDetails;invalidate(160);return;}
    if(command==3492){assistantInput=personal.draft;assistantCaret=assistantInput.size();personal.inlineEditor=false;assistantFocused=false;invalidate();return;}
    if(command==466){chooseAssistantExecutable();return;}
    personalAction(command);
}
