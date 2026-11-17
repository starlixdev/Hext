#pragma once
enum class PersonalView {None,Chats,NewChat,Memory,Assets,Editor,Action,Remember,Delete};
struct PersonalState {
    hext::PersonalStore store;
    hext::MemoryManager memory{store};
    hext::SpotifyService spotify{store};
    hext::MediaService media;
    hext::AssistantPreferences preferences;
    hext::ActionGate gate;
    PersonalView view=PersonalView::None,editorReturn=PersonalView::None;
    bool ready=false,saved=false,dirty=false,requestActive=false,spotifyOwned=false,mediaOwned=false;
    std::string chat=hext::randomID(),title="Nova conversa",requestChat,editorKind,editorID;
    std::string pendingChat;
    std::filesystem::path requestWork;
    bool followUp=false;
    std::wstring draft,editorTitle,notice;
    int settingsTab=0,listPage=0,permissionPage=0,spotifyPage=0,rounds=0;
    std::vector<hext::Json> rows;
    std::vector<hext::ActionRequest> queued;
    std::optional<hext::ActionTicket> approval;
    hext::Json memoryDraft=nullptr;
    Spring transition{1,0,1};
};
PersonalState personal;
bool personalVerification=false,personalProviderVerification=false,personalNativeVerification=false;
void personalAction(int id);
void personalPoll();
void personalSaveChat();
void personalOpenChat(const std::string&);
void personalReply();
void personalSend();
void personalRunProvider();
void personalCommitEditor();
void startTimer();
void personalCloseView();
void personalChangePage();
void personalRevoke();
bool personalBusy(){return assistantBridge.busy||personal.requestActive||personal.spotifyOwned||personal.mediaOwned;}
