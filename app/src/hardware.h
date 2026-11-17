#pragma once
#include <windows.h>
#include <wbemidl.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cctype>
#include <cwctype>
#include <memory>
namespace hext {
enum Metric {CpuLoad,CpuTemperature,CpuPower,GpuLoad,GpuTemperature,GpuPower,Ram,MetricCount};
constexpr unsigned AllMetrics=(1u<<MetricCount)-1;
struct Reading {
    double value=0;
    bool available=false;
    std::wstring source;
};
struct HardwareSnapshot {
    Reading cpuTemperature,cpuPower,gpuLoad,gpuTemperature,gpuPower;
    std::wstring gpuName;
    bool nvidia=false,afterburner=false,wmi=false;
    ULONGLONG sampled=0;
};
inline bool validReading(double value,double low,double high){
    return std::isfinite(value)&&value>=low&&value<=high;
}
inline void offer(Reading& target,double value,double low,double high,const wchar_t* source){
    if(!target.available&&validReading(value,low,high))target={value,true,source};
}
inline std::wstring decimal(double value,int digits=1){
    wchar_t output[64];swprintf(output,64,digits?L"%.1f":L"%.0f",value);
    std::wstring result=output;std::replace(result.begin(),result.end(),L'.',L',');return result;
}
inline std::wstring lowerSensor(std::wstring value){
    std::transform(value.begin(),value.end(),value.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});
    return value;
}
// Interoperable v2 layout documented by the installed MSI Afterburner SDK.
// Read-only: Hext never writes to the mapping or alters hardware settings.
inline bool readAfterburnerBuffer(const void* data,size_t size,std::time_t now,HardwareSnapshot& snapshot){
    if(size<32)return false;
    const auto* bytes=static_cast<const unsigned char*>(data);
    auto u32=[&](size_t offset){std::uint32_t value=0;memcpy(&value,bytes+offset,4);return value;};
    if(u32(0)!=0x4D41484D||u32(4)<0x00020000)return false;
    size_t header=u32(8),count=u32(12),entrySize=u32(16);
    auto polled=static_cast<std::int32_t>(u32(20));
    if(header<32||header>size||entrySize<1324||count>4096||count>(size-header)/entrySize||
       now-polled>8||now-polled< -2)return false;
    Reading cpuTemp,cpuPower;bool packageTemperature=false,packagePower=false;
    for(size_t i=0;i<count;i++){
        const unsigned char* entry=bytes+header+i*entrySize;
        char rawName[261]{},rawUnits[261]{};memcpy(rawName,entry,260);memcpy(rawUnits,entry+260,260);
        std::string name=rawName,units=rawUnits;
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        float value=0;std::uint32_t id=0,gpu=0;
        memcpy(&value,entry+1300,4);memcpy(&gpu,entry+1316,4);memcpy(&id,entry+1320,4);
        bool package=name.find("package")!=std::string::npos||name.find("tctl")!=std::string::npos||
                     name.find("tdie")!=std::string::npos||name=="cpu temperature"||name=="cpu power";
        if(id==0x80&&validReading(value,-50,180)){
            if(!cpuTemp.available||(package&&!packageTemperature)||(package==packageTemperature&&value>cpuTemp.value)){
                cpuTemp={value,true,L"MSI Afterburner"};packageTemperature=package;
            }
        }
        if(id==0x100&&units=="W"&&validReading(value,0,2000)){
            if(!cpuPower.available||(package&&!packagePower)||(package==packagePower&&value>cpuPower.value)){
                cpuPower={value,true,L"MSI Afterburner"};packagePower=package;
            }
        }
        if(gpu!=0||snapshot.nvidia)continue;
        if(id==0)offer(snapshot.gpuTemperature,value,-50,180,L"MSI Afterburner");
        if(id==0x30)offer(snapshot.gpuLoad,value,0,100,L"MSI Afterburner");
        if(id==0x61&&units=="W")offer(snapshot.gpuPower,value,0,2000,L"MSI Afterburner");
    }
    if(cpuTemp.available&&!snapshot.cpuTemperature.available)snapshot.cpuTemperature=cpuTemp;
    if(cpuPower.available&&!snapshot.cpuPower.available)snapshot.cpuPower=cpuPower;
    snapshot.afterburner=true;return true;
}
class HardwareReader {
    HMODULE nvml=nullptr;
    void* gpu=nullptr;
    bool nvmlInitialized=false,nvmlActive=false,pdhInitialized=false;
    PDH_HQUERY query=nullptr;PDH_HCOUNTER counter=nullptr;
    IWbemLocator* locator=nullptr;
    struct WmiNamespace{IWbemServices* service=nullptr;ULONGLONG retry=0;};
    std::array<WmiNamespace,2> namespaces{};
    template<class F> F procedure(const char* name){
        F function=nullptr;FARPROC address=nvml?GetProcAddress(nvml,name):nullptr;
        static_assert(sizeof(function)==sizeof(address));memcpy(&function,&address,sizeof(function));return function;
    }
    void initializeNvml(){
        if(nvmlInitialized)return;
        nvmlInitialized=true;
        wchar_t system[32768]{};GetSystemDirectoryW(system,32768);
        auto path=std::wstring(system)+L"\\nvml.dll";nvml=LoadLibraryW(path.c_str());
        if(!nvml){
            wchar_t programs[32768]{};GetEnvironmentVariableW(L"ProgramW6432",programs,32768);
            path=std::wstring(programs)+L"\\NVIDIA Corporation\\NVSMI\\nvml.dll";nvml=LoadLibraryW(path.c_str());
        }
        auto init=procedure<int(*)()>("nvmlInit_v2");
        auto device=procedure<int(*)(unsigned,void**)>("nvmlDeviceGetHandleByIndex_v2");
        if(!init||!device)return;
        nvmlActive=init()==0;
        if(!nvmlActive||device(0,&gpu)!=0)gpu=nullptr;
    }
    void readNvidia(HardwareSnapshot& snapshot){
        initializeNvml();if(!gpu)return;
        snapshot.nvidia=true;
        auto name=procedure<int(*)(void*,char*,unsigned)>("nvmlDeviceGetName");
        char buffer[128]{};
        if(name&&name(gpu,buffer,sizeof(buffer))==0)snapshot.gpuName.assign(buffer,buffer+strlen(buffer));
        auto temperature=procedure<int(*)(void*,unsigned,unsigned*)>("nvmlDeviceGetTemperature");
        auto power=procedure<int(*)(void*,unsigned*)>("nvmlDeviceGetPowerUsage");
        struct Utilization{unsigned gpu,memory;};
        auto utilization=procedure<int(*)(void*,Utilization*)>("nvmlDeviceGetUtilizationRates");
        unsigned value=0;Utilization rates{};
        if(temperature&&temperature(gpu,0,&value)==0)offer(snapshot.gpuTemperature,value,-50,180,L"Driver NVIDIA");
        if(power&&power(gpu,&value)==0)offer(snapshot.gpuPower,value/1000.0,0,2000,L"Driver NVIDIA");
        if(utilization&&utilization(gpu,&rates)==0)offer(snapshot.gpuLoad,rates.gpu,0,100,L"Driver NVIDIA");
    }
    void readAfterburner(HardwareSnapshot& snapshot){
        HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"MAHMSharedMemory");
        if(!mapping)return;
        auto view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);
        if(view){
            MEMORY_BASIC_INFORMATION info{};
            if(VirtualQuery(view,&info,sizeof(info)))readAfterburnerBuffer(view,info.RegionSize,std::time(nullptr),snapshot);
            UnmapViewOfFile(view);
        }
        CloseHandle(mapping);
    }
    static std::wstring property(IWbemClassObject* object,const wchar_t* name){
        VARIANT value{};std::wstring result;
        if(SUCCEEDED(object->Get(name,0,&value,nullptr,nullptr))&&value.vt==VT_BSTR&&value.bstrVal)result=value.bstrVal;
        VariantClear(&value);return result;
    }
    void readWmi(HardwareSnapshot& snapshot,HANDLE stop){
        if(!locator&&FAILED(CoCreateInstance(CLSID_WbemLocator,nullptr,CLSCTX_INPROC_SERVER,IID_IWbemLocator,
            reinterpret_cast<void**>(&locator))))return;
        for(size_t index=0;index<namespaces.size();index++){
            if(WaitForSingleObject(stop,0)==WAIT_OBJECT_0)return;
            auto& current=namespaces[index];
            const wchar_t* path=index==0?L"ROOT\\LibreHardwareMonitor":L"ROOT\\OpenHardwareMonitor";
            const wchar_t* source=index==0?L"LibreHardwareMonitor":L"OpenHardwareMonitor";
            if(!current.service){
                if(GetTickCount64()<current.retry)continue;
                current.retry=GetTickCount64()+5000;BSTR name=SysAllocString(path);
                HRESULT result=locator->ConnectServer(name,nullptr,nullptr,nullptr,WBEM_FLAG_CONNECT_USE_MAX_WAIT,
                    nullptr,nullptr,&current.service);SysFreeString(name);
                if(FAILED(result)||!current.service)continue;
                CoSetProxyBlanket(current.service,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,nullptr,
                    RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE);
            }
            BSTR language=SysAllocString(L"WQL");
            BSTR sql=SysAllocString(L"SELECT Name,Identifier,Parent,SensorType,Value FROM Sensor WHERE SensorType='Temperature' OR SensorType='Power' OR SensorType='Load'");
            IEnumWbemClassObject* rows=nullptr;
            HRESULT result=current.service->ExecQuery(language,sql,WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,nullptr,&rows);
            SysFreeString(language);SysFreeString(sql);
            if(FAILED(result)||!rows){current.service->Release();current.service=nullptr;continue;}
            int tempRank=-1,powerRank=-1;Reading cpuTemp,cpuPower;
            bool allowGpu=!snapshot.nvidia&&!snapshot.gpuLoad.available&&!snapshot.gpuTemperature.available&&!snapshot.gpuPower.available;
            std::wstring selectedGpu;
            auto deadline=GetTickCount64()+300;
            for(int count=0;count<512&&GetTickCount64()<deadline;count++){
                if(WaitForSingleObject(stop,0)==WAIT_OBJECT_0)break;
                IWbemClassObject* item=nullptr;ULONG returned=0;
                result=rows->Next(50,1,&item,&returned);
                if(!returned){if(result==WBEM_S_TIMEDOUT)continue;break;}
                auto parent=lowerSensor(property(item,L"Parent"));
                auto identifier=lowerSensor(property(item,L"Identifier"));
                auto name=lowerSensor(property(item,L"Name")),type=property(item,L"SensorType");
                VARIANT raw{},converted{};double value=0;bool valid=false;
                if(SUCCEEDED(item->Get(L"Value",0,&raw,nullptr,nullptr))&&raw.vt!=VT_NULL&&raw.vt!=VT_EMPTY&&
                   SUCCEEDED(VariantChangeType(&converted,&raw,0,VT_R8))){value=converted.dblVal;valid=std::isfinite(value);}
                VariantClear(&raw);VariantClear(&converted);item->Release();if(!valid)continue;
                snapshot.wmi=true;
                bool cpu=(parent.find(L"cpu")!=std::wstring::npos||identifier.find(L"cpu")!=std::wstring::npos)&&
                         parent.find(L"gpu")==std::wstring::npos&&identifier.find(L"gpu")==std::wstring::npos;
                bool package=name.find(L"package")!=std::wstring::npos||name.find(L"tctl")!=std::wstring::npos||name.find(L"tdie")!=std::wstring::npos;
                int rank=package?3:name.find(L"total")!=std::wstring::npos?2:1;
                // These WMI providers encode a missing nullable reading as 0.
                // Keep ambiguous zero temperature/power unavailable.
                if(cpu&&type==L"Temperature"&&value!=0&&validReading(value,-50,180)&&
                   (rank>tempRank||(rank==tempRank&&value>cpuTemp.value))){cpuTemp={value,true,source};tempRank=rank;}
                if(cpu&&type==L"Power"&&value>0&&validReading(value,0,2000)&&
                   (rank>powerRank||(rank==powerRank&&value>cpuPower.value))){cpuPower={value,true,source};powerRank=rank;}
                if(allowGpu&&parent.find(L"gpu")!=std::wstring::npos){
                    if(selectedGpu.empty())selectedGpu=parent;
                    if(selectedGpu!=parent)continue;
                    if(type==L"Temperature"&&value!=0&&(name.find(L"core")!=std::wstring::npos||name.find(L"gpu")!=std::wstring::npos))
                        offer(snapshot.gpuTemperature,value,-50,180,source);
                    if(type==L"Power"&&value>0&&(package||name.find(L"total")!=std::wstring::npos))
                        offer(snapshot.gpuPower,value,0,2000,source);
                    if(type==L"Load"&&name.find(L"core")!=std::wstring::npos)offer(snapshot.gpuLoad,value,0,100,source);
                }
            }
            rows->Release();
            if(cpuTemp.available&&!snapshot.cpuTemperature.available)snapshot.cpuTemperature=cpuTemp;
            if(cpuPower.available&&!snapshot.cpuPower.available)snapshot.cpuPower=cpuPower;
            if(snapshot.cpuTemperature.available&&snapshot.cpuPower.available)break;
        }
    }
    void readGpuCounter(HardwareSnapshot& snapshot){
        if(snapshot.nvidia||snapshot.gpuLoad.available||snapshot.gpuTemperature.available||snapshot.gpuPower.available)return;
        if(!pdhInitialized){
            pdhInitialized=true;
            if(PdhOpenQueryW(nullptr,0,&query)==ERROR_SUCCESS&&
               PdhAddEnglishCounterW(query,L"\\GPU Engine(*)\\Utilization Percentage",0,&counter)==ERROR_SUCCESS)
                PdhCollectQueryData(query);
            return;
        }
        if(!query||!counter||PdhCollectQueryData(query)!=ERROR_SUCCESS)return;
        DWORD bytes=0,count=0;
        if(PdhGetFormattedCounterArrayW(counter,PDH_FMT_DOUBLE,&bytes,&count,nullptr)!=static_cast<PDH_STATUS>(PDH_MORE_DATA)||bytes>8*1024*1024)return;
        std::vector<unsigned char> buffer(bytes);
        auto items=reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if(PdhGetFormattedCounterArrayW(counter,PDH_FMT_DOUBLE,&bytes,&count,items)!=ERROR_SUCCESS)return;
        std::map<std::wstring,double> engines;
        for(DWORD i=0;i<count;i++){
            if(items[i].FmtValue.CStatus!=PDH_CSTATUS_VALID_DATA&&items[i].FmtValue.CStatus!=PDH_CSTATUS_NEW_DATA)continue;
            std::wstring name=items[i].szName?items[i].szName:L"";auto key=name.find(L"_luid_");
            if(key==std::wstring::npos)continue;
            double value=items[i].FmtValue.doubleValue;
            if(std::isfinite(value)&&value>=0)engines[name.substr(key)]+=value;
        }
        if(!engines.empty()){
            double busiest=0;for(auto& engine:engines)busiest=std::max(busiest,engine.second);
            offer(snapshot.gpuLoad,std::min(100.0,busiest),0,100,L"Windows");
        }
    }
public:
    HardwareSnapshot sample(HANDLE stop,unsigned mask){
        HardwareSnapshot result;
        if(mask&56)readNvidia(result);
        auto missing=[&](){
            return ((mask&(1u<<CpuTemperature))&&!result.cpuTemperature.available)||
                ((mask&(1u<<CpuPower))&&!result.cpuPower.available)||
                ((mask&(1u<<GpuLoad))&&!result.gpuLoad.available)||
                ((mask&(1u<<GpuTemperature))&&!result.gpuTemperature.available)||
                ((mask&(1u<<GpuPower))&&!result.gpuPower.available);
        };
        if(missing())readAfterburner(result);
        if(missing())readWmi(result,stop);
        if(mask&(1u<<GpuLoad))readGpuCounter(result);
        result.sampled=GetTickCount64();return result;
    }
    ~HardwareReader(){
        for(auto& current:namespaces)if(current.service)current.service->Release();
        if(locator)locator->Release();
        if(query)PdhCloseQuery(query);
        if(nvml){auto shutdown=procedure<int(*)()>("nvmlShutdown");if(nvmlActive&&shutdown)shutdown();FreeLibrary(nvml);}
    }
};
class HardwareMonitor {
    HANDLE stopEvent=nullptr,requestEvent=nullptr,worker=nullptr;
    DWORD workerId=0;
    volatile LONG requestedMask=AllMetrics;
    SRWLOCK lock=SRWLOCK_INIT;
    HardwareSnapshot latest;
    static DWORD WINAPI run(void* context){
        auto& self=*static_cast<HardwareMonitor*>(context);
        CoInitializeEx(nullptr,COINIT_MULTITHREADED);CoEnableCallCancellation(nullptr);
        {
            std::unique_ptr<HardwareReader> reader;
            HANDLE events[]={self.stopEvent,self.requestEvent};
            for(;;){
                DWORD event=WaitForMultipleObjects(2,events,FALSE,reader?12000:INFINITE);
                if(event==WAIT_TIMEOUT){reader.reset();continue;}
                if(event!=WAIT_OBJECT_0+1)break;
                if(!reader)reader=std::make_unique<HardwareReader>();
                auto value=reader->sample(self.stopEvent,static_cast<unsigned>(InterlockedCompareExchange(&self.requestedMask,0,0)));
                AcquireSRWLockExclusive(&self.lock);self.latest=std::move(value);ReleaseSRWLockExclusive(&self.lock);
            }
        }
        CoDisableCallCancellation(nullptr);CoUninitialize();return 0;
    }
public:
    void request(unsigned mask=AllMetrics){
        InterlockedExchange(&requestedMask,static_cast<LONG>(mask&AllMetrics));
        if(!worker){
            stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);requestEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
            if(stopEvent&&requestEvent)worker=CreateThread(nullptr,0,run,this,0,&workerId);
        }
        if(worker)SetEvent(requestEvent);
    }
    HardwareSnapshot snapshot(){
        AcquireSRWLockShared(&lock);auto value=latest;ReleaseSRWLockShared(&lock);return value;
    }
    void stop(){
        if(worker){SetEvent(stopEvent);CoCancelCall(workerId,0);WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=nullptr;}
        if(stopEvent){CloseHandle(stopEvent);stopEvent=nullptr;}
        if(requestEvent){CloseHandle(requestEvent);requestEvent=nullptr;}
    }
    ~HardwareMonitor(){stop();}
};
}
