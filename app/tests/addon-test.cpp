#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <cassert>
#include <iostream>
#include "../src/audio-addon.h"
int main(int argc,char** argv){
    hext::SHA256 hash;const std::string abc="abc";assert(hash.update(abc.data(),abc.size()));
    assert(hash.finish()=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const auto missing=std::filesystem::current_path()/"dist"/"not-an-addon.exe";
    assert(!hext::verifiedAddon(missing));
    if(argc>1&&std::string(argv[1])=="--download"){
        hext::AudioAddon addon;assert(addon.start(std::filesystem::current_path()/"dist/addon-download-test"));
        auto started=GetTickCount64();
        while(addon.snapshot().state==hext::AddonState::Downloading&&GetTickCount64()-started<180000)Sleep(100);
        auto state=addon.snapshot();addon.stop();
        if(state.state!=hext::AddonState::Ready){std::wcerr<<state.message<<L"\n";return 1;}
        assert(hext::verifiedAddon(std::filesystem::current_path()/"dist/addon-download-test/Hext-Audio-Setup.exe"));
        std::cout<<"PASS: real GitHub add-on download, exact file size and pinned SHA-256; installer was not run\n";
    }
    std::cout<<"PASS: SHA-256 known-answer and unavailable installer rejection\n";
}
