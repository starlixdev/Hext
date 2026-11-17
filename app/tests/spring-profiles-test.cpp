#include "../src/motion.h"
#include <cassert>
#include <iostream>
int main(){
    for(auto style:{MotionStyle::Smooth,MotionStyle::Snappy,MotionStyle::Bouncy}){
        Spring a{0,0,100},b=a,c=a;a.tune(style);b.tune(style);c.tune(style);
        double maxValue=0;
        for(int i=0;i<144;i++){c.step(1.0/144);maxValue=std::max(maxValue,c.value);}
        for(int i=0;i<60;i++)a.step(1.0/60);
        for(int i=0;i<24;i++)b.step(1.0/24);
        assert(std::abs(a.value-b.value)<.03&&std::abs(b.value-c.value)<.03);
        if(style==MotionStyle::Smooth)assert(maxValue<=100);
        if(style==MotionStyle::Snappy)assert(maxValue>100&&maxValue<102);
        if(style==MotionStyle::Bouncy)assert(maxValue>102&&maxValue<106);
        Spring interrupt{184,0,736};interrupt.tune(style);
        for(int i=0;i<8;i++)interrupt.step(1.0/60);
        auto position=interrupt.value,velocity=interrupt.velocity;
        interrupt.target=184;interrupt.tune(style,true);
        assert(interrupt.value==position&&interrupt.velocity==velocity);
        for(int i=0;i<120;i++){
            interrupt.step(1.0/60);
            assert(std::isfinite(interrupt.value)&&interrupt.value>0);
        }
        assert(std::abs(interrupt.value-184)<.02);
    }
    Spring over{0,0,1,20,1.4};
    for(int i=0;i<120;i++){over.step(1.0/60);assert(over.value>=0&&over.value<=1);}
    assert(std::abs(over.value-1)<.02);
    std::cout<<"PASS: smooth/snappy/bouncy, bounded overshoot, 24/60/144 Hz, interrupt continuity and overdamping.\n";
}
