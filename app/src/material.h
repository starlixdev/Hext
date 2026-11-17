#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <wincodec.h>
#include <d2d1.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>

namespace hext {
template<class T> void drop(T*& value){if(value){value->Release();value=nullptr;}}
// Cached material derived exclusively from the wallpaper file, never the screen.
// This is intentionally NOT advertised as DWM Acrylic or Apple's vibrancy API.
struct WallpaperMaterial {
    ID2D1Bitmap* bitmap=nullptr;
    std::wstring key;
    float logicalWidth=800,logicalHeight=540;
    bool ready=false;
    ~WallpaperMaterial(){drop(bitmap);}
    void clear(){drop(bitmap);key.clear();ready=false;}
    static void blur(std::vector<std::uint8_t>& data,int w,int h,int radius){
        std::vector<std::uint8_t> temp(data.size());
        for(int pass=0;pass<3;pass++){
            for(int axis=0;axis<2;axis++){
                int length=axis?h:w,lines=axis?w:h,diameter=radius*2+1;
                for(int line=0;line<lines;line++){
                    int sums[4]{};
                    auto offset=[&](int at){at=std::clamp(at,0,length-1);return (axis?(at*w+line):(line*w+at))*4;};
                    for(int at=-radius;at<=radius;at++){
                        int p=offset(at);for(int c=0;c<4;c++)sums[c]+=data[p+c];
                    }
                    for(int at=0;at<length;at++){
                        int dst=offset(at),remove=offset(at-radius),add=offset(at+radius+1);
                        for(int c=0;c<4;c++){
                            temp[dst+c]=static_cast<std::uint8_t>(sums[c]/diameter);
                            sums[c]+=data[add+c]-data[remove+c];
                        }
                    }
                }
                data.swap(temp);
            }
        }
    }
    bool refresh(ID2D1RenderTarget* target,RECT monitor,float scale,float panelW,float panelH){
        if(!target)return false;
        IDesktopWallpaper* desktop=nullptr;
        std::wstring path;
        DESKTOP_WALLPAPER_POSITION position=DWPOS_FILL;
        COLORREF background=RGB(25,25,28);
        if(SUCCEEDED(CoCreateInstance(CLSID_DesktopWallpaper,nullptr,CLSCTX_ALL,
            __uuidof(IDesktopWallpaper),reinterpret_cast<void**>(&desktop)))){
            desktop->GetPosition(&position);desktop->GetBackgroundColor(&background);
            UINT count=0;desktop->GetMonitorDevicePathCount(&count);
            for(UINT i=0;i<count;i++){
                LPWSTR id=nullptr,file=nullptr;RECT r{};
                if(SUCCEEDED(desktop->GetMonitorDevicePathAt(i,&id))&&
                   SUCCEEDED(desktop->GetMonitorRECT(id,&r))&&EqualRect(&r,&monitor)&&
                   SUCCEEDED(desktop->GetWallpaper(id,&file))&&file)path=file;
                CoTaskMemFree(file);CoTaskMemFree(id);
                if(!path.empty())break;
            }
            drop(desktop);
        }
        if(path.empty()){
            wchar_t file[32768]{};
            if(SystemParametersInfoW(SPI_GETDESKWALLPAPER,32768,file,0))path=file;
        }
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&attributes);
        std::wstring next=path+L"|"+std::to_wstring(position)+L"|"+std::to_wstring(scale)+L"|"+
            std::to_wstring(monitor.left)+L"|"+std::to_wstring(monitor.top)+L"|"+
            std::to_wstring(monitor.right)+L"|"+std::to_wstring(monitor.bottom)+L"|"+
            std::to_wstring(attributes.ftLastWriteTime.dwHighDateTime)+L"|"+
            std::to_wstring(attributes.ftLastWriteTime.dwLowDateTime);
        if(next==key&&ready)return true;
        clear();key=next;logicalWidth=panelW+64;logicalHeight=panelH+64;
        if(path.empty())return false;
        IWICImagingFactory* wic=nullptr;
        IWICBitmapDecoder* decoder=nullptr;
        IWICBitmapFrameDecode* frame=nullptr;
        IWICBitmapScaler* scaler=nullptr;
        IWICFormatConverter* converter=nullptr;
        HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
            __uuidof(IWICImagingFactory),reinterpret_cast<void**>(&wic));
        if(SUCCEEDED(hr))hr=wic->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,&decoder);
        if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);
        UINT iw=0,ih=0,dw=0,dh=0;
        if(SUCCEEDED(hr))hr=frame->GetSize(&iw,&ih);
        if(SUCCEEDED(hr)&&iw&&ih){
            double factor=std::min(1.0,1920.0/std::max(iw,ih));
            dw=std::max(1U,static_cast<UINT>(iw*factor));dh=std::max(1U,static_cast<UINT>(ih*factor));
            hr=wic->CreateBitmapScaler(&scaler);
            if(SUCCEEDED(hr))hr=scaler->Initialize(frame,dw,dh,WICBitmapInterpolationModeFant);
        }else hr=E_FAIL;
        if(SUCCEEDED(hr))hr=wic->CreateFormatConverter(&converter);
        if(SUCCEEDED(hr))hr=converter->Initialize(scaler,GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
        std::vector<std::uint8_t> image;
        if(SUCCEEDED(hr)){
            image.resize(static_cast<size_t>(dw)*dh*4);
            hr=converter->CopyPixels(nullptr,dw*4,static_cast<UINT>(image.size()),image.data());
        }
        drop(converter);drop(scaler);drop(frame);drop(decoder);drop(wic);
        if(FAILED(hr))return false;
        int w=std::max(1,static_cast<int>(std::ceil(logicalWidth*scale*.5f)));
        int h=std::max(1,static_cast<int>(std::ceil(logicalHeight*scale*.5f)));
        std::vector<std::uint8_t> pixels(static_cast<size_t>(w)*h*4);
        RECT reference=monitor;
        if(position==DWPOS_SPAN){
            reference={GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),
                GetSystemMetrics(SM_XVIRTUALSCREEN)+GetSystemMetrics(SM_CXVIRTUALSCREEN),
                GetSystemMetrics(SM_YVIRTUALSCREEN)+GetSystemMetrics(SM_CYVIRTUALSCREEN)};
        }
        double mw=reference.right-reference.left,mh=reference.bottom-reference.top;
        double sx=mw/dw,sy=mh/dh;
        if(position==DWPOS_FILL||position==DWPOS_SPAN)sx=sy=std::max(sx,sy);
        if(position==DWPOS_FIT)sx=sy=std::min(sx,sy);
        if(position==DWPOS_CENTER||position==DWPOS_TILE){sx=double(iw)/dw;sy=double(ih)/dh;}
        double ox=reference.left+(mw-dw*sx)/2,oy=reference.top+(mh-dh*sy)/2;
        double left=(monitor.left+monitor.right-logicalWidth*scale)/2;
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            double wx=left+(x+.5)*logicalWidth*scale/w;
            double wy=monitor.top+(y+.5)*logicalHeight*scale/h;
            int ix=static_cast<int>(std::floor((wx-ox)/sx)),iy=static_cast<int>(std::floor((wy-oy)/sy));
            if(position==DWPOS_TILE){
                ix=static_cast<int>(std::floor((wx-reference.left)/sx))%static_cast<int>(dw);
                iy=static_cast<int>(std::floor((wy-reference.top)/sy))%static_cast<int>(dh);
                if(ix<0)ix+=dw;
                if(iy<0)iy+=dh;
            }
            size_t p=(static_cast<size_t>(y)*w+x)*4;
            if(ix>=0&&iy>=0&&ix<static_cast<int>(dw)&&iy<static_cast<int>(dh)){
                size_t q=(static_cast<size_t>(iy)*dw+ix)*4;
                for(int c=0;c<3;c++)pixels[p+c]=image[q+c];
            }else{pixels[p]=GetBValue(background);pixels[p+1]=GetGValue(background);pixels[p+2]=GetRValue(background);}
            pixels[p+3]=255;
        }
        blur(pixels,w,h,std::max(2,static_cast<int>(8*scale)));
        auto properties=D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED));
        hr=target->CreateBitmap(D2D1::SizeU(w,h),pixels.data(),w*4,properties,&bitmap);
        ready=SUCCEEDED(hr);return ready;
    }
};
}
