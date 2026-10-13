#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

// Index for ai_playerbot_named_location, used by RandomBotFacade::GetNamedLocation
// (runtime/PlayerbotRuntimeFacade.cpp). The table is read once at startup and
// lookups are served from this index, so they never touch the database.
//
// Pure data with no core types, so the lookup rules are unit-tested on their own
// (tools/test_named_location_index.cpp).
//
// Lifecycle: the single loading thread adds rows, then calls Seal() once. Find()
// answers nothing before the seal, and Add() refuses rows after it, so the map is
// never written while another thread can read it. The release/acquire pair on the
// seal flag makes every row visible to a reader that sees the seal.
//
// Names match the old query (WHERE name = ... on the table's utf8mb3 default
// collation) for the ASCII names the table holds: letters are case-insensitive and
// trailing spaces do not count.

namespace TortoiseBots
{

struct NamedLocationPoint
{
    uint32_t mapId = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float orientation = 0.0f;
};

// Key form of a name: ASCII upper case, trailing spaces removed.
inline std::string NamedLocationKey(std::string const& name)
{
    std::string key(name);
    for (char& c : key)
    {
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
    }

    while (!key.empty() && key.back() == ' ')
        key.pop_back();

    return key;
}

class NamedLocationIndex
{
public:
    // Loading only. False after Seal(), or when the key is already present: the
    // first row wins, as it would under the table's primary key.
    bool Add(std::string const& name, NamedLocationPoint const& point)
    {
        if (sealed_.load(std::memory_order_acquire))
            return false;

        return points_.emplace(NamedLocationKey(name), point).second;
    }

    void Seal()
    {
        sealed_.store(true, std::memory_order_release);
    }

    bool Sealed() const
    {
        return sealed_.load(std::memory_order_acquire);
    }

    std::size_t Size() const
    {
        return points_.size();
    }

    // False before the seal, so a lookup never sees a half-built index.
    bool Find(std::string const& name, NamedLocationPoint& point) const
    {
        if (!sealed_.load(std::memory_order_acquire))
            return false;

        auto it = points_.find(NamedLocationKey(name));
        if (it == points_.end())
            return false;

        point = it->second;
        return true;
    }

private:
    std::unordered_map<std::string, NamedLocationPoint> points_;
    std::atomic<bool> sealed_{false};
};

} // namespace TortoiseBots
