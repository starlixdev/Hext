#pragma once
#include "assistant.h"
#include "../deps/json.hpp"
#include "../deps/sqlite-amalgamation-3540100/sqlite3.h"
#include <wincrypt.h>
#include <bcrypt.h>
#include <sddl.h>
#include <aclapi.h>
#include <mutex>
#include <set>
#include <optional>
#include <functional>
#include <ctime>
#include <array>

namespace hext {
using Json=nlohmann::json;
inline std::optional<Json> parseJSON(const std::string& input,size_t max=1024*1024){
    if(input.size()>max)return {};
    try {
        std::array<std::set<std::string>,33> names;
        auto callback=[&](int depth,Json::parse_event_t event,Json& value){
            if(depth>30)throw std::runtime_error("JSON nesting limit");
            if(event==Json::parse_event_t::object_start)names[depth+1].clear();
            if(event==Json::parse_event_t::key&&!names[depth].insert(value.get<std::string>()).second)throw std::runtime_error("Duplicate field");
            return true;
        };
        auto value=Json::parse(input,callback,false);
        if(!value.is_discarded())return value;
    }catch(...){}
    return {};
}
inline bool fields(const Json& value,std::initializer_list<const char*> required,std::initializer_list<const char*> optional={}){
    if(!value.is_object())return false;
    std::set<std::string> allowed;
    for(auto key:required){allowed.insert(key);if(!value.contains(key))return false;}
    for(auto key:optional)allowed.insert(key);
    for(auto it=value.begin();it!=value.end();++it)if(!allowed.count(it.key()))return false;
    return true;
}
inline bool textField(const Json& j,const char* key,size_t max,bool empty=false){
    if(!j.contains(key)||!j[key].is_string())return false;
    const auto& s=j[key].get_ref<const std::string&>();
    return s.size()<=max&&(empty||!s.empty())&&s.find('\0')==std::string::npos;
}
inline bool intField(const Json& j,const char* key,int low,int high){
    if(!j.contains(key)||!j[key].is_number_integer())return false;
    try {auto value=j[key].get<long long>();return value>=low&&value<=high;}catch(...){return false;}
}
inline std::string randomID(){
    std::array<unsigned char,16> bytes{};
    if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Random source unavailable");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:bytes){out+=hex[b>>4];out+=hex[b&15];}return out;
}
inline long long timestamp(){return static_cast<long long>(std::time(nullptr));}
inline std::vector<unsigned char> protect(const std::string& input){
    DATA_BLOB plain{static_cast<DWORD>(input.size()),reinterpret_cast<BYTE*>(const_cast<char*>(input.data()))},encrypted{};
    if(!CryptProtectData(&plain,L"Hext private data",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&encrypted))throw std::runtime_error("Data protection failed");
    std::vector<unsigned char> out(encrypted.pbData,encrypted.pbData+encrypted.cbData);LocalFree(encrypted.pbData);return out;
}
inline std::string unprotect(const void* bytes,int count){
    DATA_BLOB encrypted{static_cast<DWORD>(count),reinterpret_cast<BYTE*>(const_cast<void*>(bytes))},plain{};
    if(count<=0||count>4*1024*1024||!CryptUnprotectData(&encrypted,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&plain))throw std::runtime_error("Private data could not be decrypted");
    std::string out(reinterpret_cast<char*>(plain.pbData),plain.cbData);SecureZeroMemory(plain.pbData,plain.cbData);LocalFree(plain.pbData);return out;
}
inline bool privateDirectory(const std::filesystem::path& path){
    std::error_code error;std::filesystem::create_directories(path,error);if(error)return false;
    auto attributes=GetFileAttributesW(path.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
    HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
    DWORD size=0;GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<BYTE> user(size);
    bool read=GetTokenInformation(token,TokenUser,user.data(),size,&size)!=FALSE;CloseHandle(token);if(!read)return false;
    LPWSTR sid=nullptr;if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid,&sid))return false;
    std::wstring sddl=L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;"+std::wstring(sid)+L")";LocalFree(sid);
    PSECURITY_DESCRIPTOR descriptor=nullptr;if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&descriptor,nullptr))return false;
    BOOL present=FALSE,defaulted=FALSE;PACL acl=nullptr;GetSecurityDescriptorDacl(descriptor,&present,&acl,&defaulted);
    DWORD result=SetNamedSecurityInfoW(const_cast<wchar_t*>(path.c_str()),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,acl,nullptr);
    LocalFree(descriptor);return result==ERROR_SUCCESS;
}
class PersonalStore {
    sqlite3* db=nullptr;std::recursive_mutex mutex;
    struct Statement {sqlite3_stmt* p=nullptr;~Statement(){if(p)sqlite3_finalize(p);}};
    void execute(const char* sql){if(sqlite3_exec(db,sql,nullptr,nullptr,nullptr)!=SQLITE_OK)throw std::runtime_error("Local storage operation failed");}
    void prepare(Statement& q,const char* sql){if(sqlite3_prepare_v2(db,sql,-1,&q.p,nullptr)!=SQLITE_OK)throw std::runtime_error("Local storage query failed");}
public:
    PersonalStore()=default;PersonalStore(const PersonalStore&)=delete;~PersonalStore(){if(db)sqlite3_close(db);}
    void open(const std::filesystem::path& root){
        if(!privateDirectory(root))throw std::runtime_error("Cannot protect local data directory");
        auto path=root/L"assistant.db";
        if(std::filesystem::exists(path)&&(GetFileAttributesW(path.c_str())&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Unexpected storage link");
        if(sqlite3_open_v2(utf8(path.wstring()).c_str(),&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr)!=SQLITE_OK)throw std::runtime_error("Cannot open local database");
        sqlite3_busy_timeout(db,1500);sqlite3_limit(db,SQLITE_LIMIT_LENGTH,2*1024*1024);sqlite3_limit(db,SQLITE_LIMIT_SQL_LENGTH,4096);
        execute("PRAGMA trusted_schema=OFF; PRAGMA temp_store=MEMORY; PRAGMA journal_mode=DELETE; PRAGMA synchronous=FULL; PRAGMA secure_delete=ON; PRAGMA max_page_count=16384;");
        Statement q;prepare(q,"PRAGMA user_version");if(sqlite3_step(q.p)!=SQLITE_ROW||sqlite3_column_int(q.p,0)>1)throw std::runtime_error("Database requires a newer Hext");
        execute("CREATE TABLE IF NOT EXISTS documents(kind TEXT NOT NULL,id TEXT NOT NULL,data BLOB NOT NULL,updated INTEGER NOT NULL,PRIMARY KEY(kind,id)) WITHOUT ROWID; PRAGMA user_version=1;");
    }
    bool ready()const{return db!=nullptr;}
    void put(const std::string& kind,const std::string& id,const Json& value){
        std::lock_guard<std::recursive_mutex> lock(mutex);if(!db)throw std::runtime_error("Storage unavailable");
        auto serialized=value.dump();if(serialized.size()>1024*1024)throw std::runtime_error("Item exceeds storage limit");auto encrypted=protect(serialized);
        Statement q;prepare(q,"INSERT INTO documents VALUES(?1,?2,?3,?4) ON CONFLICT(kind,id) DO UPDATE SET data=excluded.data,updated=excluded.updated");
        sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(q.p,2,id.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_blob(q.p,3,encrypted.data(),static_cast<int>(encrypted.size()),SQLITE_TRANSIENT);sqlite3_bind_int64(q.p,4,timestamp());
        if(sqlite3_step(q.p)!=SQLITE_DONE)throw std::runtime_error("Cannot save private data (storage may be full)");
    }
    Json get(const std::string& kind,const std::string& id){
        std::lock_guard<std::recursive_mutex> lock(mutex);if(!db)return nullptr;
        Statement q;prepare(q,"SELECT data FROM documents WHERE kind=?1 AND id=?2");sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(q.p,2,id.c_str(),-1,SQLITE_TRANSIENT);
        auto status=sqlite3_step(q.p);if(status==SQLITE_DONE)return nullptr;if(status!=SQLITE_ROW)throw std::runtime_error("Cannot read private data");
        auto parsed=parseJSON(unprotect(sqlite3_column_blob(q.p,0),sqlite3_column_bytes(q.p,0)));if(!parsed)throw std::runtime_error("Invalid private record");return *parsed;
    }
    std::vector<Json> list(const std::string& kind,int limit=200){
        std::lock_guard<std::recursive_mutex> lock(mutex);std::vector<Json> out;if(!db)return out;
        Statement q;prepare(q,"SELECT id,data FROM documents WHERE kind=?1 ORDER BY updated DESC,id LIMIT ?2");sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_int(q.p,2,std::clamp(limit,1,500));
        int status;while((status=sqlite3_step(q.p))==SQLITE_ROW){auto parsed=parseJSON(unprotect(sqlite3_column_blob(q.p,1),sqlite3_column_bytes(q.p,1)));if(!parsed||!parsed->is_object())throw std::runtime_error("Invalid private record");(*parsed)["id"]=reinterpret_cast<const char*>(sqlite3_column_text(q.p,0));out.push_back(*parsed);}
        if(status!=SQLITE_DONE)throw std::runtime_error("Cannot read private records");
        return out;
    }
    void erase(const std::string& kind,const std::string& id){
        std::lock_guard<std::recursive_mutex> lock(mutex);if(!db)return;Statement q;prepare(q,"DELETE FROM documents WHERE kind=?1 AND id=?2");sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(q.p,2,id.c_str(),-1,SQLITE_TRANSIENT);if(sqlite3_step(q.p)!=SQLITE_DONE)throw std::runtime_error("Cannot delete private data");
    }
    void clear(const std::string& kind){
        std::lock_guard<std::recursive_mutex> lock(mutex);if(!db)return;Statement q;prepare(q,"DELETE FROM documents WHERE kind=?1");sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);if(sqlite3_step(q.p)!=SQLITE_DONE)throw std::runtime_error("Cannot delete private records");
    }
    void transaction(const std::function<void()>& action){
        std::lock_guard<std::recursive_mutex> lock(mutex);execute("BEGIN IMMEDIATE");
        try{action();execute("COMMIT");}catch(...){sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);throw;}
    }
    int count(const std::string& kind){
        std::lock_guard<std::recursive_mutex> lock(mutex);Statement q;prepare(q,"SELECT count(*) FROM documents WHERE kind=?1");sqlite3_bind_text(q.p,1,kind.c_str(),-1,SQLITE_TRANSIENT);
        if(sqlite3_step(q.p)!=SQLITE_ROW)throw std::runtime_error("Cannot count private records");
        return sqlite3_column_int(q.p,0);
    }
};
enum class Autonomy {Chat,Ask,Allowed};
enum class Grant {Deny,Ask,Allow};
enum class Category {Audio,Timers,Browser,Apps,Files,Media,Spotify,History,Count};
inline constexpr const char* CategoryKeys[]={"audio","timers","browser","apps","files","media","spotify","history"};
inline const wchar_t* categoryName(Category c){const wchar_t* names[]={L"Áudio do Windows",L"Temporizadores",L"Navegador",L"Aplicativos",L"Arquivos e pastas",L"Mídia do Windows",L"Spotify",L"Conversas anteriores"};return names[static_cast<int>(c)];}
struct AssistantPreferences {
    Autonomy autonomy=Autonomy::Ask;std::array<Grant,static_cast<int>(Category::Count)> grants{};
    bool memory=true;std::string language="auto",searchEngine="https://www.google.com/search?q=";unsigned revision=1;
    AssistantPreferences(){grants.fill(Grant::Ask);grants[static_cast<int>(Category::History)]=Grant::Deny;}
    Json json()const{Json out={{"autonomy",static_cast<int>(autonomy)},{"memory",memory},{"language",language},{"grants",Json::array()}};for(auto g:grants)out["grants"].push_back(static_cast<int>(g));return out;}
    void restore(const Json& j){if(!j.is_object())return;if(intField(j,"autonomy",0,2))autonomy=static_cast<Autonomy>(j["autonomy"].get<int>());if(j.contains("memory")&&j["memory"].is_boolean())memory=j["memory"].get<bool>();if(j.contains("language")&&j["language"].is_string()){auto l=j["language"].get<std::string>();if(l=="auto"||l=="pt-BR"||l=="en")language=l;}if(j.contains("grants")&&j["grants"].is_array()&&j["grants"].size()==grants.size())for(size_t i=0;i<grants.size();i++)if(j["grants"][i].is_number_integer()){int v=j["grants"][i].get<int>();if(v>=0&&v<=2)grants[i]=static_cast<Grant>(v);}}
};
enum class ActionOrigin {Text,Voice};
struct ActionRequest {std::string name;Json args;Category category=Category::Audio;bool sensitive=false;ActionOrigin origin=ActionOrigin::Text;};
inline std::optional<ActionRequest> actionRequest(const Json& value){
    if(!fields(value,{"name","arguments"})||!textField(value,"name",64)||!value["arguments"].is_object())return {};
    ActionRequest r;r.name=value["name"].get<std::string>();r.args=value["arguments"];auto& a=r.args;bool valid=false;
    if(r.name=="audio.set_volume"){r.category=Category::Audio;valid=fields(a,{"percent"},{"device"})&&intField(a,"percent",0,100)&&(!a.contains("device")||textField(a,"device",256));r.sensitive=valid&&a["percent"].get<int>()>80;}
    else if(r.name=="audio.set_mute"){r.category=Category::Audio;valid=fields(a,{"muted"},{"device"})&&a["muted"].is_boolean()&&(!a.contains("device")||textField(a,"device",256));}
    else if(r.name=="audio.list_devices"){r.category=Category::Audio;valid=fields(a,{});}
    else if(r.name=="audio.set_default_device"){r.category=Category::Audio;r.sensitive=true;valid=fields(a,{"device"})&&textField(a,"device",256);}
    else if(r.name=="timer.start"){r.category=Category::Timers;valid=fields(a,{"minutes"})&&intField(a,"minutes",1,1440);}
    else if(r.name=="timer.pause"){r.category=Category::Timers;valid=fields(a,{});}
    else if(r.name=="browser.search"){r.category=Category::Browser;valid=fields(a,{"query"})&&textField(a,"query",512);}
    else if(r.name=="apps.open"||r.name=="files.open"){r.category=r.name=="apps.open"?Category::Apps:Category::Files;valid=fields(a,{"id"})&&textField(a,"id",64);}
    else if(r.name=="media.control"){r.category=Category::Media;valid=fields(a,{"operation"})&&textField(a,"operation",16);if(valid){auto op=a["operation"].get<std::string>();valid=op=="play"||op=="pause"||op=="next"||op=="previous";}}
    else if(r.name=="spotify.search"||r.name=="spotify.play"){r.category=Category::Spotify;valid=fields(a,{"query"})&&textField(a,"query",256);}
    else if(r.name=="spotify.pause"||r.name=="spotify.next"||r.name=="spotify.previous"||r.name=="spotify.resume"){r.category=Category::Spotify;valid=fields(a,{});}
    else if(r.name=="spotify.set_volume"){r.category=Category::Spotify;valid=fields(a,{"percent"})&&intField(a,"percent",0,100);r.sensitive=valid&&a["percent"].get<int>()>80;}
    else if(r.name=="history.search"){r.category=Category::History;valid=fields(a,{"query"})&&textField(a,"query",256);}
    return valid?std::optional<ActionRequest>(r):std::nullopt;
}
enum class Decision {Denied,Confirm,Execute};
struct ActionTicket {std::string nonce,chat;ActionRequest request;unsigned revision=0;ULONGLONG expires=0;};
class ActionGate {
    std::optional<ActionTicket> pending;
public:
    Decision evaluate(const AssistantPreferences& p,const ActionRequest& r)const{
        if(static_cast<int>(r.category)<0||r.category>=Category::Count)return Decision::Denied;
        if(r.category==Category::Spotify&&r.origin==ActionOrigin::Voice)return Decision::Denied;
        auto grant=p.grants[static_cast<int>(r.category)];if(p.autonomy==Autonomy::Chat||grant==Grant::Deny)return Decision::Denied;
        return p.autonomy==Autonomy::Ask||grant==Grant::Ask||r.sensitive?Decision::Confirm:Decision::Execute;
    }
    ActionTicket propose(const AssistantPreferences& p,const ActionRequest& r,const std::string& chat){pending=ActionTicket{randomID(),chat,r,p.revision,GetTickCount64()+120000};return *pending;}
    std::optional<ActionRequest> consume(const std::string& nonce,const AssistantPreferences& p,const std::string& chat){
        if(!pending)return {};
        auto ticket=*pending;pending.reset();
        if(ticket.nonce!=nonce||ticket.chat!=chat||ticket.revision!=p.revision||GetTickCount64()>ticket.expires||evaluate(p,ticket.request)==Decision::Denied)return {};
        return ticket.request;
    }
    void revoke(){pending.reset();}
};
inline bool privateMemory(const std::string& key,const std::string& value){
    if(key.empty()||key.size()>64||value.empty()||value.size()>512)return false;
    auto s=lower(key+" "+value);
    for(auto word:{"password","senha","token","secret","api_key","api key","sk-","bearer ","-----begin","cpf","credit card","cartão","cartao","ssn","cookie"})if(s.find(word)!=std::string::npos)return false;
    return true;
}
inline std::string memoryKey(std::string key){
    key=lower(key);while(!key.empty()&&std::isspace(static_cast<unsigned char>(key.back())))key.pop_back();size_t i=0;while(i<key.size()&&std::isspace(static_cast<unsigned char>(key[i])))i++;key=key.substr(i);
    if(key=="style"||key=="estilo"||key=="estilo_resposta"||key=="response style")return "response_style";
    if(key=="idioma"||key=="preferred_language")return "language";
    if(key=="nome"||key=="name")return "preferred_name";
    return key;
}
class MemoryManager {
    PersonalStore& store;
public:
    explicit MemoryManager(PersonalStore& s):store(s){}
    std::vector<Json> list(){return store.list("memory");}
    void maintain(){for(auto& row:list())if(!row.value("explicit",false)&&!row.value("pinned",false)&&timestamp()-row.value("updated",0LL)>90*86400LL)store.erase("memory",row.at("id"));}
    void remember(const std::string& key,const std::string& value,bool explicitValue,const std::string& source,bool pinned=false,bool userConfirmed=false){
        auto normalized=memoryKey(key);if(!privateMemory(normalized,value))throw std::runtime_error("Sensitive or invalid preference");maintain();
        auto rows=list();std::string id;for(auto& row:rows)if(row.value("key","")==normalized){if(row.value("explicit",false)&&!explicitValue&&!userConfirmed)throw std::runtime_error("An inference cannot replace an explicit preference");id=row.at("id");break;}
        if(id.empty()){if(rows.size()>=200)throw std::runtime_error("Memory limit reached");id=randomID();}
        store.put("memory",id,{{"key",normalized},{"value",value},{"explicit",explicitValue},{"source",source},{"pinned",pinned},{"updated",timestamp()}});
    }
    void eraseChat(const std::string& chat){for(auto& row:list())if(row.value("source","")==chat&&!row.value("pinned",false))store.erase("memory",row.at("id"));}
    Json relevant(const std::string& query){Json out=Json::array();size_t bytes=0;auto q=lower(query);for(auto& row:list()){
        if(!row.value("explicit",false)&&timestamp()-row.value("updated",0LL)>90*86400LL)continue;
        auto key=row.value("key","");if(out.size()>=12)break;
        if(out.size()<4||q.find(lower(key))!=std::string::npos){Json item={{"preference",key},{"value",row.value("value","")},{"inferred",!row.value("explicit",false)}};bytes+=item.dump().size();if(bytes>4096)break;out.push_back(item);}
    }return out;}
};
inline bool safeAsset(const std::filesystem::path& path,bool app){
    if(!path.is_absolute()||path.wstring().rfind(L"\\\\",0)==0)return false;
    std::error_code error;auto resolved=std::filesystem::canonical(path,error);if(error)return false;
    auto attributes=GetFileAttributesW(path.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
    auto ext=lower(utf8(resolved.extension().wstring()));
    if(app)return ext==".exe"&&std::filesystem::is_regular_file(resolved,error);
    if(std::filesystem::is_directory(resolved,error))return true;
    for(auto permitted:{".txt",".md",".pdf",".png",".jpg",".jpeg",".gif",".webp",".bmp",".mp3",".wav",".flac",".ogg",".m4a",".mp4",".mkv",".mov",".webm",".csv",".json",".yaml",".yml",".log",".docx",".xlsx",".pptx",".odt",".ods",".odp",".rtf"})if(ext==permitted)return std::filesystem::is_regular_file(resolved,error);
    return false;
}
inline std::string utf8Excerpt(const std::string& value,size_t start,size_t limit){
    start=std::min(start,value.size());while(start<value.size()&&(static_cast<unsigned char>(value[start])&0xC0)==0x80)start++;
    auto end=std::min(value.size(),start+limit);while(end>start&&end<value.size()&&(static_cast<unsigned char>(value[end])&0xC0)==0x80)end--;
    return value.substr(start,end-start);
}
inline std::string urlEncode(const std::string& value){constexpr char h[]="0123456789ABCDEF";std::string out;for(unsigned char c:value){if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')out+=c;else{out+='%';out+=h[c>>4];out+=h[c&15];}}return out;}
}
