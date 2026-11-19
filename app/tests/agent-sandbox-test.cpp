#define UNICODE
#define _UNICODE
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../src/agent-process.h"
#include <iostream>
int main(){
    using namespace hext;std::atomic<bool> cancel{false};auto id=randomID();auto privateRoot=std::filesystem::current_path()/"app/dist"/("sandbox-private-"+randomID());if(!privateDirectory(privateRoot))return 2;
    std::ofstream(privateRoot/"canary.txt")<<"generated-private-canary";
    auto quote=[](const std::filesystem::path& path){std::string value=utf8(path.wstring());size_t offset=0;while((offset=value.find('\'',offset))!=std::string::npos){value.insert(offset,1,'\'');offset+=2;}return "'"+value+"'";};
    auto script="[IO.File]::WriteAllText([IO.Path]::Combine($env:TEMP,'answer.txt'),'native-sandbox');[IO.File]::ReadAllText([IO.Path]::Combine($env:TEMP,'answer.txt'));try{[void][IO.File]::ReadAllText("+quote(privateRoot/"canary.txt")+");'PRIVATE-READ-FAILED'}catch{'PRIVATE-READ-BLOCKED'};try{[IO.File]::WriteAllText("+quote(privateRoot/"forbidden.txt")+",'bad');'OUTSIDE-WRITE-FAILED'}catch{'OUTSIDE-WRITE-BLOCKED'};try{$taskSocket=[Net.Sockets.TcpClient]::new();$taskConnect=$taskSocket.BeginConnect('1.1.1.1',443,$null,$null);if($taskConnect.AsyncWaitHandle.WaitOne(5000)){$taskSocket.EndConnect($taskConnect);'NETWORK-UNEXPECTEDLY-CONNECTED'}else{'NETWORK-TIMEOUT'}}catch{'NETWORK-ERROR-'+$_.Exception.GetBaseException().NativeErrorCode}finally{if($taskSocket){$taskSocket.Dispose()}}";
    auto result=sandboxPowerShell(id,script,cancel);std::cout<<"exit="<<result.code<<" output="<<result.output<<" error="<<result.error<<"\n";
    bool passed=!result.code&&result.output.find("native-sandbox")!=std::string::npos&&result.output.find("PRIVATE-READ-BLOCKED")!=std::string::npos&&result.output.find("OUTSIDE-WRITE-BLOCKED")!=std::string::npos&&result.output.find("NETWORK-ERROR-10013")!=std::string::npos&&!std::filesystem::exists(privateRoot/"forbidden.txt");
    auto again=sandboxPowerShell(id,"[IO.File]::ReadAllText([IO.Path]::Combine($env:TEMP,'answer.txt'))",cancel);passed=passed&&!again.code&&again.output.find("native-sandbox")!=std::string::npos;
    passed=passed&&SUCCEEDED(DeleteAppContainerProfile((L"Hext.Task."+wide(id)).c_str()));if(passed)std::cout<<"PASS: real AppContainer script, persistent own workspace, denied private-file read, denied external write and Windows network access denied (10013).\n";return passed?0:1;
}
