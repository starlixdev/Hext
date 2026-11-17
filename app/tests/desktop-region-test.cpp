#include "../src/desktop-region.h"
#include <cassert>
#include <iostream>
int main(){
    using namespace hext;Region island{868,0,1052,42};
    assert(!coversIsland(island,{139,130,628,712},true,false,false));
    assert(coversIsland(island,{0,0,1920,1080},true,false,false));
    assert(coversIsland(island,{1000,10,1200,100},true,false,false));
    assert(!coversIsland(island,{1052,0,1200,300},true,false,false));
    assert(!coversIsland(island,{0,42,1920,600},true,false,false));
    assert(!coversIsland(island,{0,0,1920,1080},false,false,false));
    assert(!coversIsland(island,{0,0,1920,1080},true,true,false));
    assert(!coversIsland(island,{0,0,1920,1080},true,false,true));
    assert(!coversIsland(island,{-1920,0,0,1080},true,false,false));
    assert(coversIsland({-1052,0,-868,42},{-1920,0,0,1080},true,false,false));
    std::cout<<"PASS: small windows preserve exposed island; overlap, fullscreen, minimized, cloaked, touching edges and multiple monitors\n";
}
