#define UNICODE
#define _UNICODE
#define NOMINMAX
#include "../src/task-engine.h"
#include "../src/native-resources.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
    using namespace hext;
    assert(toolRegistry().find("files.create_text")&&toolRegistry().find("apps.find"));std::set<std::string> names;
    for(auto& tool:toolRegistry().tools()){assert(names.insert(tool.name).second&&capabilityDefinition(tool.capability));auto schema=tool.input;assert(schema["additionalProperties"]==false);}
    for(auto& variant:toolRegistry().providerAlternatives())assert(variant["properties"]["arguments"]["required"].size()==variant["properties"]["arguments"]["properties"].size());
    assert(!actionRequest(Json{{"name","files.create_text"},{"arguments",{{"folder","id"},{"name","x.md"},{"text","text"},{"command","cmd.exe"}}}}));
    auto timer=actionRequest(Json{{"name","timer.start"},{"arguments",{{"minutes",3}}}});assert(timer);
    auto volume=actionRequest(Json{{"name","audio.set_volume"},{"arguments",{{"percent",30}}}});assert(volume);
    AssistantPreferences preferences;preferences.autonomy=Autonomy::Allowed;preferences.grants.fill(Grant::Allow);ActionGate gate;
    preferences.serviceGrants["windows.audio"]=Grant::Deny;assert(gate.evaluate(preferences,*volume)==Decision::Denied);preferences.serviceGrants.clear();preferences.toolGrants["audio.set_volume"]=Grant::Ask;assert(gate.evaluate(preferences,*volume)==Decision::Confirm);
    preferences.grants[0]=Grant::Deny;preferences.toolGrants["audio.set_volume"]=Grant::Allow;assert(gate.evaluate(preferences,*volume)==Decision::Denied);
    AssistantPreferences restored;restored.restore(preferences.json());assert(restored.toolGrants.at("audio.set_volume")==Grant::Allow&&restored.grants[0]==Grant::Deny);
    auto media=actionRequest(Json{{"name","media.control"},{"arguments",{{"operation","pause"}}}});assert(media);media->nativeTarget="spotify";preferences=AssistantPreferences{};preferences.autonomy=Autonomy::Allowed;preferences.grants.fill(Grant::Allow);preferences.serviceGrants["spotify"]=Grant::Deny;assert(gate.evaluate(preferences,*media)==Decision::Denied);preferences.serviceGrants["spotify"]=Grant::Ask;assert(gate.evaluate(preferences,*media)==Decision::Confirm);preferences.serviceGrants["spotify"]=Grant::Allow;assert(gate.evaluate(preferences,*media)==Decision::Execute);preferences.toolGrants["spotify.pause"]=Grant::Deny;assert(gate.evaluate(preferences,*media)==Decision::Denied);assert(!actionRequest(Json{{"name","media.control"},{"arguments",{{"operation","pause"}}},{"nativeTarget","spotify"}}));
    SkillRegistry skills;assert(skills.list().size()==1&&SkillRegistry::valid(skills.list()[0]));auto expanded=SkillRegistry::expand(skills.list()[0],{{"minutes",3},{"percent",30}});assert(expanded.size()==2&&expanded[0].name=="timer.start"&&expanded[1].args["percent"]==30);
    auto malicious=skills.list()[0];malicious["id"]="unsafe";malicious["steps"][0]={{"name","shell.exec"},{"arguments",{{"command","cmd.exe"}}}};assert(!skills.add(malicious));malicious["steps"][0]={{"name","skills.run"},{"arguments",{{"id","unsafe"},{"parameters","{}"}}}};assert(!SkillRegistry::valid(malicious));
    bool invalid=false;try{SkillRegistry::expand(skills.list()[0],{{"minutes","three"},{"percent",30}});}catch(...){invalid=true;}assert(invalid);
    TaskSession task;task.begin("chat-a","Find a resource, configure it, and continue");auto spotify=actionRequest(Json{{"name","spotify.resume"},{"arguments",Json::object()}});assert(spotify);task.suspend(*spotify,"spotify");assert(task.active()&&task.waiting->name=="spotify.resume"&&task.goal=="Find a resource, configure it, and continue");task.stage=TaskStage::Executing;assert(task.dispatch(*timer));task.outcome(*timer,true,"Timer verified");assert(!task.dispatch(*timer));assert(task.plan());task.cancel();assert(!task.active()&&!task.waiting);
    task.begin("chat-b","Bound planning");for(int i=0;i<6;i++)assert(task.plan());assert(!task.plan());
    auto root=std::filesystem::current_path()/"dist"/("task-files-"+randomID());std::filesystem::create_directories(root);root=std::filesystem::canonical(root);
    auto created=createAuthorizedText(root,"first.md","native-fixture-marker\n");assert(created&&readAuthorizedText(*created)=="native-fixture-marker\n");assert(!createAuthorizedText(root,"first.md","must not overwrite"));assert(readAuthorizedText(*created)=="native-fixture-marker\n");
    for(auto name:{"../escape.md","x.ps1","stream:bad.txt","CON.txt",".hidden.md","bad.md "})assert(!safeTextName(name)&&!createAuthorizedText(root,name,"data"));
    std::ofstream(root/"invalid.txt",std::ios::binary)<<std::string("\xFF\xFE");assert(!readAuthorizedText(root/"invalid.txt"));std::ofstream(root/"oversize.txt",std::ios::binary)<<std::string(33000,'x');assert(!readAuthorizedText(root/"oversize.txt"));
    assert(!completedProviderTurn("{\"type\":\"turn.completed\"}\n"));assert(!completedProviderTurn("{\"type\":7}\n"));
    auto helper=std::filesystem::current_path()/"dist/assistant-stream-child.exe";if(!std::filesystem::exists(helper))helper=std::filesystem::current_path()/"app/dist/assistant-stream-child.exe";
    AssistantBridge stream;assert(stream.start(helper.wstring(),1,L"Only a generated lifecycle fixture",root/"stream",AssistantTask::Chat,R"({"type":"object"})"));stream.completedTurn=completedProviderTurn;auto began=GetTickCount64();
    while(stream.busy&&GetTickCount64()-began<3000){stream.poll();Sleep(10);}assert(!stream.busy&&stream.exitCode==0&&parseJSON(stream.result)->at("text")=="done"&&GetTickCount64()-began<3000&&stream.answerFile.empty());
    std::cout<<"PASS: completed-turn JSON stream finishes without waiting for a still-running CLI child; only the owned process job is terminated.\n";
    if(argc>1&&std::string(argv[1])=="--discover"){
        ApplicationDiscovery discovery;assert(discovery.start(""));auto deadline=GetTickCount64()+12000;std::optional<Json> matches;
        while(GetTickCount64()<deadline){matches=discovery.take();if(matches)break;Sleep(20);}assert(matches&&discovery.size()>0);for(auto& app:discovery.snapshot())assert(discoverableApplication(std::filesystem::path(wide(app["path"]))));
        std::cout<<"PASS: actual Windows registry and Start menu discovery found "<<discovery.size()<<" safe desktop targets without launching or authorizing any of them.\n";
    }
    std::cout<<"PASS: one discoverable typed tool registry; strict provider variants; service/action grants; declarative skills reject code and recursion; task suspension retains the goal, bounds planning and rejects duplicate writes; real native UTF-8 file creation/readback, no-overwrite and path/type limits.\n";return 0;
}
