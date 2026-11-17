#pragma once
#include <algorithm>
#include <cmath>
enum class MotionStyle {Smooth, Snappy, Bouncy};
// Closed-form damped oscillator. Target changes preserve position AND velocity,
// including underdamped motion, independently of the presentation frame rate.
struct Spring {
    double value = 0, velocity = 0, target = 0, frequency = 17, damping = 1;
    void step(double seconds) {
        seconds=std::max(0.0,seconds);
        double y=value-target,w=frequency,z=std::max(.05,damping),v=velocity;
        if(z<.9999){
            double a=z*w,b=w*std::sqrt(1-z*z),e=std::exp(-a*seconds);
            double c=std::cos(b*seconds),s=std::sin(b*seconds),j=(v+a*y)/b;
            value=target+e*(y*c+j*s);
            velocity=e*(v*c-(a*v+w*w*y)*s/b);
        }else if(z>1.0001){
            double root=w*std::sqrt(z*z-1),r1=-z*w+root,r2=-z*w-root;
            double c1=(v-r2*y)/(r1-r2),c2=y-c1;
            double e1=std::exp(r1*seconds),e2=std::exp(r2*seconds);
            value=target+c1*e1+c2*e2;velocity=r1*c1*e1+r2*c2*e2;
        }else{
            double j=v+w*y,decay=std::exp(-w*seconds);
            value=target+(y+j*seconds)*decay;velocity=(v-w*j*seconds)*decay;
        }
        if(std::abs(value-target)<.015 && std::abs(velocity)<.06) {value=target;velocity=0;}
    }
    bool moving() const {return value!=target || velocity!=0;}
    void snap(double v) {value=target=v;velocity=0;}
    void tune(MotionStyle style,bool exit=false){
        frequency=style==MotionStyle::Smooth?18:style==MotionStyle::Snappy?22:20;
        damping=style==MotionStyle::Smooth?1:style==MotionStyle::Snappy?.86:.72;
        if(exit){frequency*=1.05;damping=std::max(.9,damping);}
    }
};
inline float blend(float a,float b,float t){return a+(b-a)*t;}
inline float unit(double v){return static_cast<float>(std::clamp(v,0.0,1.0));}
inline float smoothRange(float a,float b,float value){
    float t=std::clamp((value-a)/(b-a),0.0f,1.0f);return t*t*(3-2*t);
}
