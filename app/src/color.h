#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
namespace hext {
struct HSV {double h=0,s=0,v=1;};
inline std::uint32_t rgb(HSV c){
    c.h=std::fmod(c.h+3600,360.0);c.s=std::clamp(c.s,0.0,1.0);c.v=std::clamp(c.v,0.0,1.0);
    double h=c.h/60,x=c.v*c.s*(1-std::abs(std::fmod(h,2.0)-1)),a=c.v*c.s,m=c.v-a;
    double r=0,g=0,b=0;
    if(h<1){r=a;g=x;}else if(h<2){r=x;g=a;}else if(h<3){g=a;b=x;}
    else if(h<4){g=x;b=a;}else if(h<5){r=x;b=a;}else{r=a;b=x;}
    auto channel=[&](double v){return static_cast<std::uint32_t>(std::round((v+m)*255));};
    return channel(r)<<16|channel(g)<<8|channel(b);
}
inline HSV hsv(std::uint32_t color){
    double r=((color>>16)&255)/255.0,g=((color>>8)&255)/255.0,b=(color&255)/255.0;
    double hi=std::max({r,g,b}),lo=std::min({r,g,b}),d=hi-lo,h=0;
    if(d){if(hi==r)h=60*std::fmod((g-b)/d,6.0);else if(hi==g)h=60*((b-r)/d+2);else h=60*((r-g)/d+4);}
    return {h<0?h+360:h,hi?d/hi:0,hi};
}
inline double luminance(std::uint32_t c){
    auto linear=[](double x){x/=255;return x<=.04045?x/12.92:std::pow((x+.055)/1.055,2.4);};
    return .2126*linear((c>>16)&255)+.7152*linear((c>>8)&255)+.0722*linear(c&255);
}
inline double contrast(std::uint32_t a,std::uint32_t b){
    double x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);
}
inline std::uint32_t foreground(std::uint32_t c){
    return contrast(c,0xFFFFFF)>=contrast(c,0)?0xFFFFFF:0;
}
inline std::wstring hex(std::uint32_t color){
    const wchar_t* digits=L"0123456789ABCDEF";std::wstring s=L"#000000";
    for(int i=0;i<6;i++)s[6-i]=digits[(color>>(4*i))&15];
    return s;
}
struct AccentPalette {
    std::array<std::uint32_t,4> recent{0xBED79E,0x246ED0,0xA98ACB,0xDEB89F};
    std::uint32_t current() const{return recent.front();}
    void select(std::uint32_t color){
        auto old=recent;recent[0]=color&0xFFFFFF;size_t at=1;
        for(auto c:old)if(c!=recent[0]&&at<recent.size())recent[at++]=c;
    }
    void restore(const std::array<std::uint32_t,4>& colors,std::uint32_t current){
        auto defaults=recent;size_t at=0;
        auto add=[&](std::uint32_t c){
            if(c>0xFFFFFF||at==recent.size()||std::find(recent.begin(),recent.begin()+at,c)!=recent.begin()+at)return;
            recent[at++]=c;
        };
        add(current);for(auto c:colors)add(c);for(auto c:defaults)add(c);
    }
};
}
