#pragma once

// Pure policy for the native AH read model and personal-settlement appraisal
// (issues #87, #1180): the bounded snapshot upsert rule in
// RandomBotFacade::LoadAuctionPrices, the faction visibility filter in
// RandomBotFacade::GetAhPrices, and the per-item appraisal math in
// ItemUsageValue (median, lowest listing, desired sell price). These rules
// used to exist twice - once in production, once as mock copies inside
// tools/test_auction_read_model.cpp that stayed green when production
// changed. They live here std-only so the test compiles the real rules;
// callers translate game state (AuctionEntry lookups, config, randomness)
// into plain inputs.
// No core includes.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai
{
    // Bounded per-item listings: keep at most this many lowest-unit-price
    // entries per item template so memory and appraisal sorting stay bounded.
    inline constexpr std::size_t kMaxAuctionsPerItem = 64;

    // Returned by AhListingReplaceIndex when a full snapshot holds nothing
    // dearer than the candidate (the caller then drops the candidate).
    inline constexpr std::size_t kAhNoListingIndex = static_cast<std::size_t>(-1);

    // Snapshot upsert when the per-item listing set is full: replace the
    // most expensive stored listing, but only if the candidate is strictly
    // cheaper per item. unitPriceOf maps a stored listing to its per-item
    // price; an empty set always pushes (caller checks the size bound).
    template <typename Listings, typename UnitPriceFn>
    inline std::size_t AhListingReplaceIndex(const Listings& listings, float currentUnitPrice, UnitPriceFn unitPriceOf)
    {
        std::size_t maxIdx = 0;
        float maxUnitPrice = 0.0f;
        for (std::size_t idx = 0; idx < listings.size(); ++idx)
        {
            float existingUnitPrice = unitPriceOf(listings[idx]);
            if (existingUnitPrice > maxUnitPrice)
            {
                maxUnitPrice = existingUnitPrice;
                maxIdx = idx;
            }
        }
        return currentUnitPrice < maxUnitPrice ? maxIdx : kAhNoListingIndex;
    }

    // Faction scoping: a listing is visible to the viewer's house when
    // cross-faction trade is on, either side sits on the neutral AH
    // (team 0), or the houses match.
    inline bool AhListingVisibleForFaction(bool twoSidedAuction, uint32_t listingTeam, uint32_t viewerFaction)
    {
        return twoSidedAuction || listingTeam == 0 || listingTeam == viewerFaction || viewerFaction == 0;
    }

    // Median per-item buyout across listings. unitPriceOf returns the
    // per-item price, or 0 for a listing that cannot be appraised (item
    // gone or zero count - the snapshot never stores zero-buyout listings,
    // so 0 unambiguously means "skip"). Sub-copper medians return the
    // sentinel 1, never 0: callers distinguish "no listing" (0) from
    // "cheaper than one copper" (1).
    template <typename Listings, typename UnitPriceFn>
    inline uint32_t AhMedianBuyoutPricePerItem(const Listings& listings, UnitPriceFn unitPriceOf)
    {
        std::vector<float> prices;
        for (auto const& auction : listings)
        {
            float unitPrice = unitPriceOf(auction);
            if (unitPrice > 0.0f)
                prices.push_back(unitPrice);
        }

        if (prices.empty())
            return 0;

        std::size_t n = prices.size() / 2;
        std::nth_element(prices.begin(), prices.begin() + n, prices.end());
        float median = prices[n];
        if (median > 0 && median < 1)
            return 1;
        return static_cast<uint32_t>(median);
    }

    // Lowest per-item buyout across listings, same 0 / sentinel-1 contract
    // as the median (a found sub-copper listing still returns 1, so resale
    // paths can tell it apart from "no listing").
    template <typename Listings, typename UnitPriceFn>
    inline uint32_t AhLowestBuyoutPricePerItem(const Listings& listings, UnitPriceFn unitPriceOf)
    {
        float minPrice = 0;
        bool found = false;

        for (auto const& auction : listings)
        {
            float unitPrice = unitPriceOf(auction);
            if (unitPrice > 0.0f)
            {
                if (!found || unitPrice < minPrice)
                {
                    minPrice = unitPrice;
                    found = true;
                }
            }
        }

        if (!found)
            return 0;
        if (minPrice > 0 && minPrice < 1)
            return 1;
        return (uint32_t)minPrice;
    }

    // Desired sell price per item, shared tail of
    // ItemUsageValue::DesiredPricePerItem: lerp between the configured AH
    // price band, undercut the lowest competing listing (undercut() is only
    // invoked when there is one, so the RNG stream is untouched otherwise),
    // and never land below the band's floor.
    template <typename UndercutFn>
    inline uint32_t AhDesiredPricePerItem(uint32_t minAhPrice, uint32_t maxAhPrice,
        uint32_t lowestBuyoutItemPricePerItem, bool lowestOwnedByOther, uint32_t priceModifier, UndercutFn undercut)
    {
        uint32_t desiredPricePerItem = minAhPrice + static_cast<uint32_t>((maxAhPrice - minAhPrice) * priceModifier / 100);

        if (lowestBuyoutItemPricePerItem > 0 && lowestOwnedByOther)
        {
            uint32_t undercutByMoney = std::max(static_cast<uint32_t>(1),
                static_cast<uint32_t>(lowestBuyoutItemPricePerItem * undercut()));

            if (undercutByMoney < lowestBuyoutItemPricePerItem)
                desiredPricePerItem = lowestBuyoutItemPricePerItem - undercutByMoney;
            else
                desiredPricePerItem = lowestBuyoutItemPricePerItem - 1;
        }

        desiredPricePerItem = std::max(minAhPrice, desiredPricePerItem);

        return desiredPricePerItem;
    }
} // namespace ai
