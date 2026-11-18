#include <windows.h>
#include <iostream>
int main(){
    std::cout<<R"({"type":"turn.started"})"<<"\n"<<R"({"type":"item.completed","item":{"type":"agent_message","text":"{\"text\":\"done\",\"actions\":[],\"memory\":null}"}})"<<"\n"<<R"({"type":"turn.completed"})"<<std::endl;
    Sleep(30000);return 0;
}
