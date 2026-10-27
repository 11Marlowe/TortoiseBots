// Whispers to an advertising bot LFG leader are taken on the packet hook
// (any thread) and handed to BotLfgService on the world thread; the bot's
// AI never sees them. Exercises the whisper parse, the leader set, the cap
// and two producer threads against a draining world thread.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra -pthread tools/test_bot_lfg_whisper_inbox.cpp -o /tmp/lfg34_inbox
//   /tmp/lfg34_inbox

#include "../runtime/BotChatGate.h"
#include "../runtime/BotLfgWhisperInbox.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

constexpr uint8_t kWhisper = 6, kChannel = 14;
constexpr uint32_t kAddon = 0xFFFFFFFF, kUniversal = 0;

static std::vector<uint8_t> Packet(uint8_t type, uint32_t lang, uint64_t sender, std::string const& text)
{
    std::vector<uint8_t> p;
    p.push_back(type);
    for (int i = 0; i < 4; ++i) p.push_back(uint8_t(lang >> (8 * i)));
    for (int i = 0; i < 8; ++i) p.push_back(uint8_t(sender >> (8 * i)));
    uint32_t len = uint32_t(text.size() + 1);
    for (int i = 0; i < 4; ++i) p.push_back(uint8_t(len >> (8 * i)));
    p.insert(p.end(), text.begin(), text.end());
    p.push_back(0);
    p.push_back(0); // chat tag
    return p;
}

static void TestParse()
{
    std::vector<uint8_t> p = Packet(kWhisper, kUniversal, 0x0000000000001234ULL, "inv pls");
    WhisperChatLine line;
    CHECK(ParseWhisperChatLine(p.data(), p.size(), line));
    CHECK(line.language == kUniversal && line.sender == 0x1234 && std::string(line.text, line.textLength) == "inv pls");
    CHECK(!ParseWhisperChatLine(p.data(), 16, line));
    CHECK(!ParseWhisperChatLine(nullptr, 0, line));
}

static void TestOffer()
{
    BotLfgWhisperInbox inbox;
    std::vector<uint8_t> p = Packet(kWhisper, kUniversal, 77, "inv");
    CHECK(!inbox.Offer(42, p.data(), p.size(), kWhisper, kAddon));        // nobody published
    inbox.PublishLeaders({42});
    CHECK(inbox.IsLeader(42) && !inbox.IsLeader(43));
    CHECK(!inbox.Offer(43, p.data(), p.size(), kWhisper, kAddon));        // not a leader: the AI gets it
    CHECK(inbox.Offer(42, p.data(), p.size(), kWhisper, kAddon));
    std::vector<LfgWhisper> got = inbox.Drain();
    CHECK(got.size() == 1 && got[0].leader == 42 && got[0].sender == 77 && got[0].text == "inv");
    CHECK(inbox.Drain().empty());
    std::vector<uint8_t> addon = Packet(kWhisper, kAddon, 77, "x");
    CHECK(!inbox.Offer(42, addon.data(), addon.size(), kWhisper, kAddon));
    std::vector<uint8_t> chan = Packet(kChannel, kUniversal, 77, "inv");
    CHECK(!inbox.Offer(42, chan.data(), chan.size(), kWhisper, kAddon));
    std::vector<uint8_t> self = Packet(kWhisper, kUniversal, 42, "inv");
    CHECK(!inbox.Offer(42, self.data(), self.size(), kWhisper, kAddon));
    std::vector<uint8_t> longText = Packet(kWhisper, kUniversal, 77, std::string(400, 'a'));
    CHECK(inbox.Offer(42, longText.data(), longText.size(), kWhisper, kAddon));
    CHECK(inbox.Drain()[0].text.size() == BotLfgWhisperInbox::kMaxTextBytes);
    inbox.PublishLeaders({});
    CHECK(!inbox.Offer(42, p.data(), p.size(), kWhisper, kAddon));
}

static void TestCap()
{
    BotLfgWhisperInbox inbox;
    inbox.PublishLeaders({42});
    std::vector<uint8_t> p = Packet(kWhisper, kUniversal, 77, "inv");
    for (size_t i = 0; i < BotLfgWhisperInbox::kMaxQueued + 1; ++i)
        CHECK(inbox.Offer(42, p.data(), p.size(), kWhisper, kAddon));   // over the cap still consumed
    CHECK(inbox.Drain().size() == BotLfgWhisperInbox::kMaxQueued);
    CHECK(inbox.TakeDropped() == 1 && inbox.TakeDropped() == 0);
}

static void TestThreads()
{
    BotLfgWhisperInbox inbox;
    inbox.PublishLeaders({42});
    std::vector<uint8_t> p = Packet(kWhisper, kUniversal, 77, "inv");
    std::atomic<bool> go{false};
    auto produce = [&]() { while (!go) {} for (int i = 0; i < 1000; ++i) inbox.Offer(42, p.data(), p.size(), kWhisper, kAddon); };
    std::thread a(produce), b(produce);
    go = true;
    size_t drained = 0;
    for (int i = 0; i < 2000; ++i) drained += inbox.Drain().size();
    a.join(); b.join();
    drained += inbox.Drain().size();
    CHECK(drained + inbox.TakeDropped() == 2000);
}

int main()
{
    TestParse();
    TestOffer();
    TestCap();
    TestThreads();
    std::printf("bot lfg whisper inbox: %d checks passed\n", checks);
    return 0;
}
