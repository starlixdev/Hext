#pragma once
#include "personal-core.h"
namespace hext {
enum class TaskStage {Idle,Planning,Approval,Setup,Choice,Executing,Completed,Blocked,Cancelled,Paused};
struct TaskSession {
    static constexpr unsigned MaxPlans=24,MaxSteps=64;
    TaskStage stage=TaskStage::Idle;std::string id,chat,goal,capability;
    std::optional<ActionRequest> waiting,inFlight;
    unsigned plans=0,steps=0;bool managed=false,setupConsent=false;
    std::map<std::string,unsigned> observed;Json outcomes=Json::array();
    bool active()const{return managed&&stage!=TaskStage::Idle&&stage!=TaskStage::Completed&&stage!=TaskStage::Blocked&&stage!=TaskStage::Cancelled&&stage!=TaskStage::Paused;}
    void begin(const std::string& owner,const std::string& request){*this=TaskSession{};id=randomID();chat=owner;goal=request;managed=true;stage=TaskStage::Planning;}
    void cancel(){stage=TaskStage::Cancelled;managed=false;waiting.reset();inFlight.reset();setupConsent=false;}
    bool plan(){if(!managed||plans>=MaxPlans||steps>=MaxSteps)return false;plans++;stage=TaskStage::Planning;return true;}
    bool dispatch(const ActionRequest& request){
        auto fingerprint=request.name+":"+request.args.dump();auto tool=toolRegistry().find(request.name);unsigned limit=tool&&tool->readOnly?16u:tool&&tool->repeatable?8u:1u;
        if(steps>=MaxSteps||observed[fingerprint]>=limit)return false;
        observed[fingerprint]++;steps++;inFlight=request;stage=TaskStage::Executing;return true;
    }
    void suspend(const ActionRequest& request,const std::string& resource){waiting=request;capability=resource;stage=TaskStage::Setup;}
    Json context()const{return {{"id",id},{"original_request",goal},{"steps",steps},{"rounds",plans},{"outcomes",outcomes}};}
    void outcome(const ActionRequest& request,bool success,const std::string& result){if(outcomes.size()<MaxSteps)outcomes.push_back({{"action",request.name},{"success",success},{"result",result.substr(0,1024)}});inFlight.reset();stage=success?TaskStage::Executing:TaskStage::Blocked;}
    Json checkpoint()const{
        auto action=[](const std::optional<ActionRequest>& request)->Json{if(!request)return nullptr;return {{"name",request->name},{"arguments",request->args},{"native_target",request->nativeTarget},{"origin",static_cast<int>(request->origin)}};};
        Json counts=Json::object();for(auto& entry:observed)counts[entry.first]=entry.second;
        return {{"version",1},{"id",id},{"chat",chat},{"goal",goal},{"stage",static_cast<int>(stage)},{"plans",plans},{"steps",steps},{"capability",capability},{"waiting",action(waiting)},{"in_flight",action(inFlight)},{"observed",counts},{"outcomes",outcomes}};
    }
    bool restore(const Json& value){
        try{
            if(value.value("version",0)!=1||!textField(value,"id",64)||!textField(value,"chat",64)||!textField(value,"goal",48000)||!intField(value,"plans",0,MaxPlans)||!intField(value,"steps",0,MaxSteps)||!value.at("observed").is_object()||value["observed"].size()>MaxSteps||!value.at("outcomes").is_array()||value["outcomes"].size()>MaxSteps)return false;
            TaskSession next;next.id=value["id"];next.chat=value["chat"];next.goal=value["goal"];next.plans=value["plans"];next.steps=value["steps"];next.capability=value.value("capability","");next.outcomes=value["outcomes"];next.managed=true;next.stage=TaskStage::Paused;
            for(auto it=value["observed"].begin();it!=value["observed"].end();++it){if(it.key().size()>10000||!it.value().is_number_unsigned()||it.value().get<unsigned>()>8)return false;next.observed[it.key()]=it.value().get<unsigned>();}
            for(auto field:{"waiting","in_flight"})if(!value.at(field).is_null()){
                auto& data=value.at(field);auto request=actionRequest(Json{{"name",data.at("name")},{"arguments",data.at("arguments")}});if(!request)return false;
                request->nativeTarget=data.value("native_target","");if(!request->nativeTarget.empty()&&(request->nativeTarget!="spotify"||request->name!="media.control"))return false;
                if(data.value("origin",0)==1)request->origin=ActionOrigin::Voice;
                if(std::string(field)=="waiting")next.waiting=*request;else next.inFlight=*request;
            }
            *this=std::move(next);return true;
        }catch(...){return false;}
    }
    void resumePlanning(){
        if(inFlight&&outcomes.size()<MaxSteps)outcomes.push_back({{"action",inFlight->name},{"success",false},{"result","Interrupted while in flight: outcome unknown. Observe current state; never automatically repeat this effect."},{"uncertain",true}});
        inFlight.reset();waiting.reset();setupConsent=false;plans=0;stage=TaskStage::Planning;
    }
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
        for(auto& step:recipe.at("steps")){auto substituted=skillSubstitute(step,inputs);auto request=actionRequest(substituted);if(!request||request->name=="terminal.run"||request->name=="skills.run"||request->name.rfind("capabilities.",0)==0)throw std::runtime_error("Recipe uses an unavailable tool");result.push_back(*request);}return result;
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
