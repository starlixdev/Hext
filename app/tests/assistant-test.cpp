#define NOMINMAX
#include "../src/assistant.h"
#include <shellapi.h>
#include <iostream>
#include <cassert>
int wmain(int argc,wchar_t** argv){
    bool child=GetFileType(GetStdHandle(STD_INPUT_HANDLE))==FILE_TYPE_DISK;
    if(child){
        if(argc>1&&std::wstring(argv[1])==L"--version"){std::cout<<"codex-cli 0.1; Claude Code 2.1";return 0;}
        if(argc>1&&(std::wstring(argv[1])==L"--help"||(std::wstring(argv[1])==L"exec"&&argc>2&&std::wstring(argv[2])==L"--help"))){
            std::cout<<"--output-last-message --sandbox --ephemeral --output-format --permission-mode --no-session-persistence --output-schema --ignore-user-config --tools --json-schema --strict-mcp-config --setting-sources --safe-mode --restricted --settings";return 0;
        }
        if(argc>2&&std::wstring(argv[1])==L"login"&&std::wstring(argv[2])==L"status"){
            std::cerr<<"Logged in using ChatGPT";return 0;
        }
        if(argc>2&&std::wstring(argv[1])==L"auth"&&std::wstring(argv[2])==L"status"){
            std::cout<<"{\"loggedIn\": true, \"authMethod\": \"claude.ai\"}";return 0;
        }
        std::string input((std::istreambuf_iterator<char>(std::cin)),{});
        if(input=="SLEEP"){Sleep(10000);return 0;}
        if(input=="FAIL"){std::cerr<<"simulated error";return 7;}
        std::string answer="reply:"+input;
        for(int i=1;i+1<argc;i++)if(std::wstring(argv[i])==L"--output-last-message"){
            std::ofstream file(std::filesystem::path(argv[i+1]),std::ios::binary);file<<answer;return 0;
        }
        std::cout<<answer;return 0;
    }
    wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);
    auto work=std::filesystem::path(exe).parent_path()/"dist"/"assistant-test-work";
    hext::AssistantBridge bridge;
    auto wait=[&](){
        for(int i=0;i<250&&bridge.busy;i++){bridge.poll();Sleep(10);}
        assert(!bridge.busy);
    };
    std::wstring prompt=L"Olá, çã \" & | texto";auto encoded=hext::utf8(prompt);
    assert(hext::wide(encoded)==prompt);
    for(int provider:{1,2,3}){
        assert(bridge.start(exe,provider,prompt,work));wait();
        assert(bridge.exitCode==0&&bridge.result=="reply:"+encoded);
    }
    for(int provider:{1,2}){
        assert(bridge.start(exe,provider,L"",work,hext::AssistantTask::Version));wait();
        assert(hext::providerVersion(provider,bridge.result));
        assert(bridge.start(exe,provider,L"",work,hext::AssistantTask::Capabilities));wait();
        assert(hext::providerCapabilities(provider,bridge.result));
        assert(bridge.start(exe,provider,L"",work,hext::AssistantTask::Authentication));wait();
        assert(hext::providerAuthenticated(provider,bridge.exitCode,bridge.result+bridge.diagnostic));
    }
    assert(!hext::providerAuthenticated(1,1,"Logged in using ChatGPT"));
    assert(!hext::providerAuthenticated(1,0,"Not logged in"));
    assert(!hext::providerAuthenticated(2,0,"{\"loggedIn\":false,\"authMethod\":\"oauth_token\"}"));
    assert(!hext::providerAuthenticated(2,0,"{}"));
    assert(!hext::providerVersion(1,"Claude Code 2.1"));
    assert(!hext::providerCapabilities(1,"--sandbox"));
    assert(bridge.start(exe,3,L"FAIL",work));wait();assert(bridge.exitCode==7);
    assert(bridge.start(exe,3,L"SLEEP",work));
    HANDLE childProcess=OpenProcess(SYNCHRONIZE,FALSE,GetProcessId(bridge.process));
    bridge.cancel();assert(!bridge.busy&&!bridge.process);
    assert(childProcess&&WaitForSingleObject(childProcess,1000)==WAIT_OBJECT_0);CloseHandle(childProcess);
    assert(!bridge.start(L"Z:\\missing\\assistant.exe",3,L"no",work));
    std::wstring argument=L"C:\\a path\\quote\"\\";
    auto command=L"test "+hext::quoteArgument(argument);int count=0;
    auto parsed=CommandLineToArgvW(command.c_str(),&count);
    assert(parsed&&count==2&&argument==parsed[1]);LocalFree(parsed);
    for(auto& entry:std::filesystem::directory_iterator(work))assert(entry.path().extension()!=L".tmp");
    std::cout<<"PASS: adapters, dependency/auth probes, negative auth states, UTF-8, stdin, quoting and cancellation. No inference request.\n";
    return 0;
}
