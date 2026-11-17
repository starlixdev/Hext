#include "../src/visibility.h"
#include <cassert>
#include <iostream>
int main(){
    using C=VisibilityChange;
    DesktopVisibility p;
    assert(p.update(100,true,false,false,false)==C::None&&p.compactVisible&&!p.expanded);
    assert(p.update(200,true,true,false,false)==C::None);
    assert(p.update(300,true,true,false,false)==C::Show&&p.expanded);
    assert(p.update(400,true,false,true,false)==C::None&&p.expanded);
    assert(p.update(600,true,false,false,false)==C::None&&p.expanded);
    assert(p.update(721,true,false,false,false)==C::Hide&&!p.expanded&&p.compactVisible);
    // Hover must never reveal over an ordinary foreground app.
    assert(p.update(900,false,true,true,false)==C::Suppress&&!p.compactVisible&&!p.expanded);
    p.update(1000,true,true,false,false);
    assert(p.update(1100,true,true,false,false)==C::Show);
    assert(p.update(1101,false,true,true,false)==C::Suppress&&!p.compactVisible&&!p.expanded);
    // Returning to desktop restores only the compact island.
    assert(p.update(1200,true,false,false,false)==C::None&&p.compactVisible&&!p.expanded);
    assert(p.update(1301,true,true,true,true)==C::Suppress&&!p.compactVisible);
    // Escape while hovering must not immediately reopen the island.
    p.dismiss();
    assert(p.update(1500,true,true,true,false)==C::None&&p.compactVisible&&!p.expanded);
    p.update(1600,true,false,false,false);
    p.update(1700,true,true,false,false);
    assert(p.update(1800,true,true,false,false)==C::Show&&p.expanded);
    DesktopVisibility idle;
    for(std::uint64_t t=100;t<3600000;t+=10000){
        idle.update(t,true,false,false,false);
        assert(idle.compactVisible&&!idle.expanded);
    }
    DesktopVisibility mixed;
    for(std::uint64_t i=1;i<10000;i++){
        bool desktop=i%11<7,paused=i%37==0;
        mixed.update(i*80,desktop,i%5<2,i%7<3,paused);
        assert(mixed.compactVisible==(desktop&&!paused));
        assert(!mixed.expanded||mixed.compactVisible);
    }
    std::cout<<"PASS: persistent compact desktop island, hover expansion, leave collapse, app suppression, pause and Escape.\n";
}
