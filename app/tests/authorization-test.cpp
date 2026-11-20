#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include "../src/personal-core.h"
#include <cassert>
#include <iostream>
int main(){
    using namespace hext;ActionGate gate;AssistantPreferences prefs;auto timer=actionRequest({{"name","timer.start"},{"arguments",{{"minutes",1}}}});assert(timer);
    for(auto mode:{PermissionMode::Ask,PermissionMode::Delegate,PermissionMode::Always}){
        prefs.mode=mode;assert(gate.evaluate(prefs,*timer)==(mode==PermissionMode::Ask?Decision::Confirm:mode==PermissionMode::Delegate?Decision::Review:Decision::Execute));
        assert(gate.authorizeExecution(prefs,*timer,"chat",ExecutionAuthority::None)==(mode==PermissionMode::Always));
        AssistantPreferences restored;restored.restore(prefs.json());assert(restored.mode==mode&&!prefs.json().contains("grants")&&!prefs.json().contains("services")&&!prefs.json().contains("tools"));
        for(auto invalid:{Json{{"name","files.delete"},{"arguments",{{"path","C:/private"}}}},Json{{"name","finance.transfer"},{"arguments",{{"amount",1000}}}},Json{{"name","windows.modify_system"},{"arguments",Json::object()}}}){ActionRequest request{invalid["name"],invalid["arguments"]};assert(gate.evaluate(prefs,request)==Decision::Denied);}
        auto install=actionRequest({{"name","packages.install"},{"arguments",{{"proposal","native-proposal"}}}});assert(install&&gate.evaluate(prefs,*install)==Decision::Confirm);
        auto ticket=gate.propose(prefs,*install,"chat",ExecutionAuthority::Reviewer);assert(!gate.consumeReview(ticket.nonce,prefs,"chat"));assert(!gate.authorizeExecution(prefs,*install,"chat",ExecutionAuthority::Reviewer));
        auto destructive=actionRequest({{"name","terminal.run"},{"arguments",{{"script","Remove-Item Hext:/file.txt"},{"purpose","Delete scratch data"}}}});assert(destructive&&gate.evaluate(prefs,*destructive)==Decision::Confirm);
    }
    AssistantPreferences legacy;legacy.restore({{"autonomy",2},{"grants",Json::array({2,2,2,2,2,2,2,2})},{"services",{{"files",2}}},{"language","en"},{"memory",false}});assert(legacy.mode==PermissionMode::Ask&&legacy.language=="en"&&!legacy.memory);
    prefs.mode=PermissionMode::Ask;auto user=gate.propose(prefs,*timer,"chat");assert(!gate.authorizeExecution(prefs,*timer,"chat",ExecutionAuthority::User));assert(gate.consume(user.nonce,prefs,"chat"));assert(gate.authorizeExecution(prefs,*timer,"chat",ExecutionAuthority::User));assert(!gate.authorizeExecution(prefs,*timer,"chat",ExecutionAuthority::User));
    prefs.mode=PermissionMode::Delegate;auto review=gate.propose(prefs,*timer,"chat",ExecutionAuthority::Reviewer);assert(!gate.consume(review.nonce,prefs,"chat"));
    review=gate.propose(prefs,*timer,"chat",ExecutionAuthority::Reviewer);assert(gate.consumeReview(review.nonce,prefs,"chat"));auto changed=*timer;changed.args["minutes"]=2;assert(!gate.authorizeExecution(prefs,changed,"chat",ExecutionAuthority::Reviewer));
    review=gate.propose(prefs,*timer,"chat",ExecutionAuthority::Reviewer);prefs.revision++;assert(!gate.consumeReview(review.nonce,prefs,"chat"));
    review=gate.propose(prefs,*timer,"chat",ExecutionAuthority::Reviewer);assert(!gate.consumeReview(review.nonce,prefs,"other-chat"));
    review=gate.propose(prefs,*timer,"chat",ExecutionAuthority::Reviewer);prefs.mode=PermissionMode::Always;assert(!gate.consumeReview(review.nonce,prefs,"chat"));
    assert(reviewVerdict(R"({"decision":"approve","reason":"Safe timer"})",1));assert(reviewVerdict(R"({"structured_output":{"decision":"ask","reason":"Needs user"}})",2));
    for(auto invalid:{R"({"decision":"approve","reason":"x","actions":[]})",R"({"decision":"allow_all","reason":"x"})",R"({"decision":"approve"})",R"({"decision":true,"reason":"x"})"})assert(!reviewVerdict(invalid,1));
    assert(!reviewVerdict(R"({"is_error":true,"structured_output":{"decision":"approve","reason":"x"}})",2));
    assert(sensitiveText("api_key=generated-private-example"));assert(sensitiveText(R"({"text":"{\"access_token\":\"generated\"}"})"));assert(!sensitiveText("A normal generated note for the task."));
    assert(!sensitiveText("Create a task-local test note without network or personal-file access"));assert(sensitiveText("token sk-generated_fixture_key_1234567890"));
    assert(protectedUIContext("Transferir PIX"));assert(protectedUIContext("Excluir permanentemente"));assert(protectedUIContext("Pay now"));
    std::cout<<"PASS: three global modes, conservative migration, immutable high-risk guards, bound one-use human/AI permits, stale/mismatched/forged review rejection and sensitive-disclosure detection.\n";
}
