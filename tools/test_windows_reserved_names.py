"""Windows headers #define near and far (empty), so a C++ name `near` or `far`
breaks the Windows build while Linux builds it fine. Code only: comments and
string literals may use the words."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
DIRS = ["ai", "commands", "host", "runtime", "behavior"]
NAME = re.compile(r"\b(near|far)\b")
STRING = re.compile(r'"(?:[^"\\]|\\.)*"')


class WindowsReservedNamesTest(unittest.TestCase):
    def test_no_near_or_far_names(self):
        hits = []
        for folder in DIRS:
            for path in sorted((ROOT / folder).rglob("*")):
                if path.suffix not in (".h", ".cpp"):
                    continue
                for number, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
                    code = STRING.sub('""', line.split("//")[0])
                    if NAME.search(code):
                        hits.append(f"{path.relative_to(ROOT)}:{number}: {line.strip()}")
        self.assertEqual(hits, [], "near/far are Windows macros; rename:\n" + "\n".join(hits))


if __name__ == "__main__":
    unittest.main()
