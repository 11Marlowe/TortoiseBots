"""Exercise the production saved-outfit parser across the byte boundary."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class OutfitItemParserTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_long_saved_outfit(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/PlayerbotAI.cpp").read_text()
        start = source.index("ItemIds PlayerbotAI::InventoryParseOutfitItems(std::string text)")
        method = source[start:source.index("void PlayerbotAI::Ping(", start)]
        harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <string>
using uint8 = uint8_t;
using uint32 = uint32_t;
using ItemIds = std::set<uint32>;
struct PlayerbotAI { ItemIds InventoryParseOutfitItems(std::string); };
'''
        harness += method + r'''
int main() {
    PlayerbotAI ai;
    assert(ai.InventoryParseOutfitItems("raid=10001,10002") == (ItemIds{10001,10002}));
    assert(ai.InventoryParseOutfitItems("").empty());
    assert(ai.InventoryParseOutfitItems("raid=").empty());
    assert(ai.InventoryParseOutfitItems("10001,10002") == (ItemIds{10001,10002}));
    assert(ai.InventoryParseOutfitItems("raid=0,10001,10001,") == (ItemIds{10001}));
    // Save() serializes all items accumulated through repeated outfit additions.
    std::string saved = "raid=";
    ItemIds expected;
    for (uint32 id = 10000; id < 10050; ++id) {
        if (!expected.empty()) saved += ",";
        saved += std::to_string(id);
        expected.insert(id);
    }
    assert(saved.size() > 255);
    assert(ai.InventoryParseOutfitItems(saved) == expected);
    assert(ai.InventoryParseOutfitItems(std::string(256, 'a') + "=10001") == (ItemIds{10001}));
}
'''
        with tempfile.TemporaryDirectory(prefix="outfit-parser-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=2)


if __name__ == "__main__":
    unittest.main()
