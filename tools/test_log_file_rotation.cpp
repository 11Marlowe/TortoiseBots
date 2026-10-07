#include "../ai/playerbot/LogFileRotation.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
static void Write(fs::path const& p, char const* text) { std::ofstream(p) << text; }
static std::string Read(fs::path const& p)
{
    std::ifstream in(p);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv)
{
    assert(argc == 2);
    auto root = fs::path(argv[1]) / ("bot-log-rotation-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    auto p = root / "bot_events.csv";
    std::error_code error;
    ai::LogFileRotation first;
    assert(first.Prepare(p.string(), "w", error) && !error);
    assert(!fs::exists(p.string() + ".1"));
    Write(p, "run one\n");
    ai::LogFileRotation second;
    assert(second.Prepare(p.string(), "w", error) && !error);
    assert(Read(p.string() + ".1") == "run one\n");
    Write(p, "run two\n");
    assert(second.Prepare(p.string(), "w", error));
    Write(p, "refreshed run two\n");
    assert(Read(p.string() + ".1") == "run one\n");
    ai::LogFileRotation third;
    assert(third.Prepare(p.string(), "w", error));
    assert(Read(p.string() + ".1") == "refreshed run two\n");
    Write(p, "run three\n");
    ai::LogFileRotation append;
    assert(append.Prepare(p.string(), "a", error));
    assert(Read(p) == "run three\n");
    assert(Read(p.string() + ".1") == "refreshed run two\n");
    // A nonempty destination directory makes rename fail, even when run as root.
    auto blocked = root / "blocked.csv";
    Write(blocked, "must survive\n");
    fs::create_directory(blocked.string() + ".1");
    Write(fs::path(blocked.string() + ".1") / "previous", "also survives\n");
    ai::LogFileRotation failed;
    assert(!failed.Prepare(blocked.string(), "w", error) && error);
    assert(Read(blocked) == "must survive\n");
    assert(Read(fs::path(blocked.string() + ".1") / "previous") == "also survives\n");
    assert(!failed.Prepare(blocked.string(), "w", error) && error);
    assert(failed.Prepare(blocked.string(), "a", error) && !error);
    fs::remove_all(root);
}
