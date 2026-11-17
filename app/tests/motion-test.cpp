#include "../src/motion.h"
#include <cassert>
#include <iostream>
int main(){
    Spring fast{184,0,736},slow=fast;
    for(int i=0;i<60;i++)fast.step(1.0/60);
    for(int i=0;i<30;i++)slow.step(1.0/30);
    assert(std::abs(fast.value-slow.value)<.05);
    assert(std::abs(fast.value-736)<.05);
    Spring interrupt{184,0,736};
    for(int i=0;i<7;i++)interrupt.step(1.0/60);
    double position=interrupt.value,velocity=interrupt.velocity;
    interrupt.target=184;
    assert(interrupt.value==position&&interrupt.velocity==velocity);
    for(int i=0;i<180;i++){
        interrupt.step(1.0/60);assert(std::isfinite(interrupt.value)&&interrupt.value>0);
    }
    assert(std::abs(interrupt.value-184)<.02);
    Spring rapid{184,0,736};
    for(int i=0;i<240;i++){
        if(i%4==0)rapid.target=rapid.target==184?736:184;
        rapid.step(1.0/144);assert(std::isfinite(rapid.value)&&rapid.value>100&&rapid.value<850);
    }
    std::cout<<"PASS: frame-rate independence, interrupted transitions, rapid retargeting.\n";
}
