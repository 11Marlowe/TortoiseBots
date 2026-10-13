// Issue #846: the packet hook skips copying a chat line the receiving bot's AI
// would drop at once (runtime/BotChatGate.h). Exercises the packet parse and
// the Trade-channel decision standalone.
#include "../runtime/BotChatGate.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace TortoiseBots;

namespace {

constexpr uint8_t kChatMsgChannel = 0x0E;

// SMSG_MESSAGECHAT for CHAT_MSG_CHANNEL, laid out as ChatHandler::BuildChatPacket
// writes it: type, language, channel, player rank, sender, text length, text, tag.
std::vector<uint8_t> ChannelPacket(std::string const& channel, uint64_t sender, std::string const& text)
{
    std::vector<uint8_t> p;
    auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) p.push_back(uint8_t(v >> (8 * i))); };
    auto u64 = [&](uint64_t v) { for (int i = 0; i < 8; ++i) p.push_back(uint8_t(v >> (8 * i))); };
    auto str = [&](std::string const& s) { p.insert(p.end(), s.begin(), s.end()); p.push_back(0); };
    p.push_back(kChatMsgChannel);
    u32(0);
    str(channel);
    u32(0);
    u64(sender);
    u32(uint32_t(text.size() + 1));
    str(text);
    p.push_back(0);
    return p;
}

bool IsTrade(char const* name, size_t length)
{
    return std::string(name, length).compare(0, 8, "Trade - ") == 0;
}

bool Ignores(BotChatGate const& gate, uint64_t bot, std::vector<uint8_t> const& packet)
{
    ChannelChatLine line;
    assert(ParseChannelChatLine(packet.data(), packet.size(), line));
    return BotIgnoresChannelLine(gate, bot, line, IsTrade);
}

} // namespace

int main()
{
    // The parse reads channel, sender and text where the AI reads them.
    {
        std::vector<uint8_t> packet = ChannelPacket("Trade - City", 0x1122334455667788ull, "WTS [Linen Cloth]");
        ChannelChatLine line;
        assert(ParseChannelChatLine(packet.data(), packet.size(), line));
        assert(std::string(line.channel, line.channelLength) == "Trade - City");
        assert(line.sender == 0x1122334455667788ull);
        assert(std::string(line.text, line.textLength) == "WTS [Linen Cloth]");
    }

    // Truncated bodies are not parsed (the line is then kept).
    {
        std::vector<uint8_t> packet = ChannelPacket("Trade - City", 5, "hi");
        ChannelChatLine line;
        for (size_t size : { size_t(0), size_t(5), size_t(10), size_t(1 + 4 + 13 + 4 + 8) })
            assert(!ParseChannelChatLine(packet.data(), size, line));
        assert(!ParseChannelChatLine(nullptr, 40, line));
    }

    BotChatGate pool;
    pool.ignoreUnaddressedTrade = true;
    pool.name = "Bimbo";
    uint64_t const bot = 42;

    // A masterless pool bot (#469): Trade lines that do not name it are not
    // copied; a line naming it, its own line and other channels are.
    assert(Ignores(pool, bot, ChannelPacket("Trade - City", 7, "WTS [Linen Cloth]")));
    assert(!Ignores(pool, bot, ChannelPacket("Trade - City", 7, "Bimbo, want to trade?")));
    assert(!Ignores(pool, bot, ChannelPacket("Trade - City", bot, "WTS stuff")));
    assert(!Ignores(pool, bot, ChannelPacket("General - Elwynn Forest", 7, "anyone around?")));
    assert(!Ignores(pool, bot, ChannelPacket("World", 7, "lfg deadmines")));

    // The name test is a case-sensitive substring, as the AI's find().
    assert(Ignores(pool, bot, ChannelPacket("Trade - City", 7, "bimbo?")));

    // A bot with a master, a hired or recording bot (no Trade gate), or one
    // without a published name, gets every line.
    {
        BotChatGate owned;
        owned.name = "Bimbo";
        assert(!Ignores(owned, bot, ChannelPacket("Trade - City", 7, "WTS [Linen Cloth]")));
        BotChatGate unnamed;
        unnamed.ignoreUnaddressedTrade = true;
        assert(!Ignores(unnamed, bot, ChannelPacket("Trade - City", 7, "WTS [Linen Cloth]")));
    }

    // The Trade lookup is only asked when the gate could apply.
    {
        int asked = 0;
        auto counting = [&](char const* name, size_t length) { ++asked; return IsTrade(name, length); };
        std::vector<uint8_t> packet = ChannelPacket("Trade - City", 7, "Bimbo!");
        ChannelChatLine line;
        assert(ParseChannelChatLine(packet.data(), packet.size(), line));
        assert(!BotIgnoresChannelLine(pool, bot, line, counting));
        BotChatGate owned;
        assert(!BotIgnoresChannelLine(owned, bot, line, counting));
        assert(asked == 0);
        std::vector<uint8_t> unaddressed = ChannelPacket("Trade - City", 7, "WTS");
        assert(ParseChannelChatLine(unaddressed.data(), unaddressed.size(), line));
        assert(BotIgnoresChannelLine(pool, bot, line, counting));
        assert(asked == 1);
    }

    return 0;
}
