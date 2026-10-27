#pragma once
// Which of a human's party bots the LFT service accepts an offer for (#1284).
// A random-pool bot is a fill candidate, not the human's companion, unless it is
// hired: a hire is a pool character (record.random = true,
// HireProvisionService) that belongs to its hirer for the length of the hire.
namespace TortoiseBots
{
inline bool LftPartyBotMayAccept(bool randomPool, bool hired)
{
    return !randomPool || hired;
}
} // namespace TortoiseBots
