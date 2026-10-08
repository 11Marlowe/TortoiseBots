"""Exercise production talent shifting with a controlled point allowance."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class TalentShiftBudgetTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_shift_starts_from_current_allocation(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/Talentspec.cpp").read_text()
        start = source.index("std::vector<TalentSpec::TalentListEntry> TalentSpec::SubTalentList(")
        subtract = source[start:source.index("bool TalentSpec::isEarlierVersionOf", start)]
        start = source.index("void TalentSpec::ShiftTalents(")
        shift = source[start:]
        harness = r"""
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>
using uint32 = unsigned;
constexpr int SORT_BY_POINTS_TREE = 1;
constexpr int SUBSTRACT_OLD_NEW = 1, ABSOLUTE_DIST = 0;
constexpr int ADDED_POINTS = 2, REMOVED_POINTS = -2;
struct Player { uint32 allowance; };
uint32 GetTotalTalentPoints_TB(Player* bot) { return bot->allowance; }
struct TalentSpec {
    struct TalentListEntry { int entry; int rank; };
    std::vector<TalentListEntry> talents;
    uint32 points;
    bool cropped = false;
    void SortTalents(int) {} // Single tree, already in priority order.
    void CropTalents(Player*) { cropped = true; }
    std::vector<TalentListEntry> SubTalentList(std::vector<TalentListEntry>&,
        std::vector<TalentListEntry>&, int);
    void ShiftTalents(TalentSpec*, Player*);
};
"""
        harness += subtract + shift + r"""
int main() {
    Player bot{5};
    // A rank-2 talent is one known rank spell. Do not rely on the cached
    // current-spec point count: ReadTalents(Player*) counts known spells.
    TalentSpec current{{{1, 2}, {2, 0}}, 1};
    TalentSpec requested{{{1, 4}, {2, 0}}, 4};
    requested.ShiftTalents(&current, &bot);
    assert(requested.talents[0].rank == 4);
    assert(requested.points == 4);
    assert(current.talents[0].rank == 2);
    assert(!requested.cropped);

    // Identical allocations do not change; an empty allocation can grow.
    TalentSpec identical{{{1, 2}, {2, 0}}, 2};
    identical.ShiftTalents(&current, &bot);
    assert(identical.points == 2 && identical.talents[0].rank == 2);
    TalentSpec empty{{{1, 0}, {2, 0}}, 0};
    TalentSpec growth{{{1, 2}, {2, 1}}, 3};
    growth.ShiftTalents(&empty, &bot);
    assert(growth.points == 3 && growth.talents[1].rank == 1);

    // Existing reset/crop branches stay in control for a full requested
    // build, or when the request removes an already allocated point.
    TalentSpec full{{{1, 5}, {2, 0}}, 5};
    full.ShiftTalents(&current, &bot);
    assert(full.cropped);
    TalentSpec removal{{{1, 1}, {2, 0}}, 1};
    removal.ShiftTalents(&current, &bot);
    assert(removal.cropped);
}
"""
        with tempfile.TemporaryDirectory(prefix="talent-shift-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
