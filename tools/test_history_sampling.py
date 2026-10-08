"""Exercise production history sampling and retention with a controlled clock."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class HistorySamplingTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_record_changes_not_repeated_reads(self):
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
struct History : LogCalculatedValue<float> {
    History(): LogCalculatedValue<float>(nullptr) { minChangeInterval = 60; logLength = 30; }
    float current = 100;
    float Calculate() override { return current; }
    bool EqualToLast(float v) override { return v == lastValue; }
    void Seed(float v, time_t t) { valueLog.push_back({v,t}); }
};
int main() {
    History history;
    assert(history.Get() == 100);
    assert(history.ValueLog().size() == 1);
    assert(history.ValueLog().front().first == 100);
    for (now = 101; now <= 700; ++now)
        history.Get();
    assert(history.ValueLog().size() == 1); // repeated reads must not evict the old position
    assert(history.ValueLog().front().second == 100);
    history.current = 120;
    history.Get();
    assert(history.ValueLog().size() == 2);
    assert(history.ValueLog().back().first == 120);
    history.current = 140;
    ++now;
    history.Get();
    assert(history.ValueLog().size() == 2); // honor the minimum change interval
    now += 61;
    history.Get();
    assert(history.ValueLog().size() == 3);
    assert(history.ValueLog().back().first == 140);
    for (int i = 0; i < 40; ++i) {
        now += 61;
        history.current += 1;
        history.Get();
    }
    assert(history.ValueLog().size() == 30);
    assert(history.ValueLog().back().first == 180);
    history.Reset();
    assert(history.ValueLog().empty());
    history.Get();
    assert(history.ValueLog().size() == 1); // reseed after reset, even inside the change interval
    assert(history.ValueLog().front().first == 180);
    assert(history.ValueLog().front().second == now);
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
