"""Exercise the actual world-update hook with stand-in service counters (#508).

Compile only the production OnUpdate definition, avoiding a live server or DB.
The fixture checks the disabled path and the enabled service order; it does not
reimplement the hook's control flow. Requires g++ (skipped when unavailable).
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class DisabledWorldUpdateTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_disabled_hook_is_inert_and_enabled_hook_still_ticks_services(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "host/BotHostAdapter.cpp").read_text()
        start = source.index("void BotHostAdapter::OnUpdate(uint32 diff)")
        end = source.index("void BotHostAdapter::OnShutdown()", start)
        hook = source[start:end]
        fixture = r'''
#include <cassert>
#include <cstdint>
#include <vector>
using uint32 = uint32_t;
struct { bool enabled = false; } sPlayerbotAIConfig;
namespace TortoiseBots {
std::vector<unsigned> calls;
template<unsigned Id> struct Service {
    static Service& Instance() { static Service service; return service; }
    void Update(uint32 diff) { assert(diff == 50); calls.push_back(Id); }
    void OnWorldUpdate(uint32 diff) { Update(diff); }
};
using BotActivityLeaseManager = Service<1>;
using BotManager = Service<2>;
using PlayerConvenience = Service<3>;
using RandomBotService = Service<4>;
using HireProvisionService = Service<5>;
using HireLifecycle = Service<6>;
using AhMarketService = Service<7>;
using BattlegroundQueueService = Service<8>;
using ObservabilityEmitter = Service<9>;
struct BotHostAdapter { uint32 m_ticks = 0; void OnUpdate(uint32 diff); };
'''
        fixture += hook + r'''
}
int main() {
    using namespace TortoiseBots;
    BotHostAdapter adapter;
    // More than two 60 s recovery / four 30 s reporting windows.
    for (unsigned i = 0; i < 2500; ++i) adapter.OnUpdate(50);
    assert(calls.empty());
    assert(adapter.m_ticks == 0);
    sPlayerbotAIConfig.enabled = true;
    adapter.OnUpdate(50);
    assert((calls == std::vector<unsigned>{1,2,3,4,5,6,7,8,9}));
    assert(adapter.m_ticks == 1);
    sPlayerbotAIConfig.enabled = false;
    adapter.OnUpdate(50);
    assert(calls.size() == 9);
    assert(adapter.m_ticks == 1);
}
'''
        with tempfile.TemporaryDirectory(prefix="disabled-world-update-") as folder:
            source_path = pathlib.Path(folder) / "test.cpp"
            binary_path = pathlib.Path(folder) / "test"
            source_path.write_text(fixture)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            str(source_path), "-o", str(binary_path)], check=True)
            subprocess.run([str(binary_path)], check=True)


if __name__ == "__main__":
    unittest.main()
