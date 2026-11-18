#pragma once
#include "personal-core.h"
namespace hext {
enum class TaskStage {Idle,Planning,Approval,Setup,Choice,Executing,Completed,Blocked,Cancelled};
struct TaskSession {
    TaskStage stage=TaskStage::Idle;std::string id,chat,goal,capability;
    std::optional<ActionRequest> waiting,inFlight;
    unsigned plans=0,steps=0;bool managed=false,setupConsent=false;
    std::map<std::string,unsigned> observed;Json outcomes=Json::array();
    bool active()const{return managed&&stage!=TaskStage::Idle&&stage!=TaskStage::Completed&&stage!=TaskStage::Blocked&&stage!=TaskStage::Cancelled;}
    void begin(const std::string& owner,const std::string& request){*this=TaskSession{};id=randomID();chat=owner;goal=request;managed=true;stage=TaskStage::Planning;}
    void cancel(){stage=TaskStage::Cancelled;managed=false;waiting.reset();inFlight.reset();setupConsent=false;}
    bool plan(){if(!managed||plans>=6||steps>=16)return false;plans++;stage=TaskStage::Planning;return true;}
    bool dispatch(const ActionRequest& request){
        auto fingerprint=request.name+":"+request.args.dump();auto tool=toolRegistry().find(request.name);unsigned limit=tool&&tool->repeatable?8u:tool&&tool->readOnly?2u:1u;
        if(steps>=16||observed[fingerprint]>=limit)return false;
        observed[fingerprint]++;steps++;inFlight=request;stage=TaskStage::Executing;return true;
    }
    void suspend(const ActionRequest& request,const std::string& resource){waiting=request;capability=resource;stage=TaskStage::Setup;}
    Json context()const{return {{"id",id},{"original_request",goal},{"steps",steps},{"rounds",plans},{"outcomes",outcomes}};}
    void outcome(const ActionRequest& request,bool success,const std::string& result){if(outcomes.size()<16)outcomes.push_back({{"action",request.name},{"success",success},{"result",result}});inFlight.reset();stage=success?TaskStage::Executing:TaskStage::Blocked;}
};
inline bool skillName(const std::string& name){return !name.empty()&&name.size()<=64&&std::all_of(name.begin(),name.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';});}
inline Json skillSubstitute(const Json& value,const Json& inputs,int depth=0){
    if(depth>5)throw std::runtime_error("Recipe nesting limit");
    if(value.is_object()){
        if(value.contains("$input")){if(!fields(value,{"$input"})||!textField(value,"$input",64)||!inputs.contains(value["$input"].get<std::string>()))throw std::runtime_error("Unknown recipe parameter");return inputs.at(value["$input"].get<std::string>());}
        Json result=Json::object();for(auto it=value.begin();it!=value.end();++it)result[it.key()]=skillSubstitute(it.value(),inputs,depth+1);return result;
    }
    if(value.is_array())throw std::runtime_error("Recipe arrays are not executable arguments");
    return value;
}
class SkillRegistry {
    std::vector<Json> entries;
public:
    static std::vector<ActionRequest> expand(const Json& recipe,const Json& inputs){
        if(!schemaValue(inputs,recipe.at("parameters")))throw std::runtime_error("Invalid recipe inputs");
        std::vector<ActionRequest> result;
        for(auto& step:recipe.at("steps")){auto substituted=skillSubstitute(step,inputs);auto request=actionRequest(substituted);if(!request||request->name=="skills.run"||request->name.rfind("capabilities.",0)==0)throw std::runtime_error("Recipe uses an unavailable tool");result.push_back(*request);}return result;
    }
    static bool valid(const Json& recipe){
        try{
            if(!fields(recipe,{"kind","version","id","title","description","parameters","steps"})||recipe["kind"]!="hext.skill"||recipe["version"]!=1||!textField(recipe,"id",64)||!skillName(recipe["id"])||!textField(recipe,"title",100)||!textField(recipe,"description",800)||!recipe["steps"].is_array()||recipe["steps"].empty()||recipe["steps"].size()>8)return false;
            auto schema=recipe["parameters"];if(!fields(schema,{"type","properties","required","additionalProperties"})||schema["type"]!="object"||schema["additionalProperties"]!=false||!schema["properties"].is_object()||schema["properties"].size()>8||!schema["required"].is_array()||schema["required"].size()!=schema["properties"].size())return false;
            Json example=Json::object();for(auto it=schema["properties"].begin();it!=schema["properties"].end();++it){auto p=it.value();if(!skillName(it.key())||!p.is_object())return false;auto type=p.value("type","");
                if(type=="string"){if(!fields(p,{"type","maxLength"},{"minLength","enum"})||!intField(p,"maxLength",1,16000))return false;example[it.key()]=p.contains("enum")?p["enum"].at(0):Json("example");}
                else if(type=="integer"){if(!fields(p,{"type","minimum","maximum"})||!intField(p,"minimum",0,100000)||!intField(p,"maximum",0,100000))return false;example[it.key()]=p["minimum"];}
                else if(type=="boolean"){if(!fields(p,{"type"}))return false;example[it.key()]=false;}else return false;
            }
            auto actions=expand(recipe,example);return !actions.empty();
        }catch(...){return false;}
    }
    SkillRegistry(){entries.push_back({{"kind","hext.skill"},{"version",1},{"id","hext.focus"},{"title","Sessão de foco"},{"description","Start a focus timer and set the requested Windows volume. Both values must come from the task or user; each action is permission checked."},{"parameters",toolObject({{"minutes",toolInteger(1,1440)},{"percent",toolInteger(0,100)}})},{"steps",Json::array({Json{{"name","timer.start"},{"arguments",{{"minutes",{{"$input","minutes"}}}}}},Json{{"name","audio.set_volume"},{"arguments",{{"percent",{{"$input","percent"}}}}}}})}});}
    bool add(const Json& recipe){if(!valid(recipe)||entries.size()>=20)return false;for(auto& entry:entries)if(entry["id"]==recipe["id"])return false;entries.push_back(recipe);return true;}
    const std::vector<Json>& list()const{return entries;}
    const Json* find(const std::string& id)const{for(auto& entry:entries)if(entry["id"]==id)return &entry;return nullptr;}
    Json catalog()const{Json result=Json::array();for(auto& entry:entries)result.push_back({{"id",entry["id"]},{"title",entry["title"]},{"description",entry["description"]},{"parameters",entry["parameters"]}});return result;}
};
}
