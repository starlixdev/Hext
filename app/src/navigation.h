#pragma once
#include <array>
#include <algorithm>
struct NavigationOrder {
    std::array<int,7> items{0,3,6,1,2,4,5};
    static float row(int index){return 94+44.0f*index;}
    int position(int page) const {
        auto item=std::find(items.begin(),items.end(),page);
        return item==items.end()?0:static_cast<int>(item-items.begin());
    }
    void restore(const std::array<int,7>& saved){
        auto defaults=items;size_t count=0;
        auto add=[&](int item){
            if(item<0||item>6||std::find(items.begin(),items.begin()+count,item)!=items.begin()+count)return;
            items[count++]=item;
        };
        for(int item:saved)add(item);
        for(int item:defaults)if(count<items.size())add(item);
    }
    void move(int page,int target){
        int from=position(page);target=std::clamp(target,0,6);
        if(from<target)std::rotate(items.begin()+from,items.begin()+from+1,items.begin()+target+1);
        else if(from>target)std::rotate(items.begin()+target,items.begin()+from,items.begin()+from+1);
    }
};
