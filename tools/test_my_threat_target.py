"""Compile the production threat sampler with target loss and reacquisition."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class MyThreatTargetTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_absent_and_changed_targets(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/strategy/values/ThreatValues.cpp").read_text()
        start = source.index("float MyThreatValue::Calculate()")
        method = source[start:source.index("float TankThreatValue::Calculate()", start)]
        harness = r"""
#include <cassert>
using ObjectGuid = unsigned;
struct Unit { unsigned guid; float threat; unsigned getObjectGuid() const { return guid; } };
Unit* currentTarget = nullptr;
#define AI_VALUE(type, qualifier) currentTarget
struct LogCalculatedValue { int resets = 0; void Reset() { ++resets; } };
struct ThreatValue {
    static float GetThreat(void*, Unit* target) { return target ? target->threat : 0; }
};
struct MyThreatValue : LogCalculatedValue {
    void* bot = nullptr;
    ObjectGuid lastTarget = 0;
    float Calculate();
};
"""
        harness += method + r"""
int main() {
    MyThreatValue value;
    assert(value.Calculate() == 0);
    assert(value.lastTarget == 0 && value.resets == 0);
    Unit first{1, 150}, second{2, 75};
    currentTarget = &first;
    assert(value.Calculate() == 150);
    assert(value.lastTarget == 1 && value.resets == 1);
    assert(value.Calculate() == 150 && value.resets == 1);
    currentTarget = nullptr;
    assert(value.Calculate() == 0);
    assert(value.lastTarget == 0 && value.resets == 2);
    assert(value.Calculate() == 0 && value.resets == 2);
    currentTarget = &second;
    assert(value.Calculate() == 75);
    assert(value.lastTarget == 2 && value.resets == 3);
}
"""
        with tempfile.TemporaryDirectory(prefix="my-threat-target-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
