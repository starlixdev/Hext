#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <string>
#include <iostream>
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;
    bool write=std::wstring(argv[1])==L"write";
    HANDLE file=CreateFileW(argv[2],write?GENERIC_WRITE:GENERIC_READ,FILE_SHARE_READ,nullptr,write?CREATE_NEW:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){std::cout<<"DENIED "<<GetLastError()<<"\n";return 10;}
    CloseHandle(file);std::cout<<"UNEXPECTED_ACCESS\n";return 0;
}
