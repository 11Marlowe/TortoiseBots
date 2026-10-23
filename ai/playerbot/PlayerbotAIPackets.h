#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>

class PacketHandlingHelper;

namespace ai
{
struct PacketHandlerSpec
{
    uint16_t opcode;
    char const* handler;
    bool delay;
};

using OpcodeSet = std::bitset<0x10000>;

extern PacketHandlerSpec const kMasterIncomingPacketHandlers[];
extern size_t const kMasterIncomingPacketHandlerCount;
extern PacketHandlerSpec const kBotOutgoingPacketHandlers[];
extern size_t const kBotOutgoingPacketHandlerCount;
extern PacketHandlerSpec const kMasterOutgoingPacketHandlers[];
extern size_t const kMasterOutgoingPacketHandlerCount;

OpcodeSet BuildOpcodeSet(PacketHandlerSpec const* specs, size_t count, std::initializer_list<uint16_t> extra);
void RegisterPacketHandlers(PacketHandlingHelper& helper, PacketHandlerSpec const* specs, size_t count);

void StripGmBadgeColor(std::string& text);
}
