#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../src/agent-packages.h"
#include <cassert>
#include <iostream>
int main(){
    std::atomic<bool> cancel{false};auto search=hext::agentPackages("packages.search",{{"query","Microsoft.PowerShell"}},cancel);assert(search.value("success",false));auto text=search.value("text","");std::smatch match;
    if(!std::regex_search(text,match,std::regex(R"(Microsoft\.PowerShell\s+([0-9][A-Za-z0-9.+_-]*))"))){std::cerr<<"Could not identify a published version from actual WinGet search\n";return 2;}
    auto version=match[1].str();auto proposal=hext::agentPackages("packages.inspect",{{"id","Microsoft.PowerShell"},{"version",version}},cancel);
    if(!proposal.value("success",false)){std::cerr<<proposal.dump()<<"\n";return 3;}
    assert(proposal.contains("proposal")&&proposal.value("metadata","").find("[Microsoft.PowerShell]")!=std::string::npos&&proposal.value("version","")==version);
    // Deliberately impossible metadata guarantees rejection before the installer path.
    auto tampered=proposal;tampered["metadata"]="native-fixture-invalid-review";auto rejected=hext::agentPackages("packages.install",tampered,cancel);assert(!rejected.value("success",false)&&rejected.value("error","").find("changed")!=std::string::npos);
    auto install=hext::actionRequest({{"name","packages.install"},{"arguments",{{"proposal",proposal["proposal"]}}}});hext::AssistantPreferences preferences;preferences.autonomy=hext::Autonomy::Allowed;preferences.grants.fill(hext::Grant::Allow);hext::ActionGate gate;assert(install&&gate.evaluate(preferences,*install)==hext::Decision::Confirm);preferences.grants[static_cast<int>(hext::Category::Packages)]=hext::Grant::Deny;assert(gate.evaluate(preferences,*install)==hext::Decision::Denied);
    std::cout<<"PASS: actual signed WinGet search and pinned metadata inspection ("<<version<<"), native proposal, changed-review rejection before installation, mandatory consent and denied grant. No installer was run.\n";return 0;
}
