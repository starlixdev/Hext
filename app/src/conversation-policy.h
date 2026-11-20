#pragma once
#include "personal-core.h"
#include <regex>
namespace hext {
inline std::string conversationalInput(std::string value){
    if(!sensitiveText(value))return value;
    value=std::regex_replace(value,std::regex(R"((-----BEGIN [A-Z ]*PRIVATE KEY-----)[\s\S]*?(-----END [A-Z ]*PRIVATE KEY-----))",std::regex::icase),"[private key omitted]");
    value=std::regex_replace(value,std::regex(R"(((?:access_token|refresh_token|id_token|client_secret|api_key|apikey|password|senha|authorization)\s*[=:]\s*)(?:Bearer\s+)?[^\s,;}]+)",std::regex::icase),"[sensitive value omitted]");
    value=std::regex_replace(value,std::regex(R"(\b(?:sk-|ghp_|github_pat_)[A-Za-z0-9_\-]{16,}|\beyJ[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+\.[A-Za-z0-9_\-]+)"),"[token omitted]");
    // A question naming a field is not a credential. Keep its prose when no value is present.
    auto residual=std::regex_replace(value,std::regex(R"(\bclient_secret\b(?!\s*["']?\s*[:=]))",std::regex::icase),"publicly discussed field");
    return sensitiveText(residual)?"[Sensitive content omitted. The user needs assistance using the official sign-in flow.]":value;
}
inline bool suppliedPublicValue(const std::string& message,const std::string& value){
    if(value.empty()||value.size()>512||sensitiveText(value))return false;
    auto start=message.find(value);if(start==std::string::npos)return false;
    auto word=[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';};
    return (start==0||!word(message[start-1]))&&(start+value.size()==message.size()||!word(message[start+value.size()]));
}
}
