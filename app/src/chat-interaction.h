#pragma once
#include "personal-core.h"
namespace hext {
// Presentation history only. No ticket, nonce or execution authority is serialized here.
inline Json restoreChatCard(const Json& value){
    try{
    if(!value.is_object()||value.value("version",0)!=1||!textField(value,"id",64)||!textField(value,"chat",64)||!textField(value,"task",64,true)||!textField(value,"kind",32)||!textField(value,"state",24)||!textField(value,"title",256))return nullptr;
    static const std::set<std::string> kinds={"approval","setup","connection","window","participation","skill","memory","resume"};
    static const std::set<std::string> states={"pending","approved","denied","cancelled","completed","paused","expired","failed","skipped"};
    if(!kinds.count(value["kind"])||!states.count(value["state"]))return nullptr;
    Json card={{"version",1},{"id",value["id"]},{"chat",value["chat"]},{"task",value["task"]},{"kind",value["kind"]},{"state",value["state"]},{"title",value["title"]}};
    if(card["state"]=="pending")card["state"]="paused";
    return card;
    }catch(...){return nullptr;}
}
inline std::wstring chatCardState(const std::string& state){
    if(state=="approved")return L"Permitido";
    if(state=="denied")return L"Negado";
    if(state=="cancelled")return L"Cancelado";
    if(state=="completed")return L"Concluído";
    if(state=="paused")return L"Pausado";
    if(state=="expired")return L"Expirado · precisa de nova autorização";
    if(state=="failed")return L"Não concluído";
    if(state=="skipped")return L"Agora não";
    return L"Aguardando sua participação";
}
}
