#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <iostream>
#include "../src/sound.h"
#include "../src/navigation.h"

int main(){
    NavigationOrder order;
    assert((order.items==std::array<int,7>{0,3,6,1,2,4,5}));
    order.move(5,0);assert(order.items.front()==5&&order.position(0)==1);
    order.move(5,6);assert(order.items.back()==5&&order.items.front()==0);
    order.move(6,-200);assert(order.items.front()==6);
    order.move(6,200);assert(order.items.back()==6);
    NavigationOrder restored;restored.restore(order.items);assert(restored.items==order.items);
    restored.restore({3,3,-1,500,0,6,6});
    assert((restored.items==std::array<int,7>{3,0,6,1,2,4,5}));
    for(int page=0;page<7;page++)assert(std::count(restored.items.begin(),restored.items.end(),page)==1);
    const std::wstring endpoint=L"{0.0.0.00000000}.{12345678-1234-5678-9abc-123456789abc}";
    hext::AudioEffects fx;fx.enabled=true;fx.gain=60;fx.bass=-25;fx.mid=10;fx.treble=35;
    auto config=fx.configuration(endpoint);
    assert(config.find("Device: {12345678-1234-5678-9abc-123456789abc}")!=std::string::npos);
    assert(config.find("Preamp: 6.0 dB")!=std::string::npos);
    assert(config.find("Gain -2.5 dB")!=std::string::npos);
    assert(config.find("Q 0.8")!=std::string::npos);
    assert(config.find("Stage: post-mix")!=std::string::npos);
    fx.enabled=false;config=fx.configuration(endpoint);
    assert(config.find("Preamp: 0.0 dB")!=std::string::npos&&config.find("Filter:")==std::string::npos);
    fx.enabled=true;fx.gain=-1;assert(fx.configuration(endpoint).empty());
    fx.gain=121;assert(fx.configuration(endpoint).empty());
    fx.gain=60;fx.bass=121;assert(fx.configuration(endpoint).empty());fx.bass=0;
    for(const auto& invalid:{L"",L"all",L"name\nInclude: arbitrary.dll",L"{12345678-1234-5678-9abc-123456789abg}"})assert(fx.configuration(invalid).empty());
    assert(fx.configuration(endpoint+L"\nInclude: arbitrary").empty());
    const auto directory=std::filesystem::current_path()/"dist"/("sound-config-test-"+std::to_string(GetTickCount64()));
    std::filesystem::create_directories(directory);
    const std::string original="# Existing user settings\nDevice: Another device\nPreamp: -8 dB\n";
    {std::ofstream out(directory/"config.txt",std::ios::binary);out<<original;}
    auto read=[](const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);return std::string((std::istreambuf_iterator<char>(in)),{});};
    assert(hext::ApoInstallation::writeConfiguration(directory,endpoint,fx)==0);
    auto saved=read(directory/"config.txt");
    // Existing device scopes must not suppress the Hext include; the original stays intact.
    assert(saved.find("Device: all\nInclude: Hext.txt")!=std::string::npos);
    assert(saved.find(original)==0&&read(directory/"config.before-hext.txt")==original);
    fx.gain=90;assert(hext::ApoInstallation::writeConfiguration(directory,endpoint,fx)==0);
    assert(read(directory/"config.txt")==saved&&read(directory/"config.before-hext.txt")==original);
    assert(read(directory/"Hext.txt").find("Preamp: 9.0 dB")!=std::string::npos);
    fx.enabled=false;assert(hext::ApoInstallation::writeConfiguration(directory,endpoint,fx)==0);
    assert(read(directory/"Hext.txt").find("Filter:")==std::string::npos);
    {std::ofstream out(directory/"Hext.txt",std::ios::binary);out<<"# Belongs to the user\n";}
    assert(hext::ApoInstallation::writeConfiguration(directory,endpoint,fx)==5);
    assert(read(directory/"Hext.txt")=="# Belongs to the user\n"&&read(directory/"config.txt")==saved);
    // Selection must not change the system default or start an audio stream.
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);assert(SUCCEEDED(com));
    {hext::SoundEngine engine;HRESULT hr=engine.refresh();assert(SUCCEEDED(hr));
        for(auto& device:engine.devices){assert(!device.id.empty()&&!device.name.empty());assert(device.flow==eRender||device.flow==eCapture);}
        auto output=engine.selected[0],input=engine.selected[1];engine.refresh();
        assert(output==engine.selected[0]&&input==engine.selected[1]);
        hext::ApoInstallation missing;missing.installed=false;
        assert(!missing.endpointConfigured(output));assert(missing.apply(endpoint,fx)==2);
        std::cout<<"PASS: navigation recovery, validated device-scoped effects, preserved configuration, backup, idempotence and collision protection; "<<engine.devices.size()<<" real endpoints read without changing audio\n";
    }
    CoUninitialize();
}
