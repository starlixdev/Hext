#pragma once
#include <cstdint>

enum class VisibilityChange {None, Show, Hide, Suppress};
// Desktop presence and hover expansion are separate states.
// The compact island remains visible on the desktop; foreground apps suppress both.
struct DesktopVisibility {
    bool compactVisible=false, expanded=false, awaitLeave=false;
    std::uint64_t candidateSince=0, lastInside=0;
    VisibilityChange update(std::uint64_t now,bool desktop,bool hotzone,bool inside,
                            bool paused) {
        bool wasVisible=compactVisible;
        compactVisible=desktop&&!paused;
        if(!compactVisible) {
            expanded=false;candidateSince=0;lastInside=0;
            return wasVisible?VisibilityChange::Suppress:VisibilityChange::None;
        }
        if(awaitLeave) {
            if(!hotzone&&!inside)awaitLeave=false;
            return VisibilityChange::None;
        }
        if(!expanded) {
            if(!hotzone){candidateSince=0;return VisibilityChange::None;}
            if(!candidateSince)candidateSince=now;
            if(now-candidateSince>=100) {
                expanded=true;lastInside=now;candidateSince=0;
                return VisibilityChange::Show;
            }
        } else {
            if(inside||hotzone)lastInside=now;
            else if(now-lastInside>=320) {
                expanded=false;candidateSince=0;return VisibilityChange::Hide;
            }
        }
        return VisibilityChange::None;
    }
    void dismiss() {expanded=false;candidateSince=0;awaitLeave=true;}
};
