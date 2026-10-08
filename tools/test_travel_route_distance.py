"""Production route length used when pruning redundant travel links."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class TravelRouteDistanceTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_every_edge_including_last(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/TravelNode.cpp").read_text()
        start = source.index("float TravelNodeRoute::getTotalDistance()")
        method = source[start:source.index("TravelPath TravelNodeRoute::buildPath", start)]
        harness = r'''
#include <cassert>
#include <cstddef>
#include <vector>
using uint32 = unsigned;
using std::size_t;
struct TravelNode {
    float outgoingDistance;
    float linkDistanceTo(TravelNode*) { return outgoingDistance; }
};
struct TravelNodeRoute {
    std::vector<TravelNode*> nodes;
    float getTotalDistance();
};
'''
        harness += method + r'''
int main() {
    TravelNode first{20}, middle{200}, last{0};
    TravelNodeRoute detour{{&first, &middle, &last}};
    assert(detour.getTotalDistance() == 220);
    // The pruning comparison must not treat this long detour as cheaper
    // than a 100-yard direct edge (10 yards to the detour's first node).
    assert(!(10 + detour.getTotalDistance() < 100 * 1.1f));
    TravelNodeRoute pair{{&first, &middle}};
    assert(pair.getTotalDistance() == 20);
    TravelNodeRoute single{{&first}};
    assert(single.getTotalDistance() == 0);
    TravelNodeRoute empty;
    assert(empty.getTotalDistance() == 0);
    first.outgoingDistance = 0;
    assert(pair.getTotalDistance() == 0);
}
'''
        with tempfile.TemporaryDirectory(prefix="route-distance-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-D_GLIBCXX_ASSERTIONS", str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
