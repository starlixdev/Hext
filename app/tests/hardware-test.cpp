#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../src/hardware.h"
#include <cassert>
#include <iostream>
#include <limits>
int main(){
    constexpr size_t entrySize=1324;
    std::vector<unsigned char> data(32+4*entrySize);
    auto u32=[&](size_t at,std::uint32_t value){memcpy(data.data()+at,&value,4);};
    u32(0,0x4D41484D);u32(4,0x20000);u32(8,32);u32(12,4);u32(16,entrySize);u32(20,1000);
    auto entry=[&](int index,const char* name,const char* unit,std::uint32_t id,float value){
        size_t at=32+index*entrySize;memset(data.data()+at,0,entrySize);
        memcpy(data.data()+at,name,strlen(name));memcpy(data.data()+at+260,unit,strlen(unit));
        memcpy(data.data()+at+1300,&value,4);u32(at+1320,id);
    };
    entry(0,"CPU1 temperature","C",0x80,90);
    entry(1,"CPU Package","C",0x80,55);
    entry(2,"CPU power","W",0x100,84.2f);
    entry(3,"GPU power","W",0x61,150.5f);
    hext::HardwareSnapshot sample;
    assert(hext::readAfterburnerBuffer(data.data(),data.size(),1001,sample));
    assert(sample.cpuTemperature.available&&sample.cpuTemperature.value==55);
    assert(sample.cpuPower.available&&std::abs(sample.cpuPower.value-84.2)<.01);
    assert(sample.gpuPower.available&&sample.gpuPower.value==150.5);
    hext::HardwareSnapshot empty;
    assert(!hext::readAfterburnerBuffer(data.data(),data.size(),1010,empty));
    assert(!hext::readAfterburnerBuffer(data.data(),40,1001,empty));
    entry(0,"CPU temp","C",0x80,std::numeric_limits<float>::max());
    entry(1,"CPU Package","C",0x80,std::numeric_limits<float>::quiet_NaN());
    entry(2,"CPU power","%",0x100,80);
    entry(3,"GPU relative power","%",0x60,80);
    assert(hext::readAfterburnerBuffer(data.data(),data.size(),1001,empty));
    assert(!empty.cpuTemperature.available&&!empty.cpuPower.available&&!empty.gpuPower.available);
    u32(12,0xFFFFFFFF);
    assert(!hext::readAfterburnerBuffer(data.data(),data.size(),1001,empty));
    assert(hext::decimal(15.9)==L"15,9");
    assert(!hext::validReading(std::numeric_limits<double>::infinity(),0,100));
    HANDLE stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    {
        hext::HardwareReader reader;auto disabled=reader.sample(stop,0);
        assert(!disabled.nvidia&&!disabled.afterburner&&!disabled.wmi&&!disabled.gpuLoad.available);
    }
    CloseHandle(stop);
    std::cout<<"PASS: sensor layout bounds, stale samples, package selection, watts vs percent, missing values and formatting.\n";
}
