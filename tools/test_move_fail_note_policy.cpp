#include "../ai/playerbot/MoveFailNotePolicy.h"

#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

// Blackfathom Deeps 2026-10-23: 172 "move to position - FAILED" lines in a row
// with no reason. The line now says why, the water state and the path type.
static void TestFailNoteFormat()
{
    CHECK(MoveFailNote("dispatch-short", true, false, true, 0x81) ==
        " why=dispatch-short water=1 under=0 swim=1 path=0x81");
    CHECK(MoveFailNote("empty-path", false, true, false, 0x11) ==
        " why=empty-path water=0 under=1 swim=0 path=0x11");
    CHECK(MoveFailNote(nullptr, false, false, false, 0) == " why=none water=0 under=0 swim=0 path=0x0");
}

static void TestPositionMoveFailing()
{
    CHECK(!PositionMoveFailing(0, 1000));
    CHECK(PositionMoveFailing(1000, 1000));
    CHECK(PositionMoveFailing(1000, 1000 + kPositionMoveFailFreshMs));
    CHECK(!PositionMoveFailing(1000, 1001 + kPositionMoveFailFreshMs));
    // The WorldTimer wraps.
    CHECK(PositionMoveFailing(0xFFFFFF00u, 0x00000100u));
}

// Opus review 2026-10-23: the note's reason belongs to this failure only: an action
// that failed before any move try must not show an older try's reason.
static void TestReasonOnlyForThisTry()
{
    CHECK(std::string(MoveFailNoteReason(4, 5, "dispatch-short")) == "dispatch-short");
    CHECK(std::string(MoveFailNoteReason(5, 5, "dispatch-short")) == "no-move-try");
    CHECK(std::string(MoveFailNoteReason(0xFFFFFFFFu, 0, "empty-path")) == "empty-path"); // the counter wraps
}

int main()
{
    TestFailNoteFormat();
    TestPositionMoveFailing();
    TestReasonOnlyForThisTry();
    std::cout << "move fail note policy: all tests passed\n";
    return 0;
}
