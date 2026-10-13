#include "../behavior/SummonRestorePolicy.h"

#include <cassert>
#include <iostream>

using namespace TortoiseBots;

int main()
{
    // Summon requested, nothing else ordered: follow is restored on arrival
    // (today's behaviour). A stay given BEFORE the request is not seen by the
    // policy at all, so the summon is the later order and also restores follow.
    {
        SummonRestorePolicy p;
        assert(p.ShouldRestoreFollow());
    }

    // A stationary order (stay, guard, hold, comestay) after the request and before arrival: restore cancelled.
    {
        SummonRestorePolicy p;
        p.NoteHoldOrder();
        assert(!p.ShouldRestoreFollow());
    }

    // Repeated orders keep it cancelled.
    {
        SummonRestorePolicy p;
        p.NoteHoldOrder();
        p.NoteHoldOrder();
        assert(!p.ShouldRestoreFollow());
    }

    // `come` after a hold is the latest order: follow is restored again; a
    // hold after the come cancels it once more. A come with no hold is a no-op.
    {
        SummonRestorePolicy p;
        p.NoteComeOrder();
        assert(p.ShouldRestoreFollow());
        p.NoteHoldOrder();
        assert(!p.ShouldRestoreFollow());
        p.NoteComeOrder();
        assert(p.ShouldRestoreFollow());
        p.NoteHoldOrder();
        p.NoteHoldOrder();
        p.NoteComeOrder();
        assert(p.ShouldRestoreFollow());
    }

    // A new summon request is a new policy object: a cancelled restore of an
    // earlier summon does not leak into it (the later summon is the later order).
    {
        SummonRestorePolicy first;
        first.NoteHoldOrder();
        SummonRestorePolicy second;
        assert(!first.ShouldRestoreFollow());
        assert(second.ShouldRestoreFollow());
    }

    std::cout << "summon restore policy tests passed\n";
    return 0;
}
