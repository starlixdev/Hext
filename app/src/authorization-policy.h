#pragma once
// Included inside namespace hext after the typed tool registry.
enum class Decision {Denied,Confirm,Review,Execute};
enum class ExecutionAuthority {None,User,Reviewer};
inline bool sensitiveText(const std::string& text){
    auto value=lower(text);value.erase(std::remove(value.begin(),value.end(),'\\'),value.end());
    for(auto marker:{"-----begin private key","-----begin rsa private key","access_token=","refresh_token=","id_token=","api_key=","apikey=","authorization: bearer ","password=","password:","senha=","senha:","client_secret","\"access_token\"","\"refresh_token\"","\"api_key\"","\"password\"","card number:","credit_card","cartão:","cartao:","cpf:"})if(value.find(marker)!=std::string::npos)return true;
    for(auto prefix:{"sk-","ghp_","github_pat_","eyj"}){
        size_t pos=0;while((pos=value.find(prefix,pos))!=std::string::npos){
            auto tokenChar=[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';};
            size_t end=pos;while(end<value.size()&&tokenChar(value[end]))end++;
            if((pos==0||!tokenChar(value[pos-1]))&&end-pos>=24)return true;
            pos++;
        }
    }
    return false;
}
inline bool protectedUIContext(const std::string& text){
    auto value=lower(text);
    for(auto token:{"delete","excluir","apagar","permanently","permanente","formatar","format disk","factory reset","restaurar fábrica","transferir","transfer funds","checkout","comprar","purchase","pagamento","pagar","payment","pay now","pix","bank","banco","paypal","nubank","itau","itaú","bradesco","santander","saldo","extrato","fatura","account number","credit card","cartão","cartao","senha","password","security code","código de segurança","private key","chave privada","credential","credencial","permission","permissão","administrator","administrador","firewall","registry","registro do windows","bios","firmware"})if(value.find(token)!=std::string::npos)return true;
    return sensitiveText(value);
}
inline bool benignUIOperation(const ActionRequest& request,const std::string& label){
    auto operation=request.args.value("operation","");
    if(operation=="scroll_up"||operation=="scroll_down"||operation=="expand"||operation=="collapse")return true;
    if(operation=="set_value")return !sensitiveText(request.args.value("text",""));
    auto name=lower(label);
    for(auto safe:{"search","pesquisar","buscar","play","pause","pausar","reproduzir","next","previous","próximo","anterior","refresh","atualizar visualização"})if(name==safe)return true;
    return false; // Unknown invocations may send, delete or pay; user consent remains required.
}
inline bool intrinsicConsent(const ActionRequest& request){
    if(request.sensitive||request.name=="packages.install"||request.name=="history.search")return true;
    if(request.name=="terminal.run"){auto script=lower(request.args.value("script",""));for(auto destructive:{"remove-item","clear-content","::delete(",".delete(","format-volume","format-disk","clear-disk","remove-partition","set-mppreference","disable-windowsoptionalfeature"})if(script.find(destructive)!=std::string::npos)return true;}
    if(sensitiveText(request.args.dump()))return true;
    if(request.name=="web.read"&&request.args.value("url","").find('?')!=std::string::npos)return true;
    return false;
}
inline std::string authorizationFingerprint(const ActionRequest& request){return request.name+":"+request.args.dump()+":"+std::to_string(request.sensitive)+":"+std::to_string(static_cast<int>(request.origin))+":"+request.nativeTarget;}
inline std::optional<std::pair<std::string,std::string>> reviewVerdict(const std::string& output,int provider){
    auto parsed=parseJSON(output);if(parsed&&provider==2&&parsed->is_object()){if(parsed->value("is_error",false))return {};if(parsed->contains("structured_output")){auto structured=(*parsed)["structured_output"];*parsed=std::move(structured);}}
    if(!parsed||!fields(*parsed,{"decision","reason"})||!textField(*parsed,"decision",16)||!textField(*parsed,"reason",512,true))return {};
    auto decision=(*parsed)["decision"].get<std::string>();if(decision!="approve"&&decision!="ask"&&decision!="deny")return {};
    return std::make_pair(decision,(*parsed)["reason"].get<std::string>());
}
struct ActionTicket {std::string nonce,chat;ActionRequest request;unsigned revision=0;ULONGLONG expires=0;ExecutionAuthority authority=ExecutionAuthority::User;};
class ActionGate {
    std::optional<ActionTicket> pending,permit;
    std::optional<ActionRequest> consumeTicket(const std::string& nonce,const AssistantPreferences& p,const std::string& chat,ExecutionAuthority authority){
        if(!pending)return {};
        auto ticket=*pending;pending.reset();permit.reset();
        auto decision=evaluate(p,ticket.request);
        if(ticket.authority!=authority||ticket.nonce!=nonce||ticket.chat!=chat||ticket.revision!=p.revision||GetTickCount64()>ticket.expires||decision==Decision::Denied)return {};
        if(authority==ExecutionAuthority::Reviewer&&(p.mode!=PermissionMode::Delegate||decision!=Decision::Review))return {};
        permit=ticket;return ticket.request;
    }
public:
    Decision evaluate(const AssistantPreferences& p,const ActionRequest& r)const{
        auto canonical=actionRequest(Json{{"name",r.name},{"arguments",r.args}});if(!canonical||canonical->category!=r.category||p.blocked)return Decision::Denied;
        if(!r.nativeTarget.empty()&&(r.nativeTarget!="spotify"||r.name!="media.control"))return Decision::Denied;
        if(r.origin==ActionOrigin::Voice&&(r.category==Category::Spotify||r.nativeTarget=="spotify"))return Decision::Denied;
        auto definition=toolRegistry().find(r.name);if(definition&&definition->metadata)return Decision::Execute;
        if(intrinsicConsent(r)||intrinsicConsent(*canonical))return Decision::Confirm;
        return p.mode==PermissionMode::Ask?Decision::Confirm:p.mode==PermissionMode::Delegate?Decision::Review:Decision::Execute;
    }
    ActionTicket propose(const AssistantPreferences& p,const ActionRequest& request,const std::string& chat,ExecutionAuthority authority=ExecutionAuthority::User){permit.reset();pending=ActionTicket{randomID(),chat,request,p.revision,GetTickCount64()+120000,authority};return *pending;}
    std::optional<ActionRequest> consume(const std::string& nonce,const AssistantPreferences& p,const std::string& chat){return consumeTicket(nonce,p,chat,ExecutionAuthority::User);}
    std::optional<ActionRequest> consumeReview(const std::string& nonce,const AssistantPreferences& p,const std::string& chat){return consumeTicket(nonce,p,chat,ExecutionAuthority::Reviewer);}
    bool authorizeExecution(const AssistantPreferences& p,const ActionRequest& request,const std::string& chat,ExecutionAuthority authority){
        auto decision=evaluate(p,request);if(decision==Decision::Denied)return false;
        if(authority==ExecutionAuthority::None)return decision==Decision::Execute;
        if(!permit)return false;
        auto ready=*permit;permit.reset();
        if(ready.authority!=authority||ready.chat!=chat||ready.revision!=p.revision||GetTickCount64()>ready.expires||authorizationFingerprint(ready.request)!=authorizationFingerprint(request))return false;
        return authority==ExecutionAuthority::User||(authority==ExecutionAuthority::Reviewer&&decision==Decision::Review&&p.mode==PermissionMode::Delegate);
    }
    void revoke(){pending.reset();permit.reset();}
};
