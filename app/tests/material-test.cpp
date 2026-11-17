#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../src/material.h"
#include <cassert>
#include <iostream>
int main(){
    std::vector<std::uint8_t> constant(12*9*4);
    for(size_t i=0;i<constant.size();i+=4){constant[i]=70;constant[i+1]=110;constant[i+2]=190;constant[i+3]=255;}
    auto expected=constant;
    hext::WallpaperMaterial::blur(constant,12,9,2);
    assert(constant==expected);
    std::vector<std::uint8_t> impulse(17*17*4);
    for(size_t i=3;i<impulse.size();i+=4)impulse[i]=255;
    for(int c=0;c<3;c++)impulse[(8*17+8)*4+c]=255;
    hext::WallpaperMaterial::blur(impulse,17,17,1);
    auto center=impulse[(8*17+8)*4],neighbor=impulse[(8*17+7)*4];
    assert(center>0&&center<255&&neighbor>0);
    assert(impulse[(8*17+7)*4]==impulse[(8*17+9)*4]);
    for(size_t i=3;i<impulse.size();i+=4)assert(impulse[i]==255);
    std::cout<<"PASS: cached blur preserves flat colors, symmetry and opacity; spreads image detail.\n";
}
