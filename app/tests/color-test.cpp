#include "../src/color.h"
#include <cassert>
#include <iostream>
int main(){
    hext::AccentPalette palette;auto seeds=palette.recent;
    palette.select(0xE23489);assert(palette.current()==0xE23489&&palette.recent[1]==seeds[0]);
    palette.select(seeds[1]);assert(palette.current()==seeds[1]&&palette.recent[1]==0xE23489);
    palette.select(seeds[1]);assert(palette.recent[1]==0xE23489);
    auto saved=palette.recent;hext::AccentPalette restored;restored.restore(saved,palette.current());
    assert(restored.recent==saved);
    restored.restore({0xAB1234,0xAB1234,0xAB1234,0xFFFFFFFF},0xAB1234);
    for(size_t i=0;i<4;i++)for(size_t j=i+1;j<4;j++)assert(restored.recent[i]!=restored.recent[j]);
    for(std::uint32_t c=0;c<0xFFFFFF;c+=7919){
        auto converted=hext::rgb(hext::hsv(c));
        for(int shift:{0,8,16})assert(std::abs(int((c>>shift)&255)-int((converted>>shift)&255))<=1);
        assert(hext::contrast(c,hext::foreground(c))>=4.5);
    }
    assert(hext::rgb({0,1,1})==0xFF0000);
    assert(hext::rgb({120,1,1})==0x00FF00);
    assert(hext::rgb({0,0,1})==0xFFFFFF&&hext::rgb({0,1,0})==0);
    assert(hext::hex(0x123ABF)==L"#123ABF");
    std::cout<<"PASS: full-gamut HSV, contrast, recent colors, deduplication and palette restore.\n";
}
