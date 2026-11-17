#include "../src/presentation.h"
#include <cassert>
#include <iostream>
int main(){
    DesktopEntry entry;
    assert(!entry.update(0,true));
    assert(!entry.update(80,true));
    assert(!entry.update(100,false));
    assert(!entry.update(120,true));
    assert(!entry.update(200,true));
    assert(entry.update(280,true));
    assert(entry.update(900,true));
    assert(!entry.update(901,false));
    assert(!entry.update(950,true));
    assert(!entry.update(1080,true));
    assert(entry.update(1110,true));
    using A=PresentationAction;
    assert(reconcilePresentation({true,false,false,false,false,false})==A::RestoreCompact);
    // Explorer hides an island that our application still considers shown.
    assert(reconcilePresentation({true,false,true,false,false,false})==A::RestoreCompact);
    // Show Desktop minimizes the native window; WS_VISIBLE can remain set.
    assert(reconcilePresentation({true,false,true,true,true,false})==A::RestoreCompact);
    // Desktop was raised over a visible, restored island.
    assert(reconcilePresentation({true,false,true,true,false,true})==A::Restack);
    assert(reconcilePresentation({true,false,true,true,false,false})==A::None);
    assert(reconcilePresentation({true,false,true,true,false,false,true})==A::RestoreCompact);
    assert(reconcilePresentation({false,false,true,true,false,false,true})==A::Hide);
    assert(reconcilePresentation({true,true,true,true,false,false,true})==A::Hide);
    assert(reconcilePresentation({false,false,true,true,true,true})==A::Hide);
    assert(reconcilePresentation({true,true,true,true,false,true})==A::Hide);
    assert(reconcilePresentation({false,false,false,false,false,true})==A::None);
    for(int bits=0;bits<128;bits++){
        PresentationState s{bool(bits&1),bool(bits&2),bool(bits&4),
                            bool(bits&8),bool(bits&16),bool(bits&32),bool(bits&64)};
        auto a=reconcilePresentation(s);
        if(!s.desktop||s.paused)assert(a!=A::RestoreCompact&&a!=A::Restack);
        if(s.desktop&&!s.paused&&(!s.logicalShown||!s.nativeVisible||s.minimized||s.fadingOut))
            assert(a==A::RestoreCompact);
    }
    std::cout<<"PASS: Show Desktop recovery (hidden, minimized, behind desktop and interrupted fade), app suppression and pause.\n";
}
