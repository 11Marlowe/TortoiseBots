"""Exercise production value templates with an initially empty threat history."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class ThreatHistoryTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_cold_and_reset_history(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/strategy/Value.h").read_text()
        start = source.index("    template<class T>\n    class Value")
        templates = source[start:source.index("    class Uint8CalculatedValue", start)]
        harness = r"""
#include <algorithm>
#include <cassert>
#include <ctime>
#include <list>
#include <string>
#include <type_traits>
using uint8 = unsigned char;
using uint32 = unsigned;
struct PlayerbotAI {};
struct UntypedValue {
    UntypedValue(PlayerbotAI*, std::string) {}
    virtual ~UntypedValue() = default;
    virtual void Reset() {};
    virtual bool Expired() { return false; }
    virtual bool Expired(uint32) { return false; }
    virtual bool Protected() { return false; }
    virtual uint32 LastChangeDelay() { return 0; }
};
time_t now = 100;
time_t fake_time(void*) { return now; }
#define time fake_time
"""
        harness += templates + r"""
struct Threat : LogCalculatedValue<float> {
    Threat(): LogCalculatedValue<float>(nullptr) {}
    float current = 100;
    float Calculate() override { return current; }
    bool EqualToLast(float v) override { return v == lastValue; }
    void Seed(float v, time_t t) { valueLog.push_back({v,t}); }
};
int main() {
    Threat threat;
    assert(threat.GetDelta(5) == 0); // trigger can query history before the first Get
    threat.Seed(100, 90);
    threat.current = 150;
    assert(threat.GetDelta(5) == 5); // preserve nonempty-history slope
    threat.Reset();
    assert(threat.GetDelta(5) == 0);
    threat.Seed(150, now);
    assert(threat.GetDelta(5) == 0); // no elapsed time
}
"""
        with tempfile.TemporaryDirectory(prefix="threat-history-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-overloaded-virtual",
                            "-D_GLIBCXX_DEBUG", str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
