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
    // Selection must not change the system default or start an audio stream.
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);assert(SUCCEEDED(com));
    {hext::SoundEngine engine;HRESULT hr=engine.refresh();assert(SUCCEEDED(hr));
        for(auto& device:engine.devices){assert(!device.id.empty()&&!device.name.empty());assert(device.flow==eRender||device.flow==eCapture);}
        auto output=engine.selected[0],input=engine.selected[1];engine.refresh();
        assert(output==engine.selected[0]&&input==engine.selected[1]);
        std::cout<<"PASS: sidebar navigation recovery and stable read-only device selection; "<<engine.devices.size()<<" real endpoints read without changing audio\n";
    }
    CoUninitialize();
}
