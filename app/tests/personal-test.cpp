#define UNICODE
#define _UNICODE
#define NOMINMAX
#include "../src/personal-core.h"
#include "../src/personal-net.h"
#include <cassert>
#include <iostream>

int main(int argc,char** argv){
    using namespace hext;
    assert(!parseJSON("{\"a\":1,\"a\":2}"));
    assert(!parseJSON(std::string(40,'[')+"0"+std::string(40,']')));
    for(auto invalid:{R"({"name":"shell.exec","arguments":{"command":"calc.exe"}})",R"({"name":"audio.set_volume","arguments":{"percent":101}})",R"({"name":"audio.set_volume","arguments":{"percent":30.5}})",R"({"name":"audio.set_volume","arguments":{"percent":30,"command":"cmd"}})",R"({"name":"audio.set_mute","arguments":{"muted":"true"}})",R"({"name":"timer.start","arguments":{"minutes":0}})",R"({"name":"files.open","arguments":{"path":"C:\\Windows"}})"}){
        auto j=parseJSON(invalid);assert(j&&!actionRequest(*j));
    }
    auto volume=actionRequest(Json{{"name","audio.set_volume"},{"arguments",{{"percent",30}}}});assert(volume);
    ActionGate gate;AssistantPreferences prefs;
    assert(gate.evaluate(prefs,*volume)==Decision::Confirm);
    auto ticket=gate.propose(prefs,*volume,"chat-a");assert(!gate.consume(ticket.nonce,prefs,"chat-b"));
    ticket=gate.propose(prefs,*volume,"chat-a");prefs.revision++;assert(!gate.consume(ticket.nonce,prefs,"chat-a"));
    ticket=gate.propose(prefs,*volume,"chat-a");assert(gate.consume(ticket.nonce,prefs,"chat-a"));assert(!gate.consume(ticket.nonce,prefs,"chat-a"));
    for(auto mode:{PermissionMode::Ask,PermissionMode::Delegate,PermissionMode::Always}){prefs.mode=mode;assert(gate.evaluate(prefs,*volume)==(mode==PermissionMode::Ask?Decision::Confirm:mode==PermissionMode::Delegate?Decision::Review:Decision::Execute));}
    prefs.mode=PermissionMode::Delegate;ticket=gate.propose(prefs,*volume,"chat-a",ExecutionAuthority::Reviewer);assert(gate.consumeReview(ticket.nonce,prefs,"chat-a"));assert(gate.authorizeExecution(prefs,*volume,"chat-a",ExecutionAuthority::Reviewer));assert(!gate.authorizeExecution(prefs,*volume,"chat-a",ExecutionAuthority::Reviewer));
    prefs.mode=PermissionMode::Always;    assert(utf8Excerpt("a\xC3\xA7" "b",2,2)=="b");assert(utf8Excerpt("a\xC3\xA7" "b",0,2)=="a");
    auto voice=actionRequest(Json{{"name","spotify.pause"},{"arguments",Json::object()}});assert(voice);voice->origin=ActionOrigin::Voice;assert(gate.evaluate(prefs,*voice)==Decision::Denied);
    auto loud=actionRequest(Json{{"name","audio.set_volume"},{"arguments",{{"percent",100}}}});assert(loud&&gate.evaluate(prefs,*loud)==Decision::Confirm);
    ticket=gate.propose(prefs,*volume,"chat-a");prefs.blocked=true;assert(!gate.consume(ticket.nonce,prefs,"chat-a"));
    assert(pkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")=="E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM");
    assert(oauthCode("GET /callback?code=abc%2B123&state=expected HTTP/1.1\r\n\r\n","expected")=="abc+123");
    for(auto request:{"GET /callback?code=abc&state=wrong HTTP/1.1","GET /callback?code=abc&state=expected&state=wrong HTTP/1.1","GET /callback?code=%GG&state=expected HTTP/1.1","GET /callback?error=access_denied&state=expected HTTP/1.1"})assert(!oauthCode(request,"expected"));
    auto root=std::filesystem::current_path()/"dist"/("personal-test-"+randomID());std::filesystem::create_directories(root);
    {
        PersonalStore store;store.open(root);assert(store.ready());MemoryManager memory(store);
        std::ofstream(root/"unsafe.cmd")<<"test";std::ofstream(root/"safe.txt")<<"test";
        assert(!safeAsset(root/"unsafe.cmd",false)&&!safeAsset(root/"unsafe.cmd",true)&&safeAsset(root/"safe.txt",false)&&!safeAsset(root/"safe.txt",true)&&safeAsset(root,false));
        store.put("chat","a",{{"title","Private-A-unique-3947"},{"messages",Json::array({Json{{"role","user"},{"text","chat a only"}}})}});
        store.put("chat","b",{{"title","Private-B"},{"messages",Json::array({Json{{"role","user"},{"text","chat b only"}}})}});
        assert(store.get("chat","a")["messages"][0]["text"]=="chat a only");assert(store.get("chat","b")["messages"][0]["text"]=="chat b only");
        try{store.transaction([&]{store.put("chat","rollback",{{"title","must not persist"}});throw std::runtime_error("Abort transaction");});}catch(...){}
        assert(store.get("chat","rollback").is_null()&&store.count("chat")==2);
        memory.remember("style","brief",true,"a");memory.remember("STYLE","detailed",true,"a");assert(memory.list().size()==1&&memory.list()[0]["value"]=="detailed");
        bool refused=false;try{memory.remember("style","inferred",false,"b");}catch(...){refused=true;}assert(refused);
        refused=false;try{memory.remember("password","abc",true,"a");}catch(...){refused=true;}assert(refused);
        memory.remember("pinned","keep",true,"a",true);memory.eraseChat("a");assert(memory.list().size()==1&&memory.list()[0]["key"]=="pinned");
        store.put("settings","assistant",prefs.json());store.put("credential","probe",{{"token","Private-Token-unique-5839"}});
    }
    {
        PersonalStore reopened;reopened.open(root);assert(reopened.get("chat","a")["title"]=="Private-A-unique-3947");assert(reopened.get("chat","b")["messages"][0]["text"]=="chat b only");
        AssistantPreferences restored;restored.restore(reopened.get("settings","assistant"));assert(restored.mode==PermissionMode::Always);
        auto row=reopened.get("chat","a");row["title"]="Renamed";reopened.put("chat","a",row);assert(reopened.get("chat","a")["title"]=="Renamed");
        reopened.erase("chat","a");assert(reopened.get("chat","a").is_null()&&reopened.get("chat","b").is_object());
        SpotifyService spotify(reopened);spotify.initialize();assert(!spotify.snapshot().credentials);assert(spotify.command("verify"));
        auto until=GetTickCount64()+5000;while(spotify.busy()&&GetTickCount64()<until)Sleep(10);auto state=spotify.snapshot();assert(!state.busy&&!state.success&&!state.connected);
        spotify.disconnect();assert(reopened.get("credential","spotify").is_null());reopened.erase("credential","probe");
        MemoryManager memory(reopened);for(int i=1;i<200;i++)memory.remember("preference-"+std::to_string(i),"value",true,"");assert(memory.list().size()==200);
        bool limited=false;try{memory.remember("overflow","value",true,"");}catch(...){limited=true;}assert(limited);
    }
    std::ifstream input(root/"assistant.db",std::ios::binary);std::string disk((std::istreambuf_iterator<char>(input)),{});
    assert(disk.find("Private-A-unique-3947")==std::string::npos&&disk.find("Private-Token-unique-5839")==std::string::npos&&disk.find("chat b only")==std::string::npos);
    if(argc>1&&std::string(argv[1])=="--live-auth-failure"){
        auto denied=spotifyHTTP(L"api.spotify.com",L"/v1/me/player/devices",L"GET","","hext-invalid-integration-probe");assert(denied.status==401);std::cout<<"PASS: real Spotify TLS request rejected invalid authentication (401), no playback changed\n";
    }
    std::cout<<"PASS: strict action validation; permission levels; approval binding, replay and revocation; encrypted persistence, isolated chats, rename/delete; bounded memory, explicit preference priority and provenance deletion; PKCE and OAuth state; missing Spotify authentication. No live Windows settings changed.\n";
}
