#define UNICODE
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
int main(){
    wchar_t executable[32768]{};if(!GetModuleFileNameW(nullptr,executable,32768))return 1;
    std::ofstream marker(std::filesystem::path(executable).parent_path()/L"native-action-observed.txt");
    marker<<"Hext launched an explicitly registered test application.\n";return marker?0:2;
}
