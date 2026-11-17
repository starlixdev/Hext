#pragma once
namespace hext {
struct Region {long left,top,right,bottom;};
inline bool overlaps(Region a,Region b){
    return a.left<b.right&&b.left<a.right&&a.top<b.bottom&&b.top<a.bottom;
}
inline bool coversIsland(Region island,Region window,bool visible,bool minimized,bool cloaked){
    return visible&&!minimized&&!cloaked&&overlaps(island,window);
}
}
