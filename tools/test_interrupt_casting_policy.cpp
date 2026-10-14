#include "../ai/playerbot/InterruptCastingPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::InterruptSpellInfo;
using ai::InterruptTargetSpell;
using ai::ShouldInterruptTargetCasting;

// A pummel/kick-like interrupt spell: interrupt-cast effect, silence prevention.
static InterruptSpellInfo Pummel()
{
    InterruptSpellInfo info;
    info.interruptFlags = 0x04; // SPELL_INTERRUPT_FLAG_INTERRUPT
    info.preventionType = 1;    // SILENCE
    info.effect[0] = ai::INTERRUPT_EFFECT_INTERRUPT_CAST;
    return info;
}

// A silence-prevented 2.5 s heal on its cast bar.
static InterruptTargetSpell BarCasting()
{
    InterruptTargetSpell s;
    static int marker = 1;
    s.spell = reinterpret_cast<Spell const*>(&marker);
    s.channeledSlot = false;
    s.state = ai::INTERRUPT_SPELL_PREPARING;
    s.castTime = 2500;
    s.preventionType = 1; // SILENCE
    s.interruptFlags = 0x02; // DAMAGE
    return s;
}

int main()
{
    std::cout << "Starting TortoiseBots interrupt-casting policy tests...\n";
    InterruptSpellInfo pummel = Pummel();

    // Live case (Patchwerk 2026-10-14): melee boss, no spell slots at all.
    CHECK(!ShouldInterruptTargetCasting(nullptr, nullptr, pummel));
    std::cout << "  [PASS] idle boss is not interruptible\n";

    // Real interruptible cast: heal on its bar.
    InterruptTargetSpell bar = BarCasting();
    CHECK(ShouldInterruptTargetCasting(&bar, nullptr, pummel));
    std::cout << "  [PASS] cast-bar heal is interruptible\n";

    // Instant with a missile in flight (DELAYED, e.g. Hateful Strike):
    // nothing on a bar to interrupt.
    InterruptTargetSpell delayed = BarCasting();
    delayed.state = 5; // SPELL_STATE_DELAYED
    CHECK(!ShouldInterruptTargetCasting(&delayed, nullptr, pummel));
    std::cout << "  [PASS] delayed instant is not interruptible\n";

    // Instant with no cast time sitting in PREPARING: same, no bar.
    InterruptTargetSpell instant = BarCasting();
    instant.castTime = 0;
    CHECK(!ShouldInterruptTargetCasting(&instant, nullptr, pummel));
    std::cout << "  [PASS] zero-cast-time preparing spell is not interruptible\n";

    // Finished slot leftovers are not casts.
    InterruptTargetSpell finished = BarCasting();
    finished.state = 3; // SPELL_STATE_FINISHED
    CHECK(!ShouldInterruptTargetCasting(&finished, nullptr, pummel));
    std::cout << "  [PASS] finished spell is not interruptible\n";

    // Channeled spell in flight is interruptible when channel-flagged.
    InterruptTargetSpell channel = BarCasting();
    channel.channeledSlot = true;
    channel.state = ai::INTERRUPT_SPELL_CASTING;
    channel.channelInterruptFlags = 0x04; // CHANNEL_FLAG_INTERRUPT
    CHECK(ShouldInterruptTargetCasting(nullptr, &channel, pummel));
    std::cout << "  [PASS] channeled cast is interruptible\n";

    // A non-silence-prevented cast (enrage, prevention type 0) cannot be
    // interrupted by pummel/kick.
    InterruptTargetSpell enrage = BarCasting();
    enrage.preventionType = 0;
    CHECK(!ShouldInterruptTargetCasting(&enrage, nullptr, pummel));
    std::cout << "  [PASS] non-silence cast is not interruptible\n";

    // Immunity to the interrupt effect refuses.
    InterruptSpellInfo immunePummel = Pummel();
    immunePummel.effectImmune[0] = true;
    CHECK(!ShouldInterruptTargetCasting(&bar, nullptr, immunePummel));
    std::cout << "  [PASS] immune target is not interruptible\n";

    std::cout << "All interrupt-casting policy tests passed.\n";
    return 0;
}
