"""Reject malformed talent tree separators before the production parser runs."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class TalentLinkValidationTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_tree_separator_limit(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = (root / "ai/playerbot/Talentspec.cpp").read_text()
        start = source.index("bool TalentSpec::CheckTalentLink(")
        validation = source[start:source.index("uint32 TalentSpec::LeveltoPoints", start)]
        start = source.index("void TalentSpec::ReadTalents(std::string link)")
        parsing = source[start:source.index("//Returns only a specific tree", start)]
        harness = r"""
#include <cassert>
#include <sstream>
#include <string>
#include <vector>
using std::stoi;
struct TalentSpec {
    struct Entry { int tab; int rank = 0; int tabPage() const { return tab; } };
    std::vector<Entry> talents{{0},{0},{1},{1},{2},{2}};
    unsigned points = 0;
    bool CheckTalentLink(std::string, std::ostringstream*);
    void ReadTalents(std::string);
};
"""
        harness += validation + parsing + r"""
int main() {
    for (const char* link : {"1---2", "1-2-3-4", "---1", "1---"}) {
        TalentSpec spec; std::ostringstream out;
        assert(!spec.CheckTalentLink(link, &out));
        assert(!out.str().empty());
    }
    for (const char* link : {"1", "1-2", "1-2-3", "--1", "-1", "0-0-0", "1--", "12-34-5"}) {
        TalentSpec spec; std::ostringstream out;
        assert(spec.CheckTalentLink(link, &out));
        spec.ReadTalents(link);
    }
    for (const char* link : {"", "--", "6", "abc"}) {
        TalentSpec spec; std::ostringstream out;
        assert(!spec.CheckTalentLink(link, &out));
    }
}
"""
        with tempfile.TemporaryDirectory(prefix="talent-link-") as folder:
            cpp = pathlib.Path(folder) / "test.cpp"
            exe = pathlib.Path(folder) / "test"
            cpp.write_text(harness)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
