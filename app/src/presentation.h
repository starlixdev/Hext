#pragma once
#include <cstdint>

// Explorer changes focus and stacking repeatedly during Show Desktop.
// Expose pixels only after desktop focus settles; suppress input immediately.
struct DesktopEntry {
    bool eligible=false, pending=false;
    std::uint64_t since=0;
    bool update(std::uint64_t now,bool desktop) {
        if(!desktop){eligible=pending=false;return false;}
        if(!pending){pending=true;since=now;}
        if(now-since>=160)eligible=true;
        return eligible;
    }
};

enum class PresentationAction {None, Hide, RestoreCompact, Restack};
struct PresentationState {
    bool desktop, paused, logicalShown, nativeVisible, minimized, desktopAbove;
    bool fadingOut=false;
};
// Explorer may hide/minimize/reorder a window without changing our logical state.
inline PresentationAction reconcilePresentation(PresentationState s) {
    if(!s.desktop||s.paused)
        return s.logicalShown||s.nativeVisible?PresentationAction::Hide:PresentationAction::None;
    if(!s.logicalShown||!s.nativeVisible||s.minimized||s.fadingOut)return PresentationAction::RestoreCompact;
    if(s.desktopAbove)return PresentationAction::Restack;
    return PresentationAction::None;
}
