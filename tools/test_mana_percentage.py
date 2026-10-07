"""Production mana percentage calculation, including non-mana units."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class ManaPercentageTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_zero_maximum_and_normal_percentages(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/strategy/values/StatsValues.cpp").read_text()
        start = source.index("uint8 ManaValue::Calculate()")
        calculation = source[start:source.index("bool HasManaValue::Calculate()", start)]
        harness = r"""
#include <cassert>
#include <cstdint>
using uint8 = uint8_t;
using uint32 = uint32_t;
constexpr int POWER_MANA = 0;
struct Unit {
    uint32 current, maximum;
    uint32 GetPower(int) const { return current; }
    uint32 GetMaxPower(int) const { return maximum; }
};
struct ManaValue {
    Unit* target;
    Unit* GetTarget() const { return target; }
    uint8 Calculate();
};
"""
        harness += calculation + r"""
int main() {
    Unit nonMana{0, 0};
    ManaValue value{&nonMana};
    assert(value.Calculate() == 0);
    // A mana-free target must not pass the Viper Sting >=10% gate.
    assert(!(value.Calculate() >= 10));
    Unit caster{0, 1000}; value.target = &caster;
    assert(value.Calculate() == 0);
    caster.current = 500;
    assert(value.Calculate() == 50);
    caster.current = 1000;
    assert(value.Calculate() == 100);
    caster.current = 99;
    assert(value.Calculate() == 9);
    value.target = nullptr;
    assert(value.Calculate() == 100); // Existing absent-target convention.
}
"""
        with tempfile.TemporaryDirectory(prefix="mana-percentage-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra",
                            "-fsanitize=float-cast-overflow", "-fsanitize-undefined-trap-on-error",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
