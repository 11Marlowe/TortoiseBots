#pragma once

// Issue #490 PR 1: startup cache for `tortoise_bots_hire_gear`. Loaded once
// (one pass over the rows, O(rows)); the hire path only does lookups.
// Rows arrive from the offline generator later; an empty table is a valid
// state and simply provisions nothing.

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "HireGearPolicy.h"

namespace TortoiseBots
{

class HireGearCache
{
public:
    static HireGearCache& Instance();

    // Loads the whole table once. Idempotent: clears and reloads.
    void Load();

    // Test seam: replace the rows without a DB.
    void SetRowsForTest(std::vector<HireGearRow> const& rows);

    // True when the table holds any row for this class/spec at this tier.
    bool HasSetFor(uint8_t classId, char const* spec, uint8_t tier) const;

    // Race-override lookup (see HireGearPolicy.h). False = no row.
    bool FindRow(uint8_t classId, char const* spec, uint8_t race,
        uint8_t tier, uint8_t slot, HireGearRow const*& out) const;

    size_t RowCount() const;

private:
    HireGearCache() = default;

    mutable std::mutex m_mutex;
    std::vector<HireGearRow> m_rows;
};

} // namespace TortoiseBots
