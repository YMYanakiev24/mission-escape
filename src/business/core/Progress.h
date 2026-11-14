#pragma once

#include "core/Config.h"
#include "core/LevelRules.h"
#include "core/Scoring.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

/// @file Progress.h
/// Campaign progress of one save slot: unlocked levels, records, skins and seen tier intros.

namespace core {

/// Player skins; each one is unlocked by completing a milestone level.
enum class SkinId : std::uint8_t { Wisp, Prism, Comet, Phantom, Nova };

inline constexpr int SKIN_COUNT = 5;

/// All skins in display order.
inline constexpr std::array<SkinId, SKIN_COUNT> ALL_SKINS = {
    SkinId::Wisp, SkinId::Prism, SkinId::Comet, SkinId::Phantom, SkinId::Nova};

/// Display name ("Wisp", "Prism", ...).
std::string_view skinName(SkinId skin);

/// Lower-case key used in save files ("wisp", "prism", ...).
std::string_view skinKey(SkinId skin);

/// Parses a key produced by skinKey().
std::optional<SkinId> parseSkinKey(std::string_view key);

/// Level whose completion unlocks @p skin (0 for the default skin).
int skinUnlockLevel(SkinId skin);

/// The skin unlocked by completing @p level, if any.
std::optional<SkinId> skinUnlockedByLevel(int level);

/// What changed when a level result was applied.
struct ProgressUpdate {
    bool newRecord = false;             ///< Better stars or time than before.
    bool levelUnlocked = false;         ///< The next level became playable.
    std::optional<SkinId> skinUnlocked; ///< A skin unlocked for the first time.
    bool campaignCompleted = false;     ///< The final level was completed for the first time.
};

/// Progress through the 20-level campaign for one save slot.
class Progress {
public:
    /// Fresh progress: level 1 unlocked, default skin only.
    Progress();

    /// Highest level the player may start (1..LEVEL_COUNT).
    int highestUnlockedLevel() const noexcept { return m_highestUnlocked; }

    /// True if @p level can be played.
    bool isLevelUnlocked(int level) const;

    /// Record of @p level (levels outside the range return an empty record).
    const LevelRecord& record(int level) const;

    /// Sum of the best stars over all levels.
    int totalStars() const;

    /// Number of levels completed with at least one star.
    int completedLevelCount() const;

    /// True when @p skin may be selected.
    bool isSkinUnlocked(SkinId skin) const;

    /// The skin currently worn.
    SkinId selectedSkin() const noexcept { return m_selectedSkin; }

    /// Wears @p skin if it is unlocked; returns false otherwise.
    bool selectSkin(SkinId skin);

    /// True once the intro card of @p tier has been shown to this player.
    bool hasSeenTierIntro(Tier tier) const;

    /// Remembers that the intro card of @p tier was shown.
    void markTierIntroSeen(Tier tier);

    /// Applies a finished attempt. A quit (0 stars) never changes anything.
    ProgressUpdate applyResult(int level, int stars, int timeMs);

    // --- Restoration from save files (each call validates its input) ---

    /// Sets the highest unlocked level; returns false if out of range.
    bool restoreHighestUnlocked(int level);

    /// Restores one level record; returns false if any value is invalid.
    bool restoreRecord(int level, const LevelRecord& record);

    /// Restores the unlocked-skin bit mask; the default skin is always kept.
    void restoreSkinMask(std::uint32_t mask);

    /// Restores the seen-tier-intro bit mask.
    void restoreTierIntroMask(std::uint32_t mask);

    std::uint32_t skinMask() const noexcept { return m_skinMask; }
    std::uint32_t tierIntroMask() const noexcept { return m_tierIntroMask; }

private:
    int m_highestUnlocked = 1;
    std::array<LevelRecord, LEVEL_COUNT> m_records{};
    std::uint32_t m_skinMask = 1U; ///< Bit i set = skin i unlocked (Wisp always).
    std::uint32_t m_tierIntroMask = 0U;
    SkinId m_selectedSkin = SkinId::Wisp;
};

} // namespace core
