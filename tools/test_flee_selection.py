"""Exercise production flee selection with failed-heading and fallback fixtures (#486)."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class FleeSelectionTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_successful_heading_repeats_failed_heading_defers_and_fallback_survives(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        text = (root / "ai/playerbot/FleeManager.cpp").read_text()
        start = text.index("bool FleeManager::isBetterThan(")
        end = text.index("bool FleeManager::CalculateDestination(", start)
        methods = text[start:end]
        fixture = r'''
#include "ai/playerbot/CombatSpreadPolicy.h"
#include <cassert>
#include <cmath>
#include <list>
using uint32 = unsigned;
using namespace ai;
struct WorldPosition {
    uint32 map; float x, y, z;
    WorldPosition(uint32 map = 0, float x = 0, float y = 0, float z = 0)
        : map(map), x(x), y(y), z(z) {}
    uint32 GetMapId() const { return map; }
    float GetAngleTo(WorldPosition const& other) const { return std::atan2(other.y-y, other.x-x); }
};
struct FleePoint { float x, y, z, sumDistance; };
struct FleeManager {
    WorldPosition startPosition{0,0,0,0};
    bool isBetterThan(FleePoint* point, FleePoint* other);
    FleePoint* selectOptimalDestination(std::list<FleePoint*>& points,
        FleeFailureMemory const* failures, uint32 nowMs);
};
'''
        fixture += methods + r'''
int main() {
    FleeManager manager;
    FleePoint best{10,0,0,100}, next{0,10,0,90};
    std::list<FleePoint*> points{&best,&next};
    FleeFailureMemory memory;
    assert(manager.selectOptimalDestination(points, nullptr, 1000) == &best);
    memory.BeginAttempt(1,0,0,5,1000);
    assert(manager.selectOptimalDestination(points, &memory, 1000) == &best);
    memory.Observe(1,0,7,2000);
    assert(manager.selectOptimalDestination(points, &memory, 2000) == &best);
    memory.BeginAttempt(1,0,0,5,3000);
    memory.Observe(1,0,5,6000);
    assert(manager.selectOptimalDestination(points, &memory, 6000) == &next);
    memory.BeginAttempt(1,0,1.5707963268f,5,6100);
    memory.Observe(1,0,5,9100);
    // Both directions failed: retain the original best escape as a fallback.
    assert(manager.selectOptimalDestination(points, &memory, 9100) == &best);
    assert(manager.selectOptimalDestination(points, &memory, 14100) == &best);
    points.clear();
    assert(manager.selectOptimalDestination(points, &memory, 14100) == nullptr);
}
'''
        with tempfile.TemporaryDirectory(prefix="flee-selection-") as folder:
            source = pathlib.Path(folder) / "test.cpp"
            binary = pathlib.Path(folder) / "test"
            source.write_text(fixture)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-I", str(root), str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
