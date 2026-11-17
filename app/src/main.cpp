#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#define WINVER 0x0A00
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <shellapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <dwmapi.h>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <set>
#include "motion.h"
#include "visibility.h"
#include "desktop-region.h"
#include "presentation.h"
#include "material.h"
#include "color.h"
#include "assistant.h"
#include "hardware.h"
#include "navigation.h"
#include "sound.h"
#include "audio-addon.h"
#include "shortcuts.h"

template<class T> void release(T*& p) {if(p){p->Release();p=nullptr;}}
constexpr UINT VolumeMessage=WM_APP+2, TrayMessage=WM_APP+3;
constexpr UINT ForegroundMessage=WM_APP+4;
constexpr int MaxW=2400, MaxH=1700;
constexpr float PanelW=736,PanelH=470;
HWND windowHandle;
ID2D1Factory* factory;
ID2D1DCRenderTarget* canvas;
ID2D1SolidColorBrush* brush;
IDWriteFactory* textFactory;
std::map<int,IDWriteTextFormat*> fonts;
HDC memoryDC;
HBITMAP dib, previousBitmap;
void* pixels;
float dpi=1, zoom=1;
RECT monitorRect;
HMONITOR currentMonitor;
std::vector<HMONITOR> monitors;
std::wstring configPath;
Spring width{184,0,184},height{42,0,42},reveal,pageFade,appearance;
Spring navPosition{94,0,94},popoverMotion,tooltipMotion;
Spring settingsFade{1,0,1};
Spring colorPickerMotion;
hext::AccentPalette accentPalette;
hext::HSV draftColor;
bool colorPickerOpen=false,pendingDesktopFocus=false;
int dragColor=0;
ID2D1Bitmap* colorWheelBitmap=nullptr;
hext::AssistantBridge assistantBridge;
hext::AssistantBridge connectionProbe;
hext::ConnectionState connectionState[4]{};
unsigned connectionChecks[4]{};
hext::AssistantTask connectionStage=hext::AssistantTask::Version;
int probingProvider=0,loginProvider=0;
HANDLE loginProcess=nullptr;
int assistantProvider=0;
std::wstring assistantPath[4],assistantInput,assistantStatus;
bool assistantFocused=false;
bool assistantCaretVisible=false;
size_t assistantCaret=0;
float assistantTextScroll=0,lastAssistantCaretX=218;
float assistantScroll=0,assistantScrollMax=0;
struct ChatMessage{bool user;std::wstring text;};
std::vector<ChatMessage> conversation;
int settingsSection=5,settingsReturnPage=0;
hext::Shortcuts shortcuts;
int captureShortcut=-1;
HHOOK shortcutCaptureHook=nullptr;
std::array<bool,256> shortcutKeys{};
std::wstring shortcutStatus[2];
ULONGLONG shortcutCaptureUntil=0,shortcutIgnoreUntil=0;
bool shortcutAwaitRelease=false;
hext::Shortcut shortcutReleaseGuard;
bool shortcutOverlay=false,shortcutClosing=false,shortcutHold=false;
DWORD shortcutLayerError=0;
HWND shortcutReturnWindow=nullptr;
Spring timerX{590,0,590},timerY{294,0,294},timerSize{34,0,34},timerSymbol;
Spring timerButtonX{224,0,224},timerButtonY{365,0,365},timerButtonW{138,0,138},timerButtonH{30,0,30};
Spring volumeVisual,cpuVisual,ramVisual;
Spring sendState;
hext::HardwareMonitor hardwareMonitor;
hext::HardwareSnapshot hardwareSnapshot;
unsigned metricMask=hext::AllMetrics;
int hardwareInterval=1000;
ULONGLONG lastHardwareRequest=0;
ULONGLONG lastSystemSample=0;
bool cpuSampleReady=false,ramSampleReady=false;
struct ControlMotion {Spring hover;Spring press{1,0,1};};
struct NumberMotion {std::wstring oldValue,value;Spring progress{1,0,1};};
std::map<int,ControlMotion> controlMotion;
struct ToggleMotion {Spring position;bool initialized=false;};
std::map<int,ToggleMotion> toggleMotion;
NavigationOrder navigation,dragOriginalNavigation;
std::array<Spring,7> navigationRows;
hext::SoundEngine sound;
hext::BluetoothDevices bluetooth;
hext::BluetoothSnapshot bluetoothSnapshot;
hext::ApoInstallation audioApo;
hext::AudioEffects audioEffects;
hext::AudioAddon audioAddon;
int soundTab=0,soundChooser=-1,soundDeviceOffset=0,bluetoothOffset=0,bluetoothSelected=-1,dragSound=0;
Spring soundTabVisual,soundPageFade{1,0,1};
bool apoEndpointReady=false,soundEffectsDirty=false;
ULONGLONG lastBluetoothRefresh=0;
HANDLE audioEffectsProcess=nullptr;
std::wstring soundStatus;
int navigationCandidate=-1;
bool navigationHeld=false,navigationDragging=false;
float navigationPointerY=0,navigationGrab=0;
POINT navigationDown{};
std::map<int,NumberMotion> numberMotion;
std::map<int,IDWriteTextFormat*> iconFormats;
std::wstring uiFont=L"Segoe UI",iconFont=L"Segoe MDL2 Assets";
hext::WallpaperMaterial wallpaper;
bool materialDirty=true,translucent=true,systemTransparency=true,highContrast=false;
bool popoverOpen=false;
int settingsMenu=0;
bool animationClock=false,preciseClock=false;
int motionStyle=1;
ULONGLONG lastMaterialCheck=0;
void readAppearancePreferences(){
    HIGHCONTRAST contrast{sizeof(contrast)};
    highContrast=SystemParametersInfo(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0)&&
                 (contrast.dwFlags&HCF_HIGHCONTRASTON);
    DWORD value=1,bytes=sizeof(value);
    if(RegGetValue(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"EnableTransparency",RRF_RT_REG_DWORD,nullptr,&value,&bytes)==ERROR_SUCCESS)systemTransparency=value!=0;
}
bool motionBusy();
void stepMotion(double seconds);
void snapMotion();
void closePopover();
void openPopover();
void openSettingsMenu(int kind);
void openColorPicker();
void closeColorPicker();
void symbol(wchar_t,float,float,float,UINT,float alpha=1);
void contentText(int,const std::wstring&,float,float,float,int,UINT,bool);
void notifyAccessibility();
void render();
void requestHardware(bool force=false);
void finishNavigationDrag(bool commit);
void cancelShortcutCapture();
void finishShortcutOverlay();
void toggleShortcutPanel();
DesktopVisibility visibility;
DesktopEntry desktopEntry;
HWINEVENTHOOK foregroundHook;
HWND desktopHost;
ULONGLONG lastDesktopOrderCheck=0;
bool desktopEligible=false,windowShown=false,menuOpen=false,testMode=false;
bool expanded=false,floating=false,reducedMotion=false,systemReducedMotion=false;
bool motionReduced(){return reducedMotion||systemReducedMotion;}
bool mouseInside=false,inputFocused=false,voicePreview=false,hidden=false,selectAll=false;
bool dragVolume=false;
int page=0,previousPage=0,hoverId=-1,keyboardId=-1,pressedId=-1;
UINT taskbarCreated;
HRESULT lastRenderResult=E_FAIL;
BOOL lastUpdateSucceeded=FALSE;
BYTE lastPresentedAlpha=0;
HICON appIcon;
ULONGLONG lastFrame=0,paintUntil=0,hoverSince=0,previewUntil=0,stripUntil=0;
std::wstring stripText,command,feedback;
float cpuPercent=0,ramPercent=0,volume=0;
float ramTotalGB=0,ramFreeGB=0;
bool audioReady=false;
BOOL audioMuted=FALSE;
ULONGLONG previousIdle=0,previousKernel=0,previousUser=0;
ULONGLONG timerEnd=0,timerRemaining=300000,timerDuration=300000;
bool timerRunning=false;
std::vector<std::wstring> files;
IMMDeviceEnumerator* enumerator;
IAudioEndpointVolume* audio;

struct VolumeCallback:IAudioEndpointVolumeCallback {
    virtual ~VolumeCallback() = default;
    LONG refs=1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** p) override {
        if(!p)return E_POINTER;
        if(id==__uuidof(IUnknown)||id==__uuidof(IAudioEndpointVolumeCallback)) {
            *p=static_cast<IAudioEndpointVolumeCallback*>(this);AddRef();return S_OK;
        }
        *p=nullptr;return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return InterlockedIncrement(&refs);}
    ULONG STDMETHODCALLTYPE Release() override {ULONG n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA data) override {
        if(windowHandle)PostMessage(windowHandle,VolumeMessage,static_cast<WPARAM>(data->fMasterVolume*10000),data->bMuted);
        return S_OK;
    }
};
VolumeCallback* volumeCallback;
struct Hit {int id;D2D1_RECT_F rect;std::wstring name;};
std::vector<Hit> hits;
D2D1_COLOR_F color(UINT value,float alpha=1){return D2D1::ColorF(value,alpha);}
UINT accent(){
    return colorPickerOpen?hext::rgb(draftColor):accentPalette.current();
}
void use(UINT c,float a=1){brush->SetColor(color(c,a));}
void box(float x,float y,float w,float h,UINT c,float radius=12,float a=1){
    use(c,a);canvas->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x,y,x+w,y+h),radius,radius),brush);
}
void line(float x1,float y1,float x2,float y2,UINT c,float size=1,float a=1){
    use(c,a);canvas->DrawLine(D2D1::Point2F(x1,y1),D2D1::Point2F(x2,y2),brush,size);
}
void dot(float x,float y,float r,UINT c,float a=1){
    use(c,a);canvas->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x,y),r,r),brush);
}
IDWriteTextFormat* font(int size,bool bold){
    int key=size*2+bold;
    if(!fonts.count(key)){
        IDWriteTextFormat* f=nullptr;
        textFactory->CreateTextFormat(uiFont.c_str(),nullptr,bold?DWRITE_FONT_WEIGHT_SEMI_BOLD:DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,static_cast<float>(size),L"pt-BR",&f);
        if(f){
            f->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_CHARACTER,0,0};
            IDWriteInlineObject* ellipsis=nullptr;
            textFactory->CreateEllipsisTrimmingSign(f,&ellipsis);f->SetTrimming(&trim,ellipsis);release(ellipsis);
        }
        fonts[key]=f;
    }
    return fonts[key];
}
void text(const std::wstring& s,float x,float y,float w,int size=13,UINT c=0xF3F5F0,bool bold=false,float a=1){
    auto f=font(size,bold);if(!f)return;use(c,a);
    canvas->DrawText(s.c_str(),static_cast<UINT32>(s.size()),f,D2D1::RectF(x,y,x+w,y+size*1.7f),brush,D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
// Position the visible glyph bounds, including accents and font bearings.
// This does not change the alignment of navigation or menu rows.
void alignedText(const std::wstring& value,float x,float y,float w,float h,int size,UINT tint,
                 bool bold=true,bool centered=true,float alpha=1){
    if(value.empty())return;
    IDWriteTextLayout* layout=nullptr;
    if(FAILED(textFactory->CreateTextLayout(value.c_str(),static_cast<UINT32>(value.size()),
        font(size,bold),std::max(1.0f,w),h,&layout)))return;
    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    DWRITE_OVERHANG_METRICS ink{};
    layout->GetOverhangMetrics(&ink);
    float left=-ink.left,right=w+ink.right,top=-ink.top,bottom=h+ink.bottom;
    float dx=centered?(w-left-right)/2:0,dy=(h-top-bottom)/2;
    use(tint,alpha);
    canvas->DrawTextLayout(D2D1::Point2F(x+dx,y+dy),layout,brush);
    layout->Release();
}
bool contains(D2D1_RECT_F r,float x,float y){return x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom;}
void hit(int id,float x,float y,float w,float h,const std::wstring& name=L""){
    hits.push_back({id,D2D1::RectF(x,y,x+w,y+h),name});
}
UINT accentForeground(){return hext::foreground(accent());}
bool controlEnabled(int id){
    if(id==1046){auto state=audioAddon.snapshot().state;return state!=hext::AddonState::Downloading&&state!=hext::AddonState::Installing;}
    if(id==1020||id==1021)return sound.reading[id-1020].ready;
    if(id==1022||id==1023){auto item=sound.chosen(id-1022);return item&&item->state==DEVICE_STATE_ACTIVE&&!item->isDefault;}
    if(id==1030||id==1031)return sound.reading[id-1030].ready;
    if(id==1032)return sound.reading[0].ready&&sound.reading[0].channels==2;
    if(id==1040||(id>=1041&&id<=1044))return audioApo.installed&&apoEndpointReady&&!audioEffectsProcess;
    if(id==1045)return soundEffectsDirty&&audioApo.installed&&apoEndpointReady&&!audioEffectsProcess;
    if(id==1050)return !bluetooth.refreshing();
    if(id==1052||id==1053||id==1054)return bluetoothSelected>=0&&bluetoothSelected<static_cast<int>(bluetoothSnapshot.devices.size());
    return !((id==317||id==902)&&(!systemTransparency||highContrast))&&!(id==311&&systemReducedMotion)&&
        !(id==466&&assistantProvider==0)&&
        !(id==467&&(assistantProvider==0||connectionState[assistantProvider]==hext::ConnectionState::Checking));
}
bool controlFocusable(int id){return id<520||id>=520+hext::MetricCount;}
void button(int id,const std::wstring& label,float x,float y,float w,float h,bool primary=false,bool selected=false){
    bool enabled=controlEnabled(id);
    auto& motion=controlMotion[id];
    motion.hover.target=enabled&&(hoverId==id||keyboardId==id)?1:0;
    motion.press.target=enabled&&pressedId==id?(page==5?.992:.975):1;
    motion.hover.frequency=page==5?55:35;motion.press.frequency=page==5?65:38;
    if(motionReduced()){motion.hover.snap(motion.hover.target);motion.press.snap(motion.press.target);}
    D2D1_MATRIX_3X2_F original;canvas->GetTransform(&original);
    canvas->SetTransform(D2D1::Matrix3x2F::Scale(static_cast<float>(motion.press.value),
        static_cast<float>(motion.press.value),D2D1::Point2F(x+w/2,y+h/2))*original);
    if((primary||selected)&&enabled)box(x,y,w,h,accent(),8,primary?1:.8f);
    else box(x,y,w,h,0xFFFFFF,8,.065f+.07f*unit(motion.hover.value));
    if(pressedId==id)box(x,y,w,h,0x000000,8,.12f);
    if(keyboardId==id){
        use(accent());canvas->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x-2,y-2,x+w+2,y+h+2),10,10),brush,1.5f);
    }
    float labelLeft=x+12,labelWidth=w-24;
    if(id==220){
        IDWriteTextLayout* measure=nullptr;DWRITE_TEXT_METRICS metrics{};
        textFactory->CreateTextLayout(label.c_str(),static_cast<UINT32>(label.size()),font(13,true),w-40,h,&measure);
        if(measure){measure->GetMetrics(&metrics);measure->Release();}
        float groupWidth=metrics.width+20,groupLeft=x+(w-groupWidth)/2;
        float p=unit(timerSymbol.value);UINT foreground=(primary||selected)?accentForeground():0xF2F2F6;
        symbol(0xE768,groupLeft,y+(h-13)/2,13,foreground,1-p);
        symbol(0xE769,groupLeft,y+(h-13)/2,13,foreground,p);
        labelLeft=groupLeft+20;labelWidth=metrics.width;
    }
    alignedText(label,labelLeft,y,labelWidth,h,13,
        !enabled?0x9999A4:(primary||selected)?accentForeground():0xF2F2F6,true,
        !(id>=400&&id<405));
    canvas->SetTransform(original);
    hit(id,x,y,w,h,label);
}
void stopAnimationClock(){
    KillTimer(windowHandle,1);animationClock=false;lastFrame=0;
    if(preciseClock){timeEndPeriod(1);preciseClock=false;}
}
void invalidate(int ms=180){
    if(testMode)return;
    paintUntil=std::max(paintUntil,GetTickCount64()+ms);
    // Repeated invalidations must not postpone an already scheduled frame.
    if(!animationClock){
        preciseClock=timeBeginPeriod(1)==TIMERR_NOERROR;
        lastFrame=GetTickCount64();SetTimer(windowHandle,1,16,nullptr);animationClock=true;
    }
}
void resetAssistantCaret(){
    assistantCaret=std::min(assistantCaret,assistantInput.size());
    assistantCaretVisible=assistantFocused;
    if(testMode)return;
    KillTimer(windowHandle,4);
    if(assistantFocused&&expanded&&page==0){
        UINT blink=GetCaretBlinkTime();
        if(blink!=INFINITE)SetTimer(windowHandle,4,blink?blink:530,nullptr);
    }
}
size_t previousAssistantCharacter(){
    size_t at=std::min(assistantCaret,assistantInput.size());
    if(at)at--;
    if(at&&assistantInput[at]>=0xDC00&&assistantInput[at]<=0xDFFF&&
       assistantInput[at-1]>=0xD800&&assistantInput[at-1]<=0xDBFF)at--;
    return at;
}
size_t nextAssistantCharacter(){
    size_t at=std::min(assistantCaret,assistantInput.size());
    if(at<assistantInput.size()){
        if(assistantInput[at]>=0xD800&&assistantInput[at]<=0xDBFF&&at+1<assistantInput.size()&&
           assistantInput[at+1]>=0xDC00&&assistantInput[at+1]<=0xDFFF)at++;
        at++;
    }
    return at;
}
void insertAssistantText(const std::wstring& value){
    if(selectAll){assistantInput.clear();assistantCaret=0;selectAll=false;}
    assistantCaret=std::min(assistantCaret,assistantInput.size());
    auto text=value.substr(0,4000-assistantInput.size());
    assistantInput.insert(assistantCaret,text);assistantCaret+=text.size();
    resetAssistantCaret();invalidate(120);
}
ULONGLONG remaining(){
    return timerRunning?(timerEnd>GetTickCount64()?timerEnd-GetTickCount64():0):timerRemaining;
}
std::wstring timerLabel(){
    auto seconds=(remaining()+999)/1000;
    wchar_t s[40];swprintf(s,40,L"%02llu:%02llu",seconds/60,seconds%60);return s;
}
std::wstring clockLabel(){
    SYSTEMTIME now;GetLocalTime(&now);wchar_t s[12];swprintf(s,12,L"%02d:%02d",now.wHour,now.wMinute);return s;
}
void saveConfig(){
    if(testMode)return;
    auto save=[](const wchar_t* key,int value){WritePrivateProfileString(L"Ilha",key,std::to_wstring(value).c_str(),configPath.c_str());};
    save(L"accentRGB",accentPalette.current());
    for(int i=0;i<4;i++)save((L"recent"+std::to_wstring(i)).c_str(),accentPalette.recent[i]);
    save(L"floating",floating);save(L"reducedMotion",reducedMotion);
    save(L"zoom",static_cast<int>(zoom*100));
    save(L"motion",motionStyle);save(L"material",translucent);
    save(L"assistantProvider",assistantProvider);
    save(L"metrics",static_cast<int>(metricMask));save(L"hardwareInterval",hardwareInterval);
    for(int i=0;i<7;i++)save((L"nav"+std::to_wstring(i)).c_str(),navigation.items[i]);
    for(int i=0;i<2;i++){
        save((L"shortcutMods"+std::to_wstring(i)).c_str(),shortcuts.bindings[i].modifiers);
        save((L"shortcutKey"+std::to_wstring(i)).c_str(),shortcuts.bindings[i].key);
    }
    for(int i=0;i<2;i++)WritePrivateProfileString(L"Som",i?L"input":L"output",sound.selected[i].c_str(),configPath.c_str());
    auto saveSound=[](const wchar_t* key,int value){WritePrivateProfileStringW(L"Som",key,std::to_wstring(value).c_str(),configPath.c_str());};
    saveSound(L"enabled",audioEffects.enabled);saveSound(L"gain",audioEffects.gain);saveSound(L"bass",audioEffects.bass);
    saveSound(L"mid",audioEffects.mid);saveSound(L"treble",audioEffects.treble);
    for(int i=1;i<4;i++)WritePrivateProfileString(L"Ilha",(L"assistantPath"+std::to_wstring(i)).c_str(),
        assistantPath[i].c_str(),configPath.c_str());
}
BOOL CALLBACK addMonitor(HMONITOR m,HDC,LPRECT,LPARAM){monitors.push_back(m);return TRUE;}
void updateMonitor(HMONITOR m){
    currentMonitor=m;MONITORINFO info{sizeof(info)};GetMonitorInfo(m,&info);monitorRect=info.rcMonitor;
    HMODULE shcore=LoadLibraryW(L"shcore.dll");
    if(shcore){
        using GetDpi=HRESULT(WINAPI*)(HMONITOR,int,UINT*,UINT*);
        GetDpi get=nullptr;
        FARPROC address=GetProcAddress(shcore,"GetDpiForMonitor");
        static_assert(sizeof(get)==sizeof(address));
        memcpy(&get,&address,sizeof(get));
        UINT x=96,y=96;if(get&&SUCCEEDED(get(m,0,&x,&y)))dpi=x/96.0f;FreeLibrary(shcore);
    }
    dpi=std::min(dpi*zoom,std::min((monitorRect.right-monitorRect.left-24)/736.0f,
                                  (monitorRect.bottom-monitorRect.top-80)/470.0f));
    dpi=std::max(.65f,dpi);
    materialDirty=true;
}
void setExpanded(bool value){
    if(!value&&shortcutOverlay){shortcutClosing=true;shortcutHold=false;}
    expanded=value;inputFocused=assistantFocused=false;keyboardId=-1;
    resetAssistantCaret();
    if(!testMode&&windowShown&&desktopEligible)visibility.expanded=value;
    width.target=value?PanelW:voicePreview||GetTickCount64()<stripUntil||timerRunning?358:184;
    height.target=value?PanelH:42;reveal.target=value?1:0;
    if(!value){closePopover();closeColorPicker();}
    pendingDesktopFocus=!value&&!testMode&&!shortcutOverlay&&GetForegroundWindow()==windowHandle;
    if(motionReduced())snapMotion();
    auto style=static_cast<MotionStyle>(motionStyle);
    width.tune(style,!value);height.tune(style,!value);
    reveal.frequency=value?22:29;reveal.damping=1;
    invalidate(420);
    if(value)requestHardware();
}
bool isDesktop(HWND handle) {
    if(!handle)return false;
    HWND root=GetAncestor(handle,GA_ROOT);
    if(root==GetShellWindow()||root==GetDesktopWindow())return true;
    wchar_t cls[128]{};
    GetClassName(root,cls,128);
    return wcscmp(cls,L"Progman")==0||wcscmp(cls,L"WorkerW")==0;
}
HWND findDesktopHost() {
    HWND host=GetShellWindow();
    EnumWindows([](HWND candidate,LPARAM data)->BOOL{
        if(FindWindowEx(candidate,nullptr,L"SHELLDLL_DefView",nullptr)){
            *reinterpret_cast<HWND*>(data)=candidate;return FALSE;
        }
        return TRUE;
    },reinterpret_cast<LPARAM>(&host));
    return host;
}
bool isAbove(HWND upper,HWND lower) {
    if(!upper||!lower||upper==lower)return false;
    for(HWND w=GetWindow(lower,GW_HWNDPREV);w;w=GetWindow(w,GW_HWNDPREV))
        if(w==upper)return true;
    return false;
}
bool desktopIsForeground() {
    // Test the compact island's footprint, rather than the foreground app's identity.
    // Enumerating in z-order also handles Explorer's Show Desktop restacking.
    struct Query {hext::Region island;bool exposed=true;};
    long w=static_cast<long>((timerRunning?358:184)*dpi);
    long x=monitorRect.left+(monitorRect.right-monitorRect.left-w)/2;
    long y=monitorRect.top+(floating?static_cast<long>(12*dpi):0);
    Query query{{x,y,x+w,y+static_cast<long>(42*dpi)},true};
    EnumWindows([](HWND candidate,LPARAM data)->BOOL{
        auto& q=*reinterpret_cast<Query*>(data);
        if(candidate==desktopHost||candidate==GetShellWindow())return FALSE;
        if(candidate==windowHandle||isDesktop(candidate)||!IsWindowVisible(candidate)||IsIconic(candidate))return TRUE;
        RECT bounds{};if(!GetWindowRect(candidate,&bounds))return TRUE;
        if(!hext::overlaps(q.island,{bounds.left,bounds.top,bounds.right,bounds.bottom}))return TRUE;
        DWORD cloaked=0;DwmGetWindowAttribute(candidate,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
        if(cloaked)return TRUE;
        RECT frame{};if(SUCCEEDED(DwmGetWindowAttribute(candidate,DWMWA_EXTENDED_FRAME_BOUNDS,&frame,sizeof(frame))))bounds=frame;
        if(hext::coversIsland(q.island,{bounds.left,bounds.top,bounds.right,bounds.bottom},true,false,false)){
            q.exposed=false;return FALSE;
        }
        return TRUE;
    },reinterpret_cast<LPARAM>(&query));
    return query.exposed;
}
void placeOnDesktop() {
    if(shortcutOverlay){
        shortcutLayerError=SetWindowPos(windowHandle,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)?0:GetLastError();
        return;
    }
    HWND host=desktopHost=findDesktopHost();
    HWND above=host?GetWindow(host,GW_HWNDPREV):nullptr;
    while(above==windowHandle)above=GetWindow(above,GW_HWNDPREV);
    // Insert immediately above desktop icons and below ordinary application windows.
    // No parenting, shell injection or modifications to Explorer are required.
    SetWindowPos(windowHandle,above?above:HWND_BOTTOM,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
void hideImmediately() {
    cancelShortcutCapture();
    if(shortcutOverlay)finishShortcutOverlay();
    finishNavigationDrag(false);
    pendingDesktopFocus=false;assistantFocused=false;
    resetAssistantCaret();
    visibility.expanded=false;visibility.compactVisible=false;
    appearance.snap(0);width.snap(184);height.snap(42);reveal.snap(0);
    expanded=false;inputFocused=false;mouseInside=false;hoverId=pressedId=-1;
    voicePreview=false;windowShown=false;
    closePopover();popoverMotion.snap(0);closeColorPicker();colorPickerMotion.snap(0);
    if(dragVolume||dragColor||dragSound){dragVolume=false;dragColor=dragSound=0;if(!testMode)ReleaseCapture();}
    ShowWindow(windowHandle,SW_HIDE);stopAnimationClock();
}
void dismissIsland() {
    cancelShortcutCapture();
    if(shortcutOverlay){shortcutClosing=true;shortcutHold=false;}
    visibility.dismiss();inputFocused=false;voicePreview=false;
    setExpanded(false);appearance.target=1;
    invalidate(300);
}
void beginDesktopExit(){
    cancelShortcutCapture();
    finishNavigationDrag(false);
    visibility.expanded=visibility.compactVisible=false;
    inputFocused=false;mouseInside=false;hoverId=pressedId=keyboardId=-1;
    hits.clear();
    if(dragVolume||dragColor||dragSound){dragVolume=false;dragColor=dragSound=0;if(!testMode)ReleaseCapture();}
    if(!windowShown)return;
    appearance.target=0;appearance.frequency=26;appearance.damping=1;
    if(motionReduced()){hideImmediately();return;}
    invalidate(320);
}
void restoreDesktopCompact(){
    visibility.expanded=false;visibility.candidateSince=0;
    width.snap(timerRunning?358:184);height.snap(42);reveal.snap(0);expanded=false;
    inputFocused=false;voicePreview=false;hoverId=pressedId=keyboardId=-1;
    closePopover();popoverMotion.snap(0);closeColorPicker();colorPickerMotion.snap(0);
    // Replace the stale layered bitmap BEFORE Explorer can expose the HWND.
    // This applies equally to hidden, minimized and restacked windows.
    appearance.snap(0);appearance.target=1;windowShown=true;
    render();
    if(!testMode){
        placeOnDesktop();
        ShowWindow(windowHandle,SW_SHOWNOACTIVATE);
    }
    appearance.target=1;appearance.frequency=18;appearance.damping=1;
    if(motionReduced())appearance.snap(1);
    lastFrame=GetTickCount64();invalidate(480);
}
void restackDesktop(){
    if(!testMode)placeOnDesktop();
    invalidate(120);
}
void updateVisibility() {
    if(testMode)return;
    if(shortcutOverlay&&!(GetWindowLongPtrW(windowHandle,GWL_EXSTYLE)&WS_EX_TOPMOST))placeOnDesktop();
    POINT mouse;GetCursorPos(&mouse);
    auto now=GetTickCount64();
    desktopEligible=desktopEntry.update(now,desktopIsForeground())||shortcutOverlay;
    bool checkOrder=now-lastDesktopOrderCheck>=400;
    if(!IsWindow(desktopHost)||checkOrder){
        desktopHost=findDesktopHost();lastDesktopOrderCheck=now;
    }
    RECT bounds{};GetWindowRect(windowHandle,&bounds);
    bool inside=windowShown&&IsWindowVisible(windowHandle)&&!IsIconic(windowHandle)&&
        (PtInRect(&bounds,mouse)||dragVolume||dragColor||dragSound||navigationHeld||menuOpen||shortcutHold||captureShortcut>=0);
    auto change=visibility.update(now,desktopEligible,!expanded&&inside,inside,hidden);
    auto repair=reconcilePresentation({desktopEligible,hidden,windowShown,
        IsWindowVisible(windowHandle)!=FALSE,IsIconic(windowHandle)!=FALSE,
        checkOrder&&isAbove(desktopHost,windowHandle)});
    if(repair==PresentationAction::Hide){
        beginDesktopExit();
        return;
    }
    if(!visibility.compactVisible)return;
    if(repair==PresentationAction::RestoreCompact) {
        restoreDesktopCompact();
        return;
    }
    if(repair==PresentationAction::Restack)restackDesktop();
    if(change==VisibilityChange::Show)setExpanded(true);
    else if(change==VisibilityChange::Hide){voicePreview=false;dismissIsland();}
}
void CALLBACK onForeground(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD) {
    if(windowHandle)PostMessage(windowHandle,ForegroundMessage,0,0);
}
void showStrip(const std::wstring& s,int ms=3500){
    stripText=s;stripUntil=GetTickCount64()+ms;if(!expanded)setExpanded(false);invalidate(250);
}
void disconnectAudio(){
    if(audio&&volumeCallback)audio->UnregisterControlChangeNotify(volumeCallback);
    release(audio);release(enumerator);audioReady=false;
}
void connectAudio(){
    disconnectAudio();
    if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
       __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&enumerator))))return;
    IMMDevice* device=nullptr;
    if(SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender,eMultimedia,&device))){
        if(SUCCEEDED(device->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(&audio)))){
            audioReady=SUCCEEDED(audio->GetMasterVolumeLevelScalar(&volume));audio->GetMute(&audioMuted);
            audio->RegisterControlChangeNotify(volumeCallback);
            volumeVisual.snap(volume);
        }
    }
    release(device);
}
void setVolume(float value){
    if(!audioReady)connectAudio();
    if(audio){
        value=std::clamp(value,0.0f,1.0f);
        if(SUCCEEDED(audio->SetMasterVolumeLevelScalar(value,nullptr))){
            volume=value;volumeVisual.target=volume;volumeVisual.frequency=32;
            if(dragVolume||motionReduced())volumeVisual.snap(volume);
        }else audioReady=false;
    }
    invalidate();
}
ULONGLONG asInteger(FILETIME t){return(static_cast<ULONGLONG>(t.dwHighDateTime)<<32)|t.dwLowDateTime;}
void sampleSystem(){
    lastSystemSample=GetTickCount64();
    MEMORYSTATUSEX mem{sizeof(mem)};
    if(GlobalMemoryStatusEx(&mem)){
        ramPercent=static_cast<float>(mem.dwMemoryLoad);
        ramTotalGB=static_cast<float>(mem.ullTotalPhys/1073741824.0);
        ramFreeGB=static_cast<float>(mem.ullAvailPhys/1073741824.0);
        ramSampleReady=true;
    }
    FILETIME idle,kernel,user;
    if(GetSystemTimes(&idle,&kernel,&user)){
        auto i=asInteger(idle),k=asInteger(kernel),u=asInteger(user),total=(k-previousKernel)+(u-previousUser);
        if(previousKernel&&total){
            cpuPercent=std::clamp(100.0f*(1-static_cast<float>(i-previousIdle)/total),0.0f,100.0f);
            cpuSampleReady=true;
        }
        previousIdle=i;previousKernel=k;previousUser=u;
    }
    // Telemetry refreshes one frame; it must not start a 60 Hz animation loop.
    cpuVisual.snap(cpuPercent);ramVisual.snap(ramPercent);
}
bool hardwareFresh(){
    return hardwareSnapshot.sampled&&GetTickCount64()-hardwareSnapshot.sampled<=static_cast<ULONGLONG>(std::max(5000,hardwareInterval*3));
}
void requestHardware(bool force){
    bool watching=expanded&&(page==3||(page==5&&settingsSection==4));
    unsigned sensors=(1u<<hext::CpuTemperature)|(1u<<hext::CpuPower)|(1u<<hext::GpuLoad)|
                     (1u<<hext::GpuTemperature)|(1u<<hext::GpuPower);
    auto now=GetTickCount64();
    unsigned requested=popoverOpen&&settingsMenu==360?hext::AllMetrics:metricMask;
    if(!force&&(!watching||!(requested&sensors)||!desktopEligible||hidden||now-lastHardwareRequest<static_cast<ULONGLONG>(hardwareInterval)))return;
    lastHardwareRequest=now;hardwareMonitor.request(requested);
}
std::wstring sensorApplication(bool afterburner){
    wchar_t programs[32768]{},local[32768]{};
    GetEnvironmentVariableW(afterburner?L"ProgramFiles(x86)":L"ProgramFiles",programs,32768);
    GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
    std::vector<std::filesystem::path> candidates=afterburner?
        std::vector<std::filesystem::path>{std::filesystem::path(programs)/L"MSI Afterburner/MSIAfterburner.exe"}:
        std::vector<std::filesystem::path>{std::filesystem::path(programs)/L"LibreHardwareMonitor/LibreHardwareMonitor.exe",
            std::filesystem::path(local)/L"Programs/LibreHardwareMonitor/LibreHardwareMonitor.exe"};
    std::error_code error;
    for(auto& path:candidates)if(std::filesystem::is_regular_file(path,error))return path.wstring();
    return {};
}
void mediaKey(WORD key){
    INPUT input[2]{};input[0].type=INPUT_KEYBOARD;input[0].ki.wVk=key;input[1]=input[0];input[1].ki.dwFlags=KEYEVENTF_KEYUP;
    SendInput(2,input,sizeof(INPUT));feedback=L"Comando de mídia enviado ao player ativo.";invalidate();
}
const wchar_t* assistantName(){return assistantProvider==1?L"Codex":assistantProvider==2?L"Claude":assistantProvider==3?L"Outro CLI":L"Nenhum";}
void updateNavigationTargets(bool snap=false){
    for(int i=0;i<7;i++){
        auto& motion=navigationRows[navigation.items[i]];
        motion.target=NavigationOrder::row(i);motion.frequency=36;motion.damping=1;
        if(snap||motionReduced())motion.snap(motion.target);
    }
    navPosition.target=NavigationOrder::row(navigation.position(page));
    if(snap||motionReduced())navPosition.snap(navPosition.target);
}
void beginNavigationDrag(){
    if(!navigationHeld||navigationCandidate<0)return;
    navigationDragging=true;pressedId=-1;invalidate(250);
}
void moveNavigationPointer(float y){
    navigationPointerY=std::clamp(y-navigationGrab,84.0f,414.0f);
    if(!navigationDragging)return;
    int slot=std::clamp(static_cast<int>(std::round((navigationPointerY-94)/44)),0,6);
    navigation.move(navigationCandidate,slot);updateNavigationTargets();
    navigationRows[navigationCandidate].snap(navigationPointerY);invalidate(200);
}
void finishNavigationDrag(bool commit){
    if(!navigationHeld)return;
    bool moved=navigationDragging;
    if(moved){navPosition.value=navigationRows[page].value;navPosition.velocity=navigationRows[page].velocity;}
    navigationHeld=navigationDragging=false;navigationCandidate=-1;
    KillTimer(windowHandle,5);
    if(!commit&&moved)navigation=dragOriginalNavigation;
    updateNavigationTargets();
    if(!testMode&&GetCapture()==windowHandle)ReleaseCapture();
    if(commit&&moved)saveConfig();
    invalidate(220);
}
void detectAssistant(int provider){
    if(provider<1||provider>2)return;
    std::error_code ec;
    if(std::filesystem::is_regular_file(assistantPath[provider],ec))return;
    assistantPath[provider].clear();
    wchar_t path[32768]{};
    if(SearchPathW(nullptr,provider==1?L"codex.exe":L"claude.exe",nullptr,32768,path,nullptr))assistantPath[provider]=path;
    if(!assistantPath[provider].empty())return;
    wchar_t local[32768]{},profile[32768]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);GetEnvironmentVariableW(L"USERPROFILE",profile,32768);
    std::vector<std::filesystem::path> candidates=provider==1?
        std::vector<std::filesystem::path>{std::filesystem::path(local)/L"Programs/OpenAI/Codex/bin/codex.exe"}:
        std::vector<std::filesystem::path>{std::filesystem::path(profile)/L".local/bin/claude.exe",
            std::filesystem::path(local)/L"Microsoft/WinGet/Links/claude.exe"};
    for(auto& candidate:candidates)if(std::filesystem::is_regular_file(candidate,ec)){assistantPath[provider]=candidate.wstring();break;}
}
bool assistantConfigured(){
    return assistantProvider>0&&connectionState[assistantProvider]==hext::ConnectionState::Connected;
}
void resetAssistantConnection(){
    if(probingProvider>0&&connectionState[probingProvider]==hext::ConnectionState::Checking)
        connectionState[probingProvider]=hext::ConnectionState::Disconnected;
    connectionProbe.cancel();probingProvider=0;
    if(assistantProvider>0){connectionState[assistantProvider]=hext::ConnectionState::Disconnected;connectionChecks[assistantProvider]=0;}
}
void startConnectionProbe(hext::AssistantTask stage){
    connectionStage=stage;
    auto work=std::filesystem::path(configPath).parent_path()/L"assistant-workspace";
    if(!connectionProbe.start(assistantPath[probingProvider],probingProvider,L"",work,stage)){
        connectionState[probingProvider]=hext::ConnectionState::Error;
        probingProvider=0;
    }
}
void connectAssistant(){
    resetAssistantConnection();
    if(!assistantProvider)return;
    detectAssistant(assistantProvider);
    std::error_code ec;
    if(!std::filesystem::is_regular_file(assistantPath[assistantProvider],ec)){
        connectionState[assistantProvider]=hext::ConnectionState::Missing;invalidate();return;
    }
    if(assistantProvider==3){connectionState[3]=hext::ConnectionState::Connected;saveConfig();invalidate();return;}
    probingProvider=assistantProvider;
    connectionState[assistantProvider]=hext::ConnectionState::Checking;
    startConnectionProbe(hext::AssistantTask::Version);saveConfig();invalidate();
}
void loginAssistant(){
    if(loginProcess)return;
    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;
    info.hwnd=windowHandle;info.lpFile=assistantPath[assistantProvider].c_str();
    info.lpParameters=assistantProvider==1?L"login":L"auth login";info.nShow=SW_SHOWNORMAL;
    if(ShellExecuteExW(&info)&&info.hProcess){
        loginProcess=info.hProcess;loginProvider=assistantProvider;
        connectionState[assistantProvider]=hext::ConnectionState::Checking;
    }else connectionState[assistantProvider]=hext::ConnectionState::Error;
    invalidate();
}
void pollAssistantConnection(){
    if(loginProcess&&WaitForSingleObject(loginProcess,0)==WAIT_OBJECT_0){
        CloseHandle(loginProcess);loginProcess=nullptr;
        if(assistantProvider==loginProvider)connectAssistant();
        else connectionState[loginProvider]=hext::ConnectionState::Disconnected;
        loginProvider=0;invalidate();
    }
    if(!connectionProbe.poll())return;
    int provider=probingProvider;
    if(!provider||provider!=assistantProvider){connectionProbe.result.clear();connectionProbe.diagnostic.clear();return;}
    auto output=connectionProbe.result+"\n"+connectionProbe.diagnostic;
    DWORD code=connectionProbe.exitCode;
    if(connectionStage==hext::AssistantTask::Version){
        if(!code&&hext::providerVersion(provider,output)){
            connectionChecks[provider]|=1;startConnectionProbe(hext::AssistantTask::Capabilities);
        }
        else connectionState[provider]=code?hext::ConnectionState::Error:hext::ConnectionState::Incompatible;
    }else if(connectionStage==hext::AssistantTask::Capabilities){
        if(!code&&hext::providerCapabilities(provider,output)){
            connectionChecks[provider]|=2;startConnectionProbe(hext::AssistantTask::Authentication);
        }
        else connectionState[provider]=hext::ConnectionState::Incompatible;
    }else{
        connectionState[provider]=hext::providerAuthenticated(provider,code,output)?
            hext::ConnectionState::Connected:code==WAIT_TIMEOUT?hext::ConnectionState::Error:hext::ConnectionState::NeedsLogin;
        if(connectionState[provider]==hext::ConnectionState::Connected)connectionChecks[provider]|=4;
    }
    if(connectionState[provider]!=hext::ConnectionState::Checking){
        probingProvider=0;connectionProbe.result.clear();connectionProbe.diagnostic.clear();
        if(assistantConfigured())assistantStatus.clear();
    }
    invalidate();
}
const wchar_t* assistantConnectionMessage(){
    using S=hext::ConnectionState;
    if(!assistantProvider)return L"Escolha um assistente.";
    switch(connectionState[assistantProvider]){
        case S::Checking:return loginProcess&&loginProvider==assistantProvider?
            L"Conclua a entrada na janela aberta.":L"Verificando instalação e autenticação…";
        case S::Connected:return L"Conectado. Pronto para conversar.";
        case S::Missing:return L"Assistente não encontrado neste computador.";
        case S::NeedsLogin:return L"Entre na sua conta para concluir a conexão.";
        case S::Incompatible:return L"Esta versão precisa ser atualizada.";
        case S::Error:return L"Não foi possível verificar. Tente novamente.";
        default:return L"Verifique e conecte o assistente.";
    }
}
void sendAssistant(){
    if(assistantBridge.busy){assistantBridge.cancel();assistantStatus=L"Interrompido.";invalidate();return;}
    if(assistantInput.empty())return;
    if(!assistantConfigured()){assistantStatus=L"Conecte o assistente nas configurações.";invalidate();return;}
    conversation.push_back({true,assistantInput});assistantInput.clear();assistantCaret=0;assistantTextScroll=0;resetAssistantCaret();assistantScroll=0;
    if(conversation.size()>12)conversation.erase(conversation.begin(),conversation.begin()+2);
    std::wstring prompt=L"Responda em português como assistente pessoal. Responda somente à conversa abaixo, sem executar ações no computador.\n";
    for(auto& message:conversation)prompt+=(message.user?L"\nUsuário: ":L"\nAssistant: ")+message.text+L"\n";
    if(prompt.size()>24000)prompt=L"Responda em português à conversa (histórico parcial):\n"+prompt.substr(prompt.size()-24000);
    auto work=std::filesystem::path(configPath).parent_path()/L"assistant-workspace";
    if(!assistantBridge.start(assistantPath[assistantProvider],assistantProvider,prompt,work))
        assistantStatus=L"Não foi possível iniciar o assistente. Verifique a conexão.";
    else assistantStatus=L"Pensando…";
    invalidate();
}
void chooseAssistantExecutable(){
    if(assistantProvider==0)return;
    wchar_t path[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};
    dialog.hwndOwner=windowHandle;dialog.lpstrFile=path;dialog.nMaxFile=32768;
    dialog.lpstrFilter=L"Executável do assistente (*.exe)\0*.exe\0";
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    menuOpen=true;
    if(GetOpenFileNameW(&dialog)){resetAssistantConnection();assistantPath[assistantProvider]=path;saveConfig();}
    menuOpen=false;invalidate();
}
#include "shortcut-actions.h"
#include "sound-actions.h"
#include "interface.h"

void changePage(int value){
    if(value==page)return;
    cancelShortcutCapture();
    if(value==5)settingsReturnPage=page;
    previousPage=page;
    page=value;inputFocused=assistantFocused=false;keyboardId=-1;selectAll=false;
    resetAssistantCaret();
    pageFade.snap(motionReduced()?1:0);pageFade.target=1;pageFade.frequency=30;
    navPosition.target=NavigationOrder::row(navigation.position(page));navPosition.frequency=28;navPosition.damping=.92;
    if(page==0){
        timerX.target=590;timerY.target=294;timerSize.target=34;
        timerButtonX.target=224;timerButtonY.target=365;timerButtonW.target=138;timerButtonH.target=30;
    }
    if(page==2){
        timerX.target=372;timerY.target=149;timerSize.target=64;
        timerButtonX.target=546;timerButtonY.target=379;timerButtonW.target=148;timerButtonH.target=34;
    }
    timerX.frequency=timerY.frequency=timerSize.frequency=22;
    timerButtonX.frequency=timerButtonY.frequency=timerButtonW.frequency=timerButtonH.frequency=22;
    if(motionReduced()){
        navPosition.snap(navPosition.target);timerX.snap(timerX.target);timerY.snap(timerY.target);timerSize.snap(timerSize.target);
        timerButtonX.snap(timerButtonX.target);timerButtonY.snap(timerButtonY.target);
        timerButtonW.snap(timerButtonW.target);timerButtonH.snap(timerButtonH.target);
    }
    if(!expanded)setExpanded(true);
    if(page==3){sampleSystem();requestHardware(true);}
    if(page==6){refreshSound();soundChooser=-1;}
    invalidate(250);
}
void startTimer(){
    if(timerRunning){timerRemaining=remaining();timerRunning=false;}
    else{if(!timerRemaining)timerRemaining=timerDuration;timerEnd=GetTickCount64()+timerRemaining;timerRunning=true;}
    timerSymbol.target=timerRunning?1:0;timerSymbol.frequency=32;
    if(motionReduced())timerSymbol.snap(timerSymbol.target);
    if(!expanded)setExpanded(false);
    invalidate();
}
void executeCommand(){
    std::wstring normalized=command;
    bool keepInput=false;
    std::transform(normalized.begin(),normalized.end(),normalized.begin(),towlower);
    if(normalized.rfind(L"timer ",0)==0){
        try{
            size_t used=0;int minutes=std::stoi(normalized.substr(6),&used);
            if(used!=normalized.size()-6||minutes<1||minutes>1440)throw 0;
            timerDuration=timerRemaining=minutes*60ULL*1000;timerRunning=false;startTimer();
            feedback=L"Temporizador iniciado: "+std::to_wstring(minutes)+L" min.";
        }catch(...){feedback=L"Use timer 5, com um valor de 1 a 1440 minutos.";keepInput=true;}
    }else if(normalized.rfind(L"volume ",0)==0){
        try{
            size_t used=0;int v=std::stoi(normalized.substr(7),&used);
            if(used!=normalized.size()-7||v<0||v>100)throw 0;
            setVolume(v/100.0f);feedback=audioReady?L"Volume ajustado para "+std::to_wstring(v)+L"%.":L"Nenhuma saída de áudio disponível.";
        }catch(...){feedback=L"Use volume 30, com um valor de 0 a 100.";keepInput=true;}
    }else if(normalized==L"pausar timer"){if(timerRunning)startTimer();feedback=L"Temporizador pausado.";}
    else if(normalized==L"arquivos")changePage(4);
    else if(normalized==L"meu pc")changePage(3);
    else if(normalized==L"configurações"||normalized==L"configuracoes")changePage(5);
    else if(normalized==L"relógio"||normalized==L"relogio"){setExpanded(false);showStrip(clockLabel());}
    else{feedback=L"Comando não reconhecido. Experimente: timer 5.";keepInput=true;}
    if(!keepInput)command.clear();
    selectAll=false;invalidate(180);
}
void action(int id){
    if(!controlEnabled(id))return;
    keyboardId=-1;
    if(id>=100&&id<107)changePage(id-100);
    else if(id>=1000)soundAction(id);
    else if(id>=600&&id<=605){
        cancelShortcutCapture();
        if(settingsSection!=id-600){
            settingsSection=id-600;
            settingsFade.snap(motionReduced()?1:0);settingsFade.target=1;settingsFade.frequency=55;
        }
        invalidate(160);
        if(settingsSection==4)requestHardware(true);
    }
    else if(id==610)changePage(settingsReturnPage);
    else if(id==620||id==621)beginShortcutCapture(id-620);
    else if(id==622||id==623){beginShortcutCapture(id-622);commitShortcut(hext::DefaultShortcuts[id-622]);}
    else if(id==624)cancelShortcutCapture();
    else if(id==470||id==471){changePage(5);settingsSection=4;settingsFade.snap(1);requestHardware(true);invalidate();}
    else if(id>=380&&id<380+hext::MetricCount){
        metricMask^=1u<<(id-380);saveConfig();requestHardware(true);invalidate();
    }
    else if(id==360||id==362){openSettingsMenu(id);if(id==360)requestHardware(true);invalidate();}
    else if(id>=741&&id<=743){
        hardwareInterval=id==741?1000:id==742?2000:5000;
        closePopover();saveConfig();requestHardware(true);invalidate();
    }
    else if(id==751){requestHardware(true);invalidate();}
    else if(id==752||id==753){
        auto path=sensorApplication(id==752);
        if(!path.empty())ShellExecuteW(windowHandle,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        closePopover();invalidate();
    }
    else if(id==754){closePopover();invalidate();}
    else if(id==450){assistantFocused=true;assistantCaret=assistantInput.size();resetAssistantCaret();inputFocused=false;keyboardId=450;selectAll=false;invalidate();}
    else if(id==451)sendAssistant();
    else if(id==452){if(!assistantBridge.busy){conversation.clear();assistantStatus.clear();assistantScroll=0;}invalidate();}
    else if(id==460||id==461){changePage(5);settingsSection=3;settingsFade.snap(1);invalidate();}
    else if(id==465){openSettingsMenu(465);invalidate();}
    else if(id==466)chooseAssistantExecutable();
    else if(id==467){
        if(connectionState[assistantProvider]==hext::ConnectionState::NeedsLogin&&assistantProvider<3)loginAssistant();
        else connectAssistant();
    }
    else if(id>=801&&id<=804){
        resetAssistantConnection();assistantProvider=id-801;connectionState[assistantProvider]=hext::ConnectionState::Disconnected;
        detectAssistant(assistantProvider);closePopover();saveConfig();invalidate();
    }
    else if(id==10||id==11)dismissIsland();
    else if(id==12){if(popoverOpen)closePopover();else openPopover();invalidate(260);}
    else if(id==210)mediaKey(VK_MEDIA_PREV_TRACK);
    else if(id==211)mediaKey(VK_MEDIA_PLAY_PAUSE);
    else if(id==212)mediaKey(VK_MEDIA_NEXT_TRACK);
    else if(id==220)startTimer();
    else if(id==225)changePage(2);
    else if(id>=221&&id<=223){
        int minutes=id==221?1:id==222?5:25;
        timerRunning=false;timerSymbol.target=0;timerDuration=timerRemaining=minutes*60ULL*1000;invalidate();
    }
    else if(id==224){timerRunning=false;timerSymbol.target=0;timerRemaining=timerDuration;invalidate();}
    else if(id==230){voicePreview=true;previewUntil=GetTickCount64()+5000;setExpanded(false);invalidate(motionReduced()?180:5200);}
    else if(id==240){
        inputFocused=true;selectAll=false;
        SetForegroundWindow(windowHandle);SetFocus(windowHandle);
        if(!testMode)placeOnDesktop();
        invalidate();
    }
    else if(id==241)executeCommand();
    else if(id>=300&&id<304){accentPalette.select(accentPalette.recent[id-300]);saveConfig();invalidate();}
    else if(id==330){openColorPicker();invalidate();}
    else if(id==331||id==332){keyboardId=id;invalidate();}
    else if(id==334||id==336){closeColorPicker();invalidate();}
    else if(id==335){accentPalette.select(hext::rgb(draftColor));closeColorPicker();saveConfig();invalidate();}
    else if(id==310||id==314||id==317){openSettingsMenu(id);invalidate(280);}
    else if(id==311){
        if(!systemReducedMotion)reducedMotion=!reducedMotion;
        saveConfig();
        if(motionReduced())snapMotion();
        invalidate();
    }
    else if(id==313){
        monitors.clear();EnumDisplayMonitors(nullptr,nullptr,addMonitor,0);
        auto i=std::find(monitors.begin(),monitors.end(),currentMonitor);
        if(!monitors.empty()){size_t next=i==monitors.end()?0:(i-monitors.begin()+1)%monitors.size();updateMonitor(monitors[next]);}invalidate();
    }else if(id==315){connectAudio();volumeVisual.target=volume;invalidate();}
    else if(id==701||id==702||id==711||id==712||(id>=721&&id<=724)){
        closePopover();
        if(id==701||id==702){translucent=id==701;materialDirty=true;}
        if(id==711||id==712)floating=id==712;
        if(id>=721&&id<=724){zoom=.85f+(id-721)*.15f;updateMonitor(currentMonitor);}
        saveConfig();invalidate(280);
    }
    else if(id>=320&&id<=322){
        motionStyle=id-320;saveConfig();
        width.tune(static_cast<MotionStyle>(motionStyle),!expanded);
        height.tune(static_cast<MotionStyle>(motionStyle),!expanded);
        invalidate(180);
    }
    else if(id>=901&&id<=905){
        closePopover();invalidate(220);
        if(id==901)changePage(5);
        if(id==902){translucent=!translucent;materialDirty=true;saveConfig();}
        if(id==903){
            motionStyle=(motionStyle+1)%3;saveConfig();
            width.tune(static_cast<MotionStyle>(motionStyle),!expanded);
            height.tune(static_cast<MotionStyle>(motionStyle),!expanded);
        }
        if(id==904)dismissIsland();
        if(id==905)DestroyWindow(windowHandle);
    }
    else if(id>=400&&id<405&&id-400<static_cast<int>(files.size())){
        auto result=ShellExecute(windowHandle,L"open",files[id-400].c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        if(reinterpret_cast<INT_PTR>(result)<=32)showStrip(L"Não foi possível abrir esse arquivo.");
    }else if(id==410){files.clear();invalidate();}
}
int hitAt(float x,float y){
    for(auto i=hits.rbegin();i!=hits.rend();++i) {
        if(contains(i->rect,x,y))return i->id;
    }
    return -1;
}
#include "accessibility.h"

void contextMenu(){
    menuOpen=true;
    HMENU menu=CreatePopupMenu();AppendMenu(menu,MF_STRING,1,L"Retomar ativação no topo");
    AppendMenu(menu,MF_STRING,2,L"Personalizar na área de trabalho");
    AppendMenu(menu,MF_STRING,3,L"Pausar ativação automática");
    AppendMenu(menu,MF_SEPARATOR,0,nullptr);AppendMenu(menu,MF_STRING,4,L"Sair do Hext");
    POINT p;GetCursorPos(&p);SetForegroundWindow(windowHandle);
    UINT selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,windowHandle,nullptr);DestroyMenu(menu);
    menuOpen=false;
    if(selected==1){hidden=false;visibility.awaitLeave=false;}
    if(selected==2){hidden=false;page=5;visibility.awaitLeave=false;}
    if(selected==3){hidden=true;hideImmediately();}
    if(selected==4)DestroyWindow(windowHandle);
}
void tray(bool add){
    NOTIFYICONDATA data{sizeof(data)};data.hWnd=windowHandle;data.uID=1;data.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
    data.uCallbackMessage=TrayMessage;data.hIcon=appIcon;
    wcscpy_s(data.szTip,L"Hext · passe o mouse no topo da área de trabalho");Shell_NotifyIcon(add?NIM_ADD:NIM_DELETE,&data);
}
LRESULT CALLBACK procedure(HWND wnd,UINT message,WPARAM wp,LPARAM lp){
    if(message==taskbarCreated&&taskbarCreated){
        desktopHost=nullptr;lastDesktopOrderCheck=0;tray(true);
        PostMessage(wnd,ForegroundMessage,0,0);return 0;
    }
    switch(message){
    case WM_GETOBJECT:
        if(static_cast<DWORD>(lp)==static_cast<DWORD>(OBJID_CLIENT)&&accessibleUI)
            return LresultFromObject(IID_IAccessible,wp,accessibleUI);
        break;
    case WM_APP+5:
        if(desktopEligible&&windowShown)action(static_cast<int>(wp));
        return 0;
    case WM_APP+6:
        if(desktopEligible&&windowShown)setVolume(std::clamp(static_cast<int>(wp),0,100)/100.0f);
        return 0;
    case WM_APP+7:
        if(colorPickerOpen){draftColor.v=std::clamp(static_cast<int>(wp)/100.0,0.0,1.0);invalidate();}
        return 0;
    case WM_APP+8:
        commitShortcut({static_cast<UINT>(lp),static_cast<UINT>(wp)});return 0;
    case WM_APP+9:
        // Read-only state for the runtime verifier; no paths, credentials or user data.
        if(wp==1)return GetWindowLongPtrW(wnd,GWL_EXSTYLE);
        if(wp==2)return shortcutLayerError;
        if(wp==3)return GetForegroundWindow()==windowHandle||
            (IsWindow(shortcutReturnWindow)&&isAbove(windowHandle,shortcutReturnWindow));
        return (expanded?1:0)|(shortcutOverlay?2:0)|(shortcutClosing?4:0)|(hidden?8:0)|
            (windowShown?16:0)|(shortcutHold?32:0)|(desktopEligible?64:0)|
            (shortcuts.registeredIDs[0]<<8)|(shortcuts.registeredIDs[1]<<12);
    case WM_ERASEBKGND:return 1;
    case WM_SETFOCUS:if(assistantFocused)resetAssistantCaret();return 0;
    case WM_KILLFOCUS:
        cancelShortcutCapture();
        if(assistantFocused){assistantFocused=false;resetAssistantCaret();invalidate(100);}
        return 0;
    case WM_SETCURSOR:{
        POINT point{};GetCursorPos(&point);ScreenToClient(wnd,&point);
        if(hitAt(point.x/dpi,point.y/dpi)==450){SetCursor(LoadCursor(nullptr,IDC_IBEAM));return TRUE;}
        break;
    }
    case WM_PAINT:{PAINTSTRUCT p;BeginPaint(wnd,&p);EndPaint(wnd,&p);render();return 0;}
    case WM_MOUSEACTIVATE:return MA_NOACTIVATE;
    case WM_NCHITTEST:
        if(!testMode&&(!desktopEligible||hidden||appearance.value<.08))return HTTRANSPARENT;
        break;
    case WM_MOUSEMOVE:{
        if(shortcutOverlay)shortcutHold=false;
        if(!mouseInside){mouseInside=true;hoverSince=GetTickCount64();TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,wnd,0};TrackMouseEvent(&track);}
        float x=GET_X_LPARAM(lp)/dpi,y=GET_Y_LPARAM(lp)/dpi;
        if(navigationHeld){
            if(!navigationDragging&&(std::abs(x-navigationDown.x)>5||std::abs(y-navigationDown.y)>5))beginNavigationDrag();
            moveNavigationPointer(y);
            if(navigationDragging)return 0;
        }
        if(dragColor){updateColorPointer(x,y,dragColor);return 0;}
        if(dragSound){soundPointer(dragSound,x);return 0;}
        if(dragVolume)setVolume((x-volumeTrack.left)/std::max(1.0f,volumeTrack.right-volumeTrack.left));
        int id=hitAt(x,y);if(id!=hoverId){hoverId=id;hoverSince=GetTickCount64();invalidate();}return 0;
    }
    case WM_MOUSELEAVE:mouseInside=false;hoverId=-1;invalidate();return 0;
    case WM_LBUTTONDOWN:
        if(expanded&&!testMode){
            SetForegroundWindow(wnd);SetFocus(wnd);placeOnDesktop();
        }
        pressedId=hitAt(GET_X_LPARAM(lp)/dpi,GET_Y_LPARAM(lp)/dpi);invalidate(130);
        if(captureShortcut>=0&&pressedId!=620&&pressedId!=621)cancelShortcutCapture();
        if(page!=5&&!popoverOpen&&!colorPickerOpen&&pressedId>=100&&pressedId<107){
            navigationCandidate=pressedId-100;navigationHeld=true;navigationDragging=false;
            dragOriginalNavigation=navigation;
            navigationDown={static_cast<LONG>(GET_X_LPARAM(lp)/dpi),static_cast<LONG>(GET_Y_LPARAM(lp)/dpi)};
            navigationGrab=navigationDown.y-static_cast<float>(navigationRows[navigationCandidate].value);
            navigationPointerY=static_cast<float>(navigationRows[navigationCandidate].value);
            if(!testMode){SetCapture(wnd);SetTimer(wnd,5,180,nullptr);}
        }
        if(assistantFocused&&pressedId!=450&&pressedId!=451){assistantFocused=false;resetAssistantCaret();selectAll=false;}
        if(soundSlider(pressedId)&&controlEnabled(pressedId)){
            dragSound=pressedId;keyboardId=pressedId;if(!testMode)SetCapture(wnd);
            soundPointer(dragSound,GET_X_LPARAM(lp)/dpi);return 0;
        }
        if(colorPickerOpen&&(pressedId==331||pressedId==332)){
            dragColor=pressedId;keyboardId=pressedId;SetCapture(wnd);
            updateColorPointer(GET_X_LPARAM(lp)/dpi,GET_Y_LPARAM(lp)/dpi,dragColor);return 0;
        }
        if(expanded&&hitAt(GET_X_LPARAM(lp)/dpi,GET_Y_LPARAM(lp)/dpi)==200){
            dragVolume=true;SetCapture(wnd);
            setVolume((GET_X_LPARAM(lp)/dpi-volumeTrack.left)/std::max(1.0f,volumeTrack.right-volumeTrack.left));
        }return 0;
    case WM_LBUTTONUP:{
        pressedId=-1;invalidate(130);
        if(navigationHeld){
            bool dragged=navigationDragging;finishNavigationDrag(true);
            if(dragged)return 0;
        }
        if(dragColor){dragColor=0;ReleaseCapture();return 0;}
        if(dragSound){dragSound=0;if(!testMode)ReleaseCapture();return 0;}
        if(dragVolume){dragVolume=false;ReleaseCapture();return 0;}
        int target=hitAt(GET_X_LPARAM(lp)/dpi,GET_Y_LPARAM(lp)/dpi);
        if(colorPickerOpen&&target!=334&&target!=335&&target!=336&&target!=331&&target!=332){
            closeColorPicker();invalidate();return 0;
        }
        if(popoverOpen&&!((target>=901&&target<=905)||(target>=701&&target<=724)||
                         (target>=741&&target<=754)||(target>=801&&target<=804))){
            closePopover();invalidate(200);return 0;
        }
        if(!expanded){voicePreview=false;setExpanded(true);return 0;}
        action(target);
        if(target==450)positionAssistantCaret(GET_X_LPARAM(lp)/dpi);
        return 0;
    }
    case WM_CAPTURECHANGED:
        if(navigationHeld)finishNavigationDrag(false);
        dragVolume=false;dragColor=0;dragSound=0;pressedId=-1;return 0;
    case WM_RBUTTONUP:
        if(desktopEligible){if(!expanded)setExpanded(true);openPopover();invalidate(300);}
        return 0;
    case WM_MOUSEWHEEL:
        if(page==6){
            int delta=GET_WHEEL_DELTA_WPARAM(wp)>0?-4:4;
            if(soundChooser>=0){int count=static_cast<int>(std::count_if(sound.devices.begin(),sound.devices.end(),[](const auto& d){return d.flow==soundChooser;}));
                soundDeviceOffset=std::clamp(soundDeviceOffset+delta,0,std::max(0,((count-1)/4)*4));}
            else if(soundTab==3)bluetoothOffset=std::clamp(bluetoothOffset+delta,0,std::max(0,((static_cast<int>(bluetoothSnapshot.devices.size())-1)/4)*4));
            invalidate();return 0;
        }
        if(page==0){assistantScroll=std::clamp(assistantScroll+GET_WHEEL_DELTA_WPARAM(wp)/120.0f*60,0.0f,assistantScrollMax);invalidate();}
        return 0;
    case WM_KEYDOWN:
        if(captureShortcut>=0){if(wp==VK_ESCAPE){cancelShortcutCapture();invalidate();}return 0;}
        if(page==6&&soundChooser>=0&&wp==VK_ESCAPE){soundChooser=-1;invalidate();return 0;}
        if(soundSlider(keyboardId)&&(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_UP||wp==VK_DOWN||wp==VK_HOME||wp==VK_END)){
            float step=keyboardId==1041?1/24.f:keyboardId>=1042?1/48.f:.02f;
            soundSliderValue(keyboardId,wp==VK_HOME?0:wp==VK_END?1:soundSliderPosition(keyboardId)+(wp==VK_LEFT||wp==VK_DOWN?-step:step));return 0;
        }
        if(navigationHeld&&wp==VK_ESCAPE){finishNavigationDrag(false);return 0;}
        if(page!=5&&keyboardId>=100&&keyboardId<107&&GetKeyState(VK_CONTROL)<0&&(wp==VK_UP||wp==VK_DOWN)){
            int selected=keyboardId-100;navigation.move(selected,navigation.position(selected)+(wp==VK_UP?-1:1));
            updateNavigationTargets();saveConfig();invalidate(220);return 0;
        }
        if(assistantFocused){
            if(GetKeyState(VK_CONTROL)<0&&wp=='A'){selectAll=true;assistantCaret=assistantInput.size();resetAssistantCaret();invalidate();return 0;}
            if(GetKeyState(VK_CONTROL)<0&&wp=='V'){SendMessage(wnd,WM_PASTE,0,0);return 0;}
            if(wp==VK_RETURN){sendAssistant();return 0;}
            if(wp==VK_SPACE)return 0;
            if(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_HOME||wp==VK_END){
                if(wp==VK_HOME)assistantCaret=0;
                else if(wp==VK_END)assistantCaret=assistantInput.size();
                else if(selectAll)assistantCaret=wp==VK_LEFT?0:assistantInput.size();
                else assistantCaret=wp==VK_LEFT?previousAssistantCharacter():nextAssistantCharacter();
                selectAll=false;resetAssistantCaret();invalidate(120);return 0;
            }
            if(wp==VK_DELETE){
                if(selectAll){assistantInput.clear();assistantCaret=0;selectAll=false;}
                else{auto next=nextAssistantCharacter();assistantInput.erase(assistantCaret,next-assistantCaret);}
                resetAssistantCaret();invalidate(120);return 0;
            }
        }
        if(colorPickerOpen&&wp==VK_ESCAPE){closeColorPicker();invalidate();return 0;}
        if(colorPickerOpen&&(keyboardId==331||keyboardId==332)&&
           (wp==VK_LEFT||wp==VK_RIGHT||wp==VK_UP||wp==VK_DOWN)){
            if(keyboardId==332)draftColor.v=std::clamp(draftColor.v+(wp==VK_LEFT||wp==VK_DOWN?-.02:.02),0.0,1.0);
            else if(wp==VK_LEFT||wp==VK_RIGHT)draftColor.h=std::fmod(draftColor.h+(wp==VK_LEFT?358:2),360.0);
            else draftColor.s=std::clamp(draftColor.s+(wp==VK_DOWN?-.02:.02),0.0,1.0);
            invalidate();return 0;
        }
        if(keyboardId==200&&(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_UP||wp==VK_DOWN)){
            setVolume(volume+(wp==VK_LEFT||wp==VK_DOWN?-.02f:.02f));return 0;
        }
        if(popoverOpen&&(wp==VK_UP||wp==VK_DOWN)){
            if(!hits.empty()){
                auto found=std::find_if(hits.begin(),hits.end(),[](const Hit& h){return h.id==keyboardId;});
                int index=found==hits.end()?(wp==VK_DOWN?0:static_cast<int>(hits.size())-1):
                    (static_cast<int>(found-hits.begin())+(wp==VK_DOWN?1:static_cast<int>(hits.size())-1))%static_cast<int>(hits.size());
                keyboardId=hits[index].id;invalidate();
            }
            return 0;
        }
        if(wp==VK_ESCAPE&&popoverOpen){closePopover();invalidate(200);return 0;}
        if(inputFocused&&GetKeyState(VK_CONTROL)<0&&wp=='A'){selectAll=true;invalidate();return 0;}
        if(inputFocused&&GetKeyState(VK_CONTROL)<0&&wp=='V'){SendMessage(wnd,WM_PASTE,0,0);return 0;}
        if(wp==VK_ESCAPE){if(inputFocused||assistantFocused){inputFocused=assistantFocused=false;resetAssistantCaret();}else dismissIsland();invalidate();return 0;}
        if(wp==VK_TAB&&expanded&&!hits.empty()){
            auto i=std::find_if(hits.begin(),hits.end(),[](const Hit& h){return h.id==keyboardId;});
            int direction=GetKeyState(VK_SHIFT)<0?-1:1;
            int at=i==hits.end()?(direction==1?-1:0):static_cast<int>(i-hits.begin());
            for(size_t n=0;n<hits.size();n++){
                at=(at+direction+static_cast<int>(hits.size()))%static_cast<int>(hits.size());
                if(controlEnabled(hits[at].id)&&controlFocusable(hits[at].id)){keyboardId=hits[at].id;break;}
            }
            inputFocused=false;assistantFocused=keyboardId==450;
            if(assistantFocused)assistantCaret=assistantInput.size();
            resetAssistantCaret();invalidate();return 0;
        }
        if((wp==VK_RETURN||wp==VK_SPACE)&&keyboardId!=-1){int id=keyboardId;action(id);return 0;}return 0;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if(captureShortcut>=0)return 0;
        break;
    case WM_SYSCOMMAND:
        if((wp&0xFFF0)==SC_KEYMENU)return 0;
        break;
    case WM_CHAR:
        if(assistantFocused){
            if(wp==VK_BACK){
                if(selectAll){assistantInput.clear();assistantCaret=0;}
                else{auto previous=previousAssistantCharacter();assistantInput.erase(previous,assistantCaret-previous);assistantCaret=previous;}
                selectAll=false;
                resetAssistantCaret();
            }else if(wp>=32&&wp!=127){
                insertAssistantText(std::wstring(1,static_cast<wchar_t>(wp)));
            }
            invalidate();return 0;
        }
        if(inputFocused){
            if(wp==VK_RETURN)executeCommand();
            else if(wp==VK_BACK){
                if(selectAll)command.clear();else if(!command.empty())command.pop_back();
                selectAll=false;
            }else if(wp>=32&&wp!=127){
                if(selectAll){command.clear();selectAll=false;}
                if(command.size()<120)command+=static_cast<wchar_t>(wp);
            }
            invalidate();
        }return 0;
    case WM_PASTE:
        if(assistantFocused&&OpenClipboard(wnd)){
            auto handle=GetClipboardData(CF_UNICODETEXT);
            if(handle){
                auto value=static_cast<const wchar_t*>(GlobalLock(handle));
                if(value){
                    std::wstring clean;
                    for(size_t i=0;value[i]&&clean.size()<4000;i++)if(value[i]>=32&&value[i]!=127)clean+=value[i];
                    GlobalUnlock(handle);insertAssistantText(clean);
                }
            }
            CloseClipboard();invalidate();return 0;
        }
        if(inputFocused&&OpenClipboard(wnd)){
            auto handle=GetClipboardData(CF_UNICODETEXT);
            if(handle){
                auto value=static_cast<const wchar_t*>(GlobalLock(handle));
                if(value){
                    if(selectAll)command.clear();
                    for(size_t i=0;value[i]&&command.size()<120;i++){
                        if(value[i]>=32&&value[i]!=127)command+=value[i];
                    }
                    GlobalUnlock(handle);selectAll=false;
                }
            }
            CloseClipboard();invalidate();
        }return 0;
    case WM_HOTKEY:
        if(captureShortcut>=0){commitShortcut({LOWORD(lp),HIWORD(lp)});return 0;}
        if(shortcutAwaitRelease||GetTickCount64()<shortcutIgnoreUntil)return 0;
        if(shortcuts.slotFor(static_cast<int>(wp))==0)toggleShortcutVisibility();
        else if(shortcuts.slotFor(static_cast<int>(wp))==1)toggleShortcutPanel();
        return 0;
    case ForegroundMessage:updateVisibility();return 0;
    case WM_DROPFILES:{
        auto drop=reinterpret_cast<HDROP>(wp);UINT n=DragQueryFile(drop,0xFFFFFFFF,nullptr,0);
        for(UINT i=0;i<n&&files.size()<5;i++){
            UINT len=DragQueryFile(drop,i,nullptr,0);std::wstring path(len+1,L'\0');DragQueryFile(drop,i,path.data(),len+1);path.resize(len);
            if(std::find(files.begin(),files.end(),path)==files.end())files.push_back(path);
        }
        DragFinish(drop);changePage(4);return 0;
    }
    case WM_DISPLAYCHANGE:updateMonitor(MonitorFromPoint(POINT{0,0},MONITOR_DEFAULTTOPRIMARY));invalidate();return 0;
    case WM_SHOWWINDOW:
        if(!wp)PostMessage(wnd,ForegroundMessage,0,0);
        break;
    case WM_SIZE:
        if(wp==SIZE_MINIMIZED)PostMessage(wnd,ForegroundMessage,0,0);
        break;
    case WM_DPICHANGED:updateMonitor(currentMonitor);invalidate();return 0;
    case WM_SETTINGCHANGE:{
        readAppearancePreferences();materialDirty=true;wallpaper.key.clear();
        BOOL enabled=TRUE;
        if(SystemParametersInfo(SPI_GETCLIENTAREAANIMATION,0,&enabled,0))systemReducedMotion=!enabled;
        if(motionReduced()){
            snapMotion();
        }
        invalidate();return 0;
    }
    case VolumeMessage:
        volume=wp/10000.0f;audioMuted=static_cast<BOOL>(lp);
        volumeVisual.target=volume;volumeVisual.frequency=32;
        if(dragVolume||motionReduced())volumeVisual.snap(volume);
        if(!expanded)showStrip(audioMuted?L"Som desativado":L"Volume · "+std::to_wstring(static_cast<int>(volume*100))+L"%",1800);
        else invalidate();
        return 0;
    case TrayMessage:
        if(lp==WM_RBUTTONUP)contextMenu();
        if(lp==WM_LBUTTONUP){hidden=false;visibility.awaitLeave=false;}return 0;
    case WM_TIMER:{
        auto now=GetTickCount64();
        if(wp==1){
            double elapsed=lastFrame?std::min(.1,(now-lastFrame)/1000.0):.016;lastFrame=now;
            stepMotion(elapsed);render();
            if(shortcutClosing&&!expanded&&!height.moving()&&!width.moving()){
                finishShortcutOverlay();if(!testMode)updateVisibility();
            }
            if(pendingDesktopFocus&&!expanded&&!height.moving()&&!width.moving()){
                pendingDesktopFocus=false;
                if(!testMode&&GetForegroundWindow()==windowHandle&&IsWindow(desktopHost)){
                    SetForegroundWindow(desktopHost);
                    placeOnDesktop();
                }
            }
            if(!motionBusy()&&
               (!voicePreview||motionReduced())&&now>paintUntil){
                stopAnimationClock();
            }
        }else if(wp==2){
            if(expanded&&page==6){
                if(!dragSound&&soundChooser<0)refreshSound();
                if(soundTab==3&&!bluetooth.refreshing()&&now-lastBluetoothRefresh>10000){bluetooth.refresh();lastBluetoothRefresh=now;}
            }
            if(windowShown&&now-lastMaterialCheck>30000){materialDirty=true;lastMaterialCheck=now;render();}
            if(voicePreview&&now>=previewUntil){voicePreview=false;stripUntil=0;if(!expanded)setExpanded(false);}
            if(stripUntil&&now>=stripUntil){stripUntil=0;if(!expanded)setExpanded(false);}
            if(timerRunning&&!remaining()){
                timerRunning=false;timerSymbol.target=0;timerRemaining=0;
                showStrip(L"Tempo concluído. Faça uma pausa.",8000);
            }
            static std::wstring previousClock;auto clock=clockLabel();
            if(expanded){
                bool monitoring=page==3||(page==5&&settingsSection==4);
                if(!monitoring||now-lastSystemSample>=static_cast<ULONGLONG>(hardwareInterval))sampleSystem();
                requestHardware();render();
            }else if(timerRunning||clock!=previousClock)render();
            previousClock=clock;
        }else if(wp==3){
            audioAddon.poll();
            if(page==6&&soundTab==2&&audioAddon.snapshot().state==hext::AddonState::Downloading)invalidate(100);
            if(shortcutAwaitRelease){
                UINT mods=shortcutReleaseGuard.modifiers;
                bool held=(GetAsyncKeyState(shortcutReleaseGuard.key)&0x8000)||
                    ((mods&MOD_CONTROL)&&(GetAsyncKeyState(VK_CONTROL)&0x8000))||
                    ((mods&MOD_ALT)&&(GetAsyncKeyState(VK_MENU)&0x8000))||
                    ((mods&MOD_SHIFT)&&(GetAsyncKeyState(VK_SHIFT)&0x8000));
                if(!held)shortcutAwaitRelease=false;
            }
            if(captureShortcut>=0&&now>=shortcutCaptureUntil){cancelShortcutCapture();invalidate();}
            updateVisibility();
            auto latestHardware=hardwareMonitor.snapshot();
            if(latestHardware.sampled>hardwareSnapshot.sampled){
                hardwareSnapshot=std::move(latestHardware);
                if(expanded&&(page==3||(page==5&&settingsSection==4)))render();
            }
            pollAssistantConnection();
            pollSoundEffects();
            if(bluetooth.poll(bluetoothSnapshot)){bluetoothSelected=-1;bluetoothOffset=0;if(page==6)invalidate();}
            if(assistantBridge.poll()){
                if(assistantBridge.exitCode==0&&!assistantBridge.result.empty()){
                    conversation.push_back({false,hext::wide(assistantBridge.result).substr(0,12000)});
                    assistantStatus.clear();assistantScroll=0;
                }else assistantStatus=assistantBridge.exitCode==WAIT_TIMEOUT?L"O assistente demorou demais. Tente novamente.":
                    L"Não foi possível responder. Verifique a conexão do assistente.";
                invalidate();
            }
        }else if(wp==4){
            if(!assistantFocused||!expanded||page!=0||(!testMode&&!windowShown)){
                KillTimer(wnd,4);assistantCaretVisible=false;
            }else{assistantCaretVisible=!assistantCaretVisible;render();}
        }else if(wp==5){
            KillTimer(wnd,5);beginNavigationDrag();
        }
        return 0;
    }
    case WM_CLOSE:DestroyWindow(wnd);return 0;
    case WM_DESTROY:
        audioAddon.stop();
        cancelShortcutCapture();shortcuts.releaseAll();
        hardwareMonitor.stop();
        assistantBridge.cancel();
        connectionProbe.cancel();
        if(loginProcess){CloseHandle(loginProcess);loginProcess=nullptr;}
        if(audioEffectsProcess){CloseHandle(audioEffectsProcess);audioEffectsProcess=nullptr;}
        stopAnimationClock();KillTimer(wnd,2);KillTimer(wnd,3);KillTimer(wnd,4);KillTimer(wnd,5);tray(false);disconnectAudio();
        if(foregroundHook){UnhookWinEvent(foregroundHook);foregroundHook=nullptr;}
        release(accessibleUI);
        windowHandle=nullptr;PostQuitMessage(0);return 0;
    }
    return DefWindowProc(wnd,message,wp,lp);
}
bool initializeGraphics(){
    if(FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&factory)))return false;
    if(FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(&textFactory))))return false;
    auto props=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96);
    if(FAILED(factory->CreateDCRenderTarget(&props,&canvas)))return false;
    canvas->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    if(FAILED(canvas->CreateSolidColorBrush(color(0xFFFFFF),&brush)))return false;
    IDWriteFontCollection* collection=nullptr;
    if(SUCCEEDED(textFactory->GetSystemFontCollection(&collection))){
        UINT at=0;BOOL exists=FALSE;
        collection->FindFamilyName(L"Segoe UI Variable Text",&at,&exists);
        if(exists)uiFont=L"Segoe UI Variable Text";
        collection->FindFamilyName(L"Segoe Fluent Icons",&at,&exists);
        if(exists)iconFont=L"Segoe Fluent Icons";
        collection->Release();
    }
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=MaxW;info.bmiHeader.biHeight=-MaxH;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    memoryDC=CreateCompatibleDC(nullptr);dib=CreateDIBSection(memoryDC,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if(!memoryDC||!dib)return false;
    previousBitmap=static_cast<HBITMAP>(SelectObject(memoryDC,dib));return true;
}
bool exportFrame(const std::filesystem::path& path) {
    int w=static_cast<int>(std::ceil(width.value*dpi));
    int h=static_cast<int>(std::ceil((height.value+7)*dpi));
    BITMAPFILEHEADER file{};file.bfType=0x4d42;
    file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);
    file.bfSize=file.bfOffBits+w*h*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=-h;
    info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;info.biSizeImage=w*h*4;
    std::ofstream output(path,std::ios::binary);
    output.write(reinterpret_cast<char*>(&file),sizeof(file));
    output.write(reinterpret_cast<char*>(&info),sizeof(info));
    for(int y=0;y<h;y++)output.write(static_cast<char*>(pixels)+y*MaxW*4,w*4);
    return output.good();
}
int verifyRenderer(const std::filesystem::path& directory) {
    // Animation fixtures must be independent of the runner's accessibility defaults.
    // Reduced motion is exercised explicitly below; normal runtime keeps Windows preferences.
    reducedMotion=systemReducedMotion=false;
    std::ofstream report(directory/"verification.txt");
    int failures=0;
    auto check=[&](bool condition,const char* label){
        report<<(condition?"PASS: ":"FAIL: ")<<label<<"\n";if(!condition)failures++;
    };
    auto checkMenuPadding=[&](){
        return !hits.empty()&&std::abs(hits.front().rect.top-popoverRect.top-8)<.1f&&
            std::abs(popoverRect.bottom-hits.back().rect.bottom-8)<.1f;
    };
    auto styles=GetWindowLongPtr(windowHandle,GWL_EXSTYLE);
    check(!(styles&WS_EX_TOPMOST),"normal startup does not use always-on-top");
    check(!(styles&WS_EX_APPWINDOW)&&(styles&WS_EX_TOOLWINDOW),"no taskbar or Alt+Tab entry");
    check(styles&WS_EX_NOACTIVATE,"no implicit activation");
    check(!IsWindowVisible(windowHandle),"startup is hidden");
    requestHardware(true);
    dpi=1;appearance.snap(1);width.snap(736);height.snap(470);reveal.snap(1);pageFade.snap(1);expanded=true;
    restoreDesktopCompact();
    check(lastPresentedAlpha==0&&height.value==42&&width.value==184&&!IsWindowVisible(windowHandle),
          "desktop recovery replaces stale bitmap with transparent compact frame before show");
    double previousAlpha=0;bool fadeValid=true;
    for(int i=0;i<60;i++){
        stepMotion(1.0/60);render();
        fadeValid=fadeValid&&appearance.value>=previousAlpha&&appearance.value<=1;
        previousAlpha=appearance.value;
        if(i==5)fadeValid=fadeValid&&appearance.value>.1&&appearance.value<.75;
    }
    check(fadeValid&&appearance.value>.99,"desktop entry fades progressively without overshoot");
    beginDesktopExit();previousAlpha=1;fadeValid=true;
    for(int i=0;i<60;i++){
        stepMotion(1.0/60);render();
        fadeValid=fadeValid&&appearance.value<=previousAlpha&&appearance.value>=0;
        previousAlpha=appearance.value;
    }
    check(fadeValid&&lastPresentedAlpha==0,"desktop exit fades to transparent without resetting visible geometry");
    appearance.snap(1);
    width.snap(736);height.snap(470);reveal.snap(1);expanded=true;
    for(int i=0;i<7;i++){
        changePage(i);
        for(int n=0;n<90;n++)stepMotion(1.0/60);
        hardwareSnapshot=hardwareMonitor.snapshot();
        if(i==3)sampleSystem();
        render();
        check(SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded,"Direct2D + layered-window render");
        bool valid=true;std::set<int> identifiers;
        for(auto& h:hits){
            valid=valid&&h.rect.left>=0&&h.rect.right<=736&&h.rect.top>=0&&h.rect.bottom<=470;
            valid=valid&&identifiers.insert(h.id).second;
        }
        check(valid,"controls inside panel, unique keyboard targets");
        LONG childCount=0;
        bool accessValid=accessibleUI&&SUCCEEDED(accessibleUI->get_accChildCount(&childCount))&&
                         childCount==static_cast<LONG>(hits.size());
        for(LONG child=1;child<=childCount;child++){
            VARIANT identifier{};identifier.vt=VT_I4;identifier.lVal=child;
            BSTR name=nullptr;VARIANT role{};
            accessValid=accessValid&&SUCCEEDED(accessibleUI->get_accName(identifier,&name))&&name&&SysStringLen(name)>0;
            accessValid=accessValid&&SUCCEEDED(accessibleUI->get_accRole(identifier,&role))&&role.vt==VT_I4;
            SysFreeString(name);
        }
        check(accessValid,"MSAA names and semantic roles for visible controls");
        check(exportFrame(directory/("preview-"+std::to_string(i)+".bmp")),"export native render");
    }
    changePage(5);
    for(int section=0;section<6;section++){
        action(600+section);
        for(int n=0;n<90;n++)stepMotion(1.0/60);
        render();
        bool valid=std::all_of(hits.begin(),hits.end(),[](const Hit& h){
            return h.rect.left>=0&&h.rect.right<=736&&h.rect.top>=0&&h.rect.bottom<=470;
        });
        check(valid&&std::none_of(hits.begin(),hits.end(),[](const Hit& h){return h.id>=100&&h.id<107;}),
              "dedicated settings navigation and section controls fit panel");
        check(exportFrame(directory/("settings-"+std::to_string(section)+".bmp")),"export settings section");
    }
    check(settingsSection==5&&std::any_of(hits.begin(),hits.end(),[](const Hit& h){return h.id==605&&h.rect.top==164;}),
          "General is the first settings category");
    auto originalBindings=shortcuts.bindings;auto originalIDs=shortcuts.registeredIDs;
    beginShortcutCapture(0);render();
    check(captureShortcut==0&&!shortcutCaptureHook,"shortcut capture fixture does not install an input hook");
    check(exportFrame(directory/"settings-shortcut-capture.bmp"),"export shortcut capture state");
    auto originalAcquire=shortcuts.acquire;
    shortcuts.acquire=[](int,hext::Shortcut){SetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);return false;};
    commitShortcut({MOD_CONTROL|MOD_SHIFT,'J'});render();
    check(captureShortcut==0&&shortcuts.bindings==originalBindings&&shortcuts.registeredIDs==originalIDs,
          "shortcut conflict preserves both old bindings and allows retry");
    check(exportFrame(directory/"settings-shortcut-conflict.bmp"),"export occupied shortcut state");
    shortcuts.acquire=originalAcquire;commitShortcut({MOD_CONTROL|MOD_SHIFT,'J'});
    check(captureShortcut==-1&&shortcuts.bindings[0]==hext::Shortcut{MOD_CONTROL|MOD_SHIFT,'J'},"available shortcut commits and ends capture");
    beginShortcutCapture(1);commitShortcut(shortcuts.bindings[0]);
    check(captureShortcut==1&&shortcuts.bindings[1]==originalBindings[1],"two actions cannot share a shortcut");
    commitShortcut({0,VK_ESCAPE});check(captureShortcut==-1,"Escape cancels capture without collapsing island");
    shortcuts.bindings=originalBindings;shortcuts.registeredIDs=originalIDs;
    shortcutStatus[0].clear();shortcutStatus[1].clear();shortcutIgnoreUntil=0;
    setExpanded(false);for(int frame=0;frame<90;frame++)stepMotion(1.0/60);
    bool wasEligible=desktopEligible;desktopEligible=false;windowShown=false;
    toggleShortcutPanel();
    check(shortcutOverlay&&shortcutHold&&expanded&&width.target==736&&!hidden,"expand shortcut opens a hidden island outside the desktop");
    for(int frame=0;frame<90;frame++)stepMotion(1.0/60);
    render();
    check(exportFrame(directory/"shortcut-overlay.bmp"),"export expanded shortcut overlay state");
    toggleShortcutPanel();
    check(!expanded&&shortcutClosing&&shortcutOverlay&&height.value>42,"second shortcut press begins smooth collapse before removing overlay");
    for(int frame=0;frame<90;frame++)stepMotion(1.0/60);
    finishShortcutOverlay();check(!shortcutOverlay&&!shortcutHold&&height.value==42,"collapsed shortcut returns to normal desktop mode");
    hidden=false;toggleShortcutVisibility();
    check(hidden&&!windowShown&&!expanded&&!IsWindowVisible(windowHandle),"hide shortcut suppresses compact island and expanded panel");
    toggleShortcutVisibility();check(!hidden&&!expanded,"visibility shortcut restores automatic presence without expanding panel");
    desktopEligible=wasEligible;windowShown=false;appearance.snap(1);setExpanded(true);
    for(int frame=0;frame<90;frame++)stepMotion(1.0/60);
    // Exercise native interaction handlers without capturing the desktop or changing audio.
    auto originalNavigation=navigation;
    changePage(0);pageFade.snap(1);render();
    int dragY=static_cast<int>(NavigationOrder::row(navigation.position(3))+17);
    procedure(windowHandle,WM_LBUTTONDOWN,0,MAKELPARAM(80,dragY));
    procedure(windowHandle,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(80,111));
    check(navigationDragging&&navigation.items[0]==3&&page==0,"sidebar drag moves My PC while preserving active page");
    procedure(windowHandle,WM_LBUTTONUP,0,MAKELPARAM(80,111));
    for(int i=0;i<60;i++)stepMotion(1.0/60);
    check(!navigationHeld&&!navigationDragging&&navigationRows[3].value==94,"sidebar drop settles into the new row");
    render();check(exportFrame(directory/"navigation-reordered.bmp"),"export reordered sidebar");
    auto committedNavigation=navigation;
    procedure(windowHandle,WM_LBUTTONDOWN,0,MAKELPARAM(80,111));
    procedure(windowHandle,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(80,330));
    procedure(windowHandle,WM_CAPTURECHANGED,0,0);
    check(navigation.items==committedNavigation.items&&!navigationHeld,"capture loss cancels uncommitted sidebar reorder");
    navigation=originalNavigation;updateNavigationTargets(true);
    changePage(5);settingsSection=4;pageFade.snap(1);settingsFade.snap(1);
    unsigned beforeToggleMask=metricMask;metricMask|=1;render();
    auto& toggle=toggleMotion[380].position;toggle.snap(1);
    action(380);render();double startToggle=toggle.value;
    stepMotion(.04);render();double midToggle=toggle.value,midVelocity=toggle.velocity;
    check(startToggle==1&&midToggle>0&&midToggle<1&&midVelocity<0,"toggle thumb and color interpolate instead of jumping");
    check(exportFrame(directory/"toggle-transition.bmp"),"export intermediate toggle position");
    action(380);render();
    check(toggle.value==midToggle&&toggle.velocity==midVelocity&&toggle.target==1,"rapid toggle reversal preserves position and velocity");
    for(int i=0;i<60;i++)stepMotion(1.0/60);
    check(toggle.value==1&&!toggle.moving(),"toggle settles promptly after reversal");
    bool beforeReduced=reducedMotion;reducedMotion=true;action(380);render();
    check(toggle.value==0&&!toggle.moving(),"reduced motion also applies to toggle thumb and color");
    reducedMotion=beforeReduced;metricMask=beforeToggleMask;
    changePage(6);pageFade.snap(1);refreshSound();
    for(int frame=0;frame<60;frame++)stepMotion(1.0/60);
    bluetooth.refresh(false);
    for(int wait=0;wait<300&&bluetooth.refreshing();wait++)Sleep(10);
    bluetooth.poll(bluetoothSnapshot);
    for(int tab=0;tab<4;tab++){
        soundTab=tab;soundTabVisual.snap(tab);soundPageFade.snap(1);soundChooser=-1;render();
        bool valid=true;std::set<int> soundIDs;
        for(const auto& h:hits)valid=valid&&h.rect.left>=0&&h.rect.right<=736&&h.rect.top>=0&&h.rect.bottom<=470&&soundIDs.insert(h.id).second&&!h.name.empty();
        check(valid&&SUCCEEDED(lastRenderResult),"sound tab has named unique controls inside the island");
        check(exportFrame(directory/("sound-"+std::to_string(tab)+".bmp")),"export native sound tab using real read-only device enumeration");
    }
    soundTab=0;soundTabVisual.snap(0);soundChooser=0;render();
    check(exportFrame(directory/"sound-devices.bmp"),"export paginated output device selector");
    soundChooser=-1;
    sound.devices.clear();sound.reading[0]={};render();
    check(!controlEnabled(1020)&&!controlEnabled(1030)&&!controlEnabled(1032),"disconnected endpoint disables mute volume and balance");
    check(exportFrame(directory/"sound-unavailable.bmp"),"export no-endpoint fixture");
    refreshSound();
    bool wasInstalled=audioApo.installed,wasApoReady=apoEndpointReady;auto originalEffects=audioEffects;
    audioApo.installed=true;apoEndpointReady=true;soundTab=2;soundTabVisual.snap(2);audioEffects.enabled=true;audioEffects.gain=60;
    audioEffects.bass=20;audioEffects.mid=0;audioEffects.treble=-15;soundEffectsDirty=true;render();
    check(exportFrame(directory/"sound-effects-fixture.bmp"),"export configured-effects fixture without applying filters");
    check(controlEnabled(1045),"only configured endpoint enables applying effects");
    apoEndpointReady=false;
    check(!controlEnabled(1045)&&!controlEnabled(1041),"unconfigured endpoint cannot claim amplification is available");
    audioApo.installed=wasInstalled;apoEndpointReady=wasApoReady;audioEffects=originalEffects;soundEffectsDirty=false;
    soundTab=0;soundTabVisual.snap(0);changePage(5);settingsSection=4;
    unsigned originalMetrics=metricMask;
    metricMask=hext::AllMetrics;
    for(int metric=0;metric<hext::MetricCount;metric++){
        bool was=(metricMask&(1u<<metric))!=0;action(380+metric);
        check(((metricMask&(1u<<metric))!=0)!=was,"monitoring switch toggles selected metric");
    }
    changePage(3);for(int n=0;n<90;n++)stepMotion(1.0/60);render();
    check(std::none_of(hits.begin(),hits.end(),[](const Hit& h){return h.id>=520&&h.id<527;}),
          "disabled metrics are absent from My PC");
    check(exportFrame(directory/"monitoring-empty.bmp"),"export all-metrics-disabled state");
    metricMask=originalMetrics;changePage(5);settingsSection=4;
    for(int n=0;n<90;n++)stepMotion(1.0/60);
    render();
    check(exportFrame(directory/"settings-4.bmp"),"export configurable CPU GPU and memory switches");
    action(362);for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(hits.size()==3&&hits.front().id==741&&hits.back().id==743,"monitoring interval menu has explicit options");
    check(checkMenuPadding(),"monitoring interval menu has equal top and bottom padding");
    closePopover();popoverMotion.snap(0);settingsSection=3;
    int originalProvider=assistantProvider;
    assistantProvider=1;connectionState[1]=hext::ConnectionState::Disconnected;render();
    check(!assistantConfigured()&&std::any_of(hits.begin(),hits.end(),[](const Hit& h){return h.id==467&&h.name==L"Conectar";}),
          "Codex selection offers Connect and does not treat executable presence as authentication");
    check(exportFrame(directory/"assistant-connect.bmp"),"export Assistant connect action");
    connectionState[1]=hext::ConnectionState::Checking;render();
    check(!controlEnabled(467)&&!assistantConfigured(),"checking state prevents premature connection and duplicate probes");
    connectionState[1]=hext::ConnectionState::Disconnected;assistantProvider=originalProvider;
    action(465);
    for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(popoverOpen&&hits.size()==4&&hits.front().id==801&&hits.back().id==804,
          "Assistant provider options remain in settings with their own input scope");
    check(checkMenuPadding(),"provider menu has equal top and bottom padding");
    closePopover();popoverMotion.snap(0);
    action(600);
    for(int n=0;n<60;n++){stepMotion(1.0/60);render();}
    auto originalPalette=accentPalette;
    action(330);
    for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(colorPickerOpen&&hits.size()==5&&std::all_of(hits.begin(),hits.end(),[](const Hit& h){
        return h.id>=331&&h.id<=336&&h.rect.left>=0&&h.rect.right<=736&&h.rect.bottom<=470;
    }),"color wheel has isolated accessible controls inside panel");
    draftColor={26,.75,.92};render();
    check(exportFrame(directory/"color-wheel.bmp"),"export color wheel with brightness and live preview");
    UINT selectedColor=hext::rgb(draftColor);action(335);
    check(!colorPickerOpen&&accentPalette.current()==selectedColor,"apply custom color moves it to first suggestion");
    colorPickerMotion.snap(0);render();
    check(exportFrame(directory/"color-recent.bmp"),"export updated recent color suggestions");
    action(301);
    check(accentPalette.recent[1]==selectedColor&&accentPalette.current()==originalPalette.current(),
          "selecting a suggested color reorders history without duplicates");
    action(330);draftColor={170,1,1};action(336);
    check(accentPalette.current()==originalPalette.current(),"cancel color preview preserves selected color");
    colorPickerMotion.snap(0);accentPalette=originalPalette;
    for(int n=0;n<90;n++)stepMotion(1.0/60);
    render();action(317);
    for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(popoverOpen&&hits.size()==2&&hits.front().id==701&&hits.back().id==702,
          "material selector opens actual option menu with isolated keyboard scope");
    check(exportFrame(directory/"settings-menu.bmp"),"export settings option menu");
    check(checkMenuPadding(),"material menu has equal top and bottom padding");
    procedure(windowHandle,WM_KEYDOWN,VK_DOWN,0);
    procedure(windowHandle,WM_KEYDOWN,VK_DOWN,0);
    procedure(windowHandle,WM_KEYDOWN,VK_RETURN,0);
    check(!popoverOpen&&!translucent,"keyboard selects solid material from settings menu");
    translucent=true;materialDirty=true;
    action(310);
    for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(checkMenuPadding(),"shape menu has equal top and bottom padding");
    check(exportFrame(directory/"settings-shape.bmp"),"export balanced shape menu");
    closePopover();popoverMotion.snap(0);
    action(314);
    for(int n=0;n<45;n++){stepMotion(1.0/60);render();}
    check(hits.size()==4&&hits.front().id==721&&hits.back().id==724,
          "scale selector presents four explicit options");
    check(checkMenuPadding(),"scale menu has equal top and bottom padding");
    check(exportFrame(directory/"settings-scale.bmp"),"export balanced scale menu");
    closePopover();popoverMotion.snap(0);
    action(610);check(page==settingsReturnPage&&page!=5,"settings back returns to previous app page");
    changePage(0);
    for(int n=0;n<90;n++)stepMotion(1.0/60);
    render();
    check(std::none_of(hits.begin(),hits.end(),[](const Hit& h){return h.id==200||h.id==220||h.id==225;}),
          "Assistant replaces former media and timer controls on home page");
    assistantInput=L"teste de entrada";action(450);
    procedure(windowHandle,WM_CHAR,VK_BACK,0);
    check(assistantInput==L"teste de entrad","Assistant input accepts editing without invoking local commands");
    assistantInput.clear();action(450);render();
    check(assistantCaretVisible&&assistantCaret==0&&std::abs(lastAssistantCaretX-218)<.1f,
          "empty message input shows a real drawing caret at the beginning");
    check(exportFrame(directory/"assistant-caret.bmp"),"export visible empty-field caret");
    assistantInput=L"AB";action(450);
    procedure(windowHandle,WM_KEYDOWN,VK_LEFT,0);procedure(windowHandle,WM_CHAR,L'x',0);
    check(assistantInput==L"AxB"&&assistantCaret==2,"caret inserts text in the middle");
    procedure(windowHandle,WM_KEYDOWN,VK_HOME,0);procedure(windowHandle,WM_KEYDOWN,VK_DELETE,0);
    check(assistantInput==L"xB"&&assistantCaret==0,"Home and Delete edit at caret location");
    assistantInput=L"A\U0001F600B";action(450);
    procedure(windowHandle,WM_KEYDOWN,VK_LEFT,0);procedure(windowHandle,WM_KEYDOWN,VK_LEFT,0);
    procedure(windowHandle,WM_KEYDOWN,VK_DELETE,0);
    check(assistantInput==L"AB","caret movement and Delete preserve UTF-16 surrogate boundaries");
    assistantInput.assign(200,L'a');action(450);render();
    check(assistantTextScroll>0&&lastAssistantCaretX<=640&&lastAssistantCaretX>=218,
          "long input scrolls horizontally to keep caret visible");
    positionAssistantCaret(218);
    check(assistantCaret<assistantInput.size(),"mouse positioning moves caret within scrolled text");
    procedure(windowHandle,WM_KEYDOWN,VK_HOME,0);render();
    check(std::abs(lastAssistantCaretX-218)<.1f,"Home restores caret and text scroll to field start");
    procedure(windowHandle,WM_KILLFOCUS,0,0);
    check(!assistantFocused&&!assistantCaretVisible,"caret disappears when focus leaves input");
    assistantInput.clear();assistantFocused=false;assistantTextScroll=0;
    sendState.snap(0);render();check(exportFrame(directory/"assistant-send.bmp"),"export centered send arrow");
    assistantBridge.busy=true;sendState.snap(1);render();
    check(exportFrame(directory/"assistant-stop.bmp"),"export centered stop control using renderer fixture");
    assistantBridge.busy=false;sendState.snap(0);
    conversation={{true,L"Esta é uma conversa de exemplo para verificar o layout."},
        {false,L"Prévia de layout: respostas longas quebram em várias linhas e a conversa permite rolagem. Nenhuma mensagem foi enviada a um provedor neste teste."}};
    render();check(exportFrame(directory/"assistant-layout.bmp"),"export wrapped conversation layout using explicit test fixture");
    conversation.clear();
    setExpanded(false);stepMotion(1.0/60);render();
    double collapsingHeight=height.value,collapsingVelocity=height.velocity;
    restackDesktop();
    check(collapsingHeight>42&&collapsingHeight<470&&height.value==collapsingHeight&&
          height.velocity==collapsingVelocity&&height.target==42&&appearance.target==1,
          "desktop restack preserves active collapse position and velocity after interaction");
    for(int n=0;n<100;n++)stepMotion(1.0/60);
    check(height.value==42,"interaction followed by pointer leave completes animated collapse");
    setExpanded(true);for(int n=0;n<100;n++)stepMotion(1.0/60);
    for(float scale:{1.0f,1.25f,1.5f,2.0f}){
        dpi=scale;render();
        RECT bounds{};GetWindowRect(windowHandle,&bounds);
        check(SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded&&
              bounds.right-bounds.left==static_cast<int>(std::ceil(736*scale)),
              "native rendering at 100/125/150/200 percent scale");
    }
    dpi=1;
    width.snap(184);height.snap(42);reveal.snap(0);
    width.target=736;height.target=470;reveal.target=1;
    bool framesValid=true;
    LARGE_INTEGER frequency,start,end;QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    for(int i=0;i<90;i++){
        if(i==16){width.target=184;height.target=42;reveal.target=0;}
        if(i==24){width.target=736;height.target=470;reveal.target=1;}
        stepMotion(1.0/60);render();
        framesValid=framesValid&&SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded;
    }
    QueryPerformanceCounter(&end);
    check(framesValid,"90 real Direct2D transition frames, including interrupted collapse");
    report<<"CPU render mean ms/frame: "<<(end.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart/90<<"\n";
    width.snap(736);height.snap(470);reveal.snap(1);
    for(int preset=0;preset<3;preset++){
        motionStyle=preset;width.snap(184);height.snap(42);reveal.snap(0);
        setExpanded(true);bool valid=true;
        for(int i=0;i<75;i++){
            if(i==9)setExpanded(false);
            if(i==14)setExpanded(true);
            stepMotion(1.0/60);render();
            valid=valid&&SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded&&
                std::isfinite(width.value)&&std::isfinite(height.value)&&width.value>0&&height.value>0;
        }
        check(valid,"native smooth/snappy/bouncy expansion, interruption and resume");
    }
    changePage(0);
    for(int i=0;i<90;i++)stepMotion(1.0/60);
    render();
    procedure(windowHandle,WM_LBUTTONUP,0,MAKELPARAM(80,static_cast<int>(NavigationOrder::row(navigation.position(2))+17)));
    check(page==2,"sidebar hit testing navigates to timer");
    for(int i=0;i<60;i++){stepMotion(1.0/60);render();}
    procedure(windowHandle,WM_LBUTTONUP,0,MAKELPARAM(699,31));
    for(int i=0;i<45;i++){stepMotion(1.0/60);render();}
    check(popoverOpen&&std::all_of(hits.begin(),hits.end(),[](const Hit& h){return h.id>=901&&h.id<=905;}),
          "popover has independent input scope");
    check(exportFrame(directory/"preview-menu.bmp"),"export native context-menu state");
    check(checkMenuPadding(),"context menu including exit separator has equal outer padding");
    procedure(windowHandle,WM_KEYDOWN,VK_DOWN,0);
    check(keyboardId==901,"menu keyboard navigation begins with first action");
    procedure(windowHandle,WM_KEYDOWN,VK_ESCAPE,0);
    check(!popoverOpen&&expanded,"Escape closes menu before collapsing island");
    for(int i=0;i<45;i++){stepMotion(1.0/60);render();}
    bool originalTransparency=systemTransparency;
    systemTransparency=false;render();
    check(SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded,"solid material fallback");
    check(exportFrame(directory/"preview-solid.bmp"),"export solid material state");
    systemTransparency=originalTransparency;
    motionStyle=1;
    command=L"timer 1";executeCommand();
    check(timerRunning&&remaining()>59000&&remaining()<=60000,"timer command starts countdown");
    command=L"pausar timer";executeCommand();
    check(!timerRunning&&timerRemaining>59000,"pause preserves remaining time");
    auto duration=timerDuration;
    command=L"timer 0";executeCommand();
    check(timerDuration==duration&&command==L"timer 0","invalid duration preserves valid state and input");
    command=L"volume 101";executeCommand();
    check(command==L"volume 101","out-of-range volume rejected without changing audio");
    reducedMotion=true;setExpanded(false);
    check(width.value==184&&height.value==42&&reveal.value==0,"reduced-motion collapse snaps immediately");
    setExpanded(true);
    check(width.value==736&&height.value==470&&reveal.value==1,"reduced-motion expansion snaps immediately");
    report<<"Failures: "<<failures<<"\n";
    return failures?1:0;
}
int verifyHardware(const std::filesystem::path& directory){
    std::ofstream report(directory/"hardware-check.txt");
    requestHardware(true);auto started=GetTickCount64();
    while(GetTickCount64()-started<5000){
        hardwareSnapshot=hardwareMonitor.snapshot();
        if(hardwareSnapshot.sampled)break;
        Sleep(20);
    }
    sampleSystem();
    report<<"Sample received: "<<bool(hardwareSnapshot.sampled)<<"\n";
    report<<"GPU: "<<hext::utf8(hardwareSnapshot.gpuName)<<"\n";
    auto reading=[&](const char* label,const hext::Reading& value){
        report<<label<<": ";
        if(value.available)report<<value.value<<" ("<<hext::utf8(value.source)<<")\n";else report<<"unavailable\n";
    };
    reading("CPU temperature C",hardwareSnapshot.cpuTemperature);reading("CPU power W",hardwareSnapshot.cpuPower);
    reading("GPU temperature C",hardwareSnapshot.gpuTemperature);reading("GPU power W",hardwareSnapshot.gpuPower);
    reading("GPU utilization percent",hardwareSnapshot.gpuLoad);
    report<<"Afterburner shared memory active: "<<hardwareSnapshot.afterburner<<"\nWMI sensor provider active: "<<hardwareSnapshot.wmi<<"\n";
    report<<"Read-only queries; no hardware profiles applied and no sensor application launched.\n";
    dpi=1;page=previousPage=3;pageFade.snap(1);expanded=true;
    navPosition.snap(94+44*3);
    width.snap(736);height.snap(470);reveal.snap(1);appearance.snap(1);render();
    exportFrame(directory/"hardware-live.bmp");
    page=previousPage=5;settingsSection=4;settingsFade.snap(1);render();
    exportFrame(directory/"settings-monitoring.bmp");
    openSettingsMenu(360);popoverMotion.snap(1);render();
    exportFrame(directory/"sensor-sources.bmp");
    return hardwareSnapshot.sampled?0:1;
}
int verifyAssistantConnection(const std::filesystem::path& directory){
    std::ofstream report(directory/"connection-check.txt");
    report<<"Provider: "<<hext::utf8(assistantName())<<"\n";
    if(!assistantProvider){report<<"No provider selected.\n";return 2;}
    connectAssistant();auto started=GetTickCount64();
    while(connectionState[assistantProvider]==hext::ConnectionState::Checking&&GetTickCount64()-started<47000){
        pollAssistantConnection();Sleep(20);
    }
    unsigned verified=connectionChecks[assistantProvider];
    report<<"Version verified: "<<bool(verified&1)<<"\nCLI capabilities verified: "<<bool(verified&2)
          <<"\nAuthentication verified: "<<bool(verified&4)<<"\n";
    report<<"Connected: "<<assistantConfigured()<<"\n";
    report<<"No inference request was sent. Raw authentication output is not exported.\n";
    page=5;settingsSection=3;previousPage=5;pageFade.snap(1);settingsFade.snap(1);
    expanded=true;width.snap(736);height.snap(470);reveal.snap(1);appearance.snap(1);render();
    exportFrame(directory/"assistant-connection-result.bmp");
    return assistantConfigured()?0:1;
}
void disposeGraphics() {
    release(colorWheelBitmap);
    wallpaper.clear();
    for(auto& item:iconFormats)release(item.second);
    release(brush);release(canvas);release(factory);
    for(auto& f:fonts)release(f.second);
    fonts.clear();release(textFactory);
    if(memoryDC){SelectObject(memoryDC,previousBitmap);DeleteObject(dib);DeleteDC(memoryDC);}
}
bool exportStagePNG(IWICImagingFactory* wic,const std::filesystem::path& file){
    constexpr int stageW=780,stageH=512;
    std::vector<std::uint8_t> stage(stageW*stageH*4);
    int w=static_cast<int>(std::ceil(width.value)),h=static_cast<int>(std::ceil(height.value+10));
    int left=(stageW-w)/2;
    for(int y=0;y<stageH;y++)for(int x=0;x<stageW;x++){
        size_t destination=(static_cast<size_t>(y)*stageW+x)*4;
        int sourceX=x-left;
        if(sourceX>=0&&sourceX<w&&y<h){
            auto source=static_cast<std::uint8_t*>(pixels)+(y*MaxW+sourceX)*4;
            int opacity=lastPresentedAlpha,inverse=255-source[3]*opacity/255;
            stage[destination]=static_cast<std::uint8_t>(std::min(255,source[0]*opacity/255+76*inverse/255));
            stage[destination+1]=static_cast<std::uint8_t>(std::min(255,source[1]*opacity/255+72*inverse/255));
            stage[destination+2]=static_cast<std::uint8_t>(std::min(255,source[2]*opacity/255+70*inverse/255));
        }else{stage[destination]=76;stage[destination+1]=72;stage[destination+2]=70;}
        stage[destination+3]=255;
    }
    IWICStream* stream=nullptr;IWICBitmapEncoder* encoder=nullptr;IWICBitmapFrameEncode* frame=nullptr;
    HRESULT result=wic->CreateStream(&stream);
    if(SUCCEEDED(result))result=stream->InitializeFromFilename(file.c_str(),GENERIC_WRITE);
    if(SUCCEEDED(result))result=wic->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);
    if(SUCCEEDED(result))result=encoder->Initialize(stream,WICBitmapEncoderNoCache);
    if(SUCCEEDED(result))result=encoder->CreateNewFrame(&frame,nullptr);
    if(SUCCEEDED(result))result=frame->Initialize(nullptr);
    if(SUCCEEDED(result))result=frame->SetSize(stageW,stageH);
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
    if(SUCCEEDED(result))result=frame->SetPixelFormat(&format);
    if(SUCCEEDED(result)&&format!=GUID_WICPixelFormat32bppBGRA)result=E_FAIL;
    if(SUCCEEDED(result))result=frame->WritePixels(stageH,stageW*4,static_cast<UINT>(stage.size()),stage.data());
    if(SUCCEEDED(result))result=frame->Commit();
    if(SUCCEEDED(result))result=encoder->Commit();
    release(frame);release(encoder);release(stream);return SUCCEEDED(result);
}
int recordMotion(const std::filesystem::path& directory){
    auto output=directory/"motion-frames";std::filesystem::create_directories(output);
    std::ofstream report(directory/"recording.txt");
    IWICImagingFactory* wic=nullptr;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),reinterpret_cast<void**>(&wic))))return 1;
    dpi=1;expanded=false;appearance.snap(0);appearance.target=1;appearance.frequency=18;
    width.snap(184);height.snap(42);reveal.snap(0);
    page=previousPage=0;navPosition.snap(94);pageFade.snap(1);
    timerX.snap(590);timerY.snap(294);timerSize.snap(34);
    timerButtonX.snap(224);timerButtonY.snap(365);timerButtonW.snap(138);timerButtonH.snap(30);
    reducedMotion=systemReducedMotion=false;
    timerRunning=false;timerDuration=timerRemaining=300000;
    bool valid=true;
    ULONGLONG started=GetTickCount64();
    for(int i=0;i<600;i++){
        if(i==45)setExpanded(true);
        if(i==80)action(450);
        if(i==100)assistantBridge.busy=true; // renderer fixture, no provider request
        if(i==112&&GetCaretBlinkTime()!=INFINITE)procedure(windowHandle,WM_TIMER,4,0);
        if(i==120)assistantBridge.busy=false;
        if(i==130)changePage(3);
        if(page==3&&i%60==0)requestHardware(true);
        if(i==205)hoverId=12;
        if(i==210)openPopover();
        if(i==230)hoverId=901;
        if(i==250){hoverId=-1;closePopover();changePage(5);}
        if(i==295)action(330);
        if(i==315)draftColor={26,.75,.92};
        if(i==340)action(335);
        if(i==375)action(604);
        if(i==386||i==396||i==405||i==416)action(380);
        if(i==430)action(360);
        if(i==455)closePopover();
        if(i==460)changePage(6);
        if(i==475)action(1001);
        if(i==490)action(1000);
        if(i==505)changePage(0);
        if(i==520){
            int y=static_cast<int>(NavigationOrder::row(navigation.position(3))+17);
            procedure(windowHandle,WM_LBUTTONDOWN,0,MAKELPARAM(80,y));
        }
        if(i>520&&i<=538){
            float from=NavigationOrder::row(dragOriginalNavigation.position(3))+17;
            int y=static_cast<int>(blend(from,111,(i-520)/18.f));
            procedure(windowHandle,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(80,y));
        }
        if(i==540)procedure(windowHandle,WM_LBUTTONUP,0,MAKELPARAM(80,111));
        if(i==555)setExpanded(false);
        if(i==580){appearance.target=0;appearance.frequency=26;}
        auto sampled=hardwareMonitor.snapshot();
        if(sampled.sampled>hardwareSnapshot.sampled)hardwareSnapshot=std::move(sampled);
        stepMotion(1.0/60);render();
        wchar_t name[40];swprintf(name,40,L"motion-%04d.png",i);
        valid=valid&&SUCCEEDED(lastRenderResult)&&lastUpdateSucceeded;
        if(!exportStagePNG(wic,output/name)){valid=false;break;}
        ULONGLONG next=started+(i+1)*1000/60;
        if(next>GetTickCount64())Sleep(static_cast<DWORD>(next-GetTickCount64()));
    }
    release(wic);
    report<<"Renderer recording: "<<(valid?"PASS":"FAIL")<<"\n"
          <<"600 native frames; 60 fps timeline; interrupted toggles, sidebar drag, sound tabs, send/stop fixtures and live hardware; no screen capture, injected input or inference request.\n"
          <<"Wallpaper material loaded: "<<wallpaper.ready<<"\n";
    return valid?0:1;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR arguments,int){
    bool recording=std::wstring(arguments).find(L"--record-motion")!=std::wstring::npos;
    bool connectionCheck=std::wstring(arguments).find(L"--verify-connection")!=std::wstring::npos;
    bool hardwareCheck=std::wstring(arguments).find(L"--verify-hardware")!=std::wstring::npos;
    testMode=recording||connectionCheck||hardwareCheck||std::wstring(arguments).find(L"--verify-render")!=std::wstring::npos;
    bool effectsHelper=std::wstring(arguments).rfind(L"--apply-audio-effects",0)==0;
    HANDLE single=testMode||effectsHelper?nullptr:CreateMutex(nullptr,FALSE,L"Local\\HextIsland");
    if(!testMode&&!effectsHelper&&GetLastError()==ERROR_ALREADY_EXISTS){
        MessageBox(nullptr,L"O Hext já está aberto. Passe o mouse no centro superior da área de trabalho.",L"Hext",MB_OK);
        if(single)CloseHandle(single);
        return 0;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    int argumentCount=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argumentCount);
    if(argv&&argumentCount>1&&std::wstring(argv[1])==L"--apply-audio-effects"){
        int result=1;
        if(argumentCount==8){
            auto integer=[](const wchar_t* s,int& value){wchar_t* end=nullptr;long n=wcstol(s,&end,10);
                if(!s[0]||*end||n<-120||n>120)return false;
                value=static_cast<int>(n);return true;};
            int enabled=0;hext::AudioEffects effects;
            if(integer(argv[3],enabled)&&enabled>=0&&enabled<=1&&integer(argv[4],effects.gain)&&integer(argv[5],effects.bass)&&
                integer(argv[6],effects.mid)&&integer(argv[7],effects.treble)){
                effects.enabled=enabled!=0;hext::ApoInstallation installation;installation.detect();result=installation.apply(argv[2],effects);
            }
        }
        LocalFree(argv);CoUninitialize();if(single)CloseHandle(single);return result;
    }
    if(argv)LocalFree(argv);
    wchar_t path[32768];GetModuleFileName(nullptr,path,32768);configPath=path;
    configPath=configPath.substr(0,configPath.find_last_of(L"\\/")+1)+L"hext.ini";
    auto setting=[](const wchar_t* key,int fallback){return static_cast<int>(GetPrivateProfileInt(L"Ilha",key,fallback,configPath.c_str()));};
    auto recent=accentPalette.recent;
    int legacyAccent=std::clamp(setting(L"accent",1),0,3);
    int savedAccent=setting(L"accentRGB",-1);
    for(int i=0;i<4;i++)recent[i]=static_cast<UINT>(setting((L"recent"+std::to_wstring(i)).c_str(),recent[i]));
    accentPalette.restore(recent,savedAccent>=0&&savedAccent<=0xFFFFFF?static_cast<UINT>(savedAccent):accentPalette.recent[legacyAccent]);
    floating=setting(L"floating",0);reducedMotion=setting(L"reducedMotion",0);
    motionStyle=std::clamp(setting(L"motion",1),0,2);translucent=setting(L"material",1)!=0;
    assistantProvider=std::clamp(setting(L"assistantProvider",0),0,3);
    metricMask=static_cast<unsigned>(setting(L"metrics",hext::AllMetrics))&hext::AllMetrics;
    hardwareInterval=setting(L"hardwareInterval",1000);
    if(hardwareInterval!=1000&&hardwareInterval!=2000&&hardwareInterval!=5000)hardwareInterval=1000;
    auto savedOrder=navigation.items;
    for(int i=0;i<7;i++)savedOrder[i]=setting((L"nav"+std::to_wstring(i)).c_str(),savedOrder[i]);
    navigation.restore(savedOrder);updateNavigationTargets(true);
    for(int slot=0;slot<2;slot++){
        hext::Shortcut saved{static_cast<UINT>(setting((L"shortcutMods"+std::to_wstring(slot)).c_str(),hext::DefaultShortcuts[slot].modifiers)),
            static_cast<UINT>(setting((L"shortcutKey"+std::to_wstring(slot)).c_str(),hext::DefaultShortcuts[slot].key))};
        if(saved.valid())shortcuts.bindings[slot]=saved;
    }
    if(shortcuts.bindings[0]==shortcuts.bindings[1])shortcuts.bindings=hext::DefaultShortcuts;
    for(int i=0;i<2;i++){
        wchar_t saved[32768]{};GetPrivateProfileStringW(L"Som",i?L"input":L"output",L"",saved,32768,configPath.c_str());sound.selected[i]=saved;
    }
    auto soundSetting=[](const wchar_t* key,int fallback){return static_cast<int>(GetPrivateProfileIntW(L"Som",key,fallback,configPath.c_str()));};
    audioEffects.enabled=soundSetting(L"enabled",0)!=0;audioEffects.gain=std::clamp(soundSetting(L"gain",0),0,120);
    audioEffects.bass=std::clamp(soundSetting(L"bass",0),-120,120);audioEffects.mid=std::clamp(soundSetting(L"mid",0),-120,120);
    audioEffects.treble=std::clamp(soundSetting(L"treble",0),-120,120);
    for(int i=1;i<4;i++){
        wchar_t saved[32768]{};
        GetPrivateProfileString(L"Ilha",(L"assistantPath"+std::to_wstring(i)).c_str(),L"",saved,32768,configPath.c_str());
        assistantPath[i]=saved;
    }
    detectAssistant(assistantProvider);
    readAppearancePreferences();
    zoom=std::clamp(setting(L"zoom",100)/100.0f,.8f,1.3f);
    BOOL systemAnimations=TRUE;
    if(SystemParametersInfo(SPI_GETCLIENTAREAANIMATION,0,&systemAnimations,0))systemReducedMotion=!systemAnimations;
    updateMonitor(MonitorFromPoint(POINT{0,0},MONITOR_DEFAULTTOPRIMARY));pageFade.snap(1);
    appIcon=LoadIcon(instance,MAKEINTRESOURCE(1));
    if(!appIcon)appIcon=LoadIcon(nullptr,IDI_APPLICATION);
    WNDCLASS wc{};wc.lpfnWndProc=procedure;wc.hInstance=instance;wc.lpszClassName=L"HextIslandWindow";
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=appIcon;RegisterClass(&wc);
    windowHandle=CreateWindowEx(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,wc.lpszClassName,L"Hext",
        WS_POPUP,0,0,184,49,nullptr,nullptr,instance,nullptr);
    if(!windowHandle||!initializeGraphics()){
        MessageBox(nullptr,L"Não foi possível iniciar a interface Direct2D.",L"Hext",MB_ICONERROR);return 1;
    }
    volumeCallback=new VolumeCallback;connectAudio();sampleSystem();
    shortcuts.acquire=[](int id,hext::Shortcut binding){return testMode?true:RegisterHotKey(windowHandle,id,binding.modifiers|MOD_NOREPEAT,binding.key)!=FALSE;};
    shortcuts.relinquish=[](int id){if(!testMode)UnregisterHotKey(windowHandle,id);};
    if(!testMode)for(int slot=0;slot<2;slot++){
        if(shortcuts.replace(slot,shortcuts.bindings[slot])!=hext::ShortcutResult::Saved)
            shortcutStatus[slot]=L"Atalho indisponível. Clique para escolher outra combinação.";
    }
    accessibleUI=new HextAccessible;
    if(testMode){
        int result=hardwareCheck?verifyHardware(std::filesystem::path(configPath).parent_path()):
            connectionCheck?verifyAssistantConnection(std::filesystem::path(configPath).parent_path()):
            recording?recordMotion(std::filesystem::path(configPath).parent_path()):
            verifyRenderer(std::filesystem::path(configPath).parent_path());
        DestroyWindow(windowHandle);
        volumeCallback->Release();disposeGraphics();CoUninitialize();return result;
    }
    taskbarCreated=RegisterWindowMessage(L"TaskbarCreated");
    DragAcceptFiles(windowHandle,TRUE);tray(true);
    ShowWindow(windowHandle,SW_HIDE);
    foregroundHook=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,onForeground,0,0,WINEVENT_OUTOFCONTEXT);
    SetTimer(windowHandle,2,1000,nullptr);SetTimer(windowHandle,3,80,nullptr);
    if(assistantProvider)connectAssistant();
    MSG msg;while(GetMessage(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessage(&msg);}
    volumeCallback->Release();disposeGraphics();
    CoUninitialize();if(single)CloseHandle(single);return 0;
}

