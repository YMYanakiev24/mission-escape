#include "core/Progress.h"

#include <algorithm>

namespace core {

namespace {

constexpr int PRISM_UNLOCK_LEVEL = 5;
constexpr int COMET_UNLOCK_LEVEL = 10;
constexpr int PHANTOM_UNLOCK_LEVEL = 15;
constexpr int NOVA_UNLOCK_LEVEL = 20;
constexpr std::uint32_t ALL_SKINS_MASK = (1U << SKIN_COUNT) - 1U;
constexpr std::uint32_t ALL_TIERS_MASK = (1U << TIER_COUNT) - 1U;

std::uint32_t skinBit(SkinId skin) {
    return 1U << static_cast<unsigned>(skin);
}

std::uint32_t tierBit(Tier tier) {
    return 1U << static_cast<unsigned>(tier);
}

std::size_t recordIndex(int level) {
    return static_cast<std::size_t>(level - 1);
}

} // namespace

std::string_view skinName(SkinId skin) {
    switch (skin) {
        case SkinId::Wisp:
            return "Wisp";
        case SkinId::Prism:
            return "Prism";
        case SkinId::Comet:
            return "Comet";
        case SkinId::Phantom:
            return "Phantom";
        case SkinId::Nova:
            return "Nova";
    }
    return "Wisp";
}

std::string_view skinKey(SkinId skin) {
    switch (skin) {
        case SkinId::Wisp:
            return "wisp";
        case SkinId::Prism:
            return "prism";
        case SkinId::Comet:
            return "comet";
        case SkinId::Phantom:
            return "phantom";
        case SkinId::Nova:
            return "nova";
    }
    return "wisp";
}

std::optional<SkinId> parseSkinKey(std::string_view key) {
    for (const SkinId skin : ALL_SKINS) {
        if (skinKey(skin) == key) {
            return skin;
        }
    }
    return std::nullopt;
}

int skinUnlockLevel(SkinId skin) {
    switch (skin) {
        case SkinId::Wisp:
            return 0;
        case SkinId::Prism:
            return PRISM_UNLOCK_LEVEL;
        case SkinId::Comet:
            return COMET_UNLOCK_LEVEL;
        case SkinId::Phantom:
            return PHANTOM_UNLOCK_LEVEL;
        case SkinId::Nova:
            return NOVA_UNLOCK_LEVEL;
    }
    return 0;
}

std::optional<SkinId> skinUnlockedByLevel(int level) {
    for (const SkinId skin : ALL_SKINS) {
        if (skinUnlockLevel(skin) == level && level > 0) {
            return skin;
        }
    }
    return std::nullopt;
}

Progress::Progress() = default;

bool Progress::isLevelUnlocked(int level) const {
    return isValidLevel(level) && level <= m_highestUnlocked;
}

const LevelRecord& Progress::record(int level) const {
    static const LevelRecord EMPTY_RECORD{};
    return isValidLevel(level) ? m_records[recordIndex(level)] : EMPTY_RECORD;
}

int Progress::totalStars() const {
    int total = 0;
    for (const LevelRecord& entry : m_records) {
        total += entry.bestStars;
    }
    return total;
}

int Progress::completedLevelCount() const {
    return static_cast<int>(
        std::count_if(m_records.begin(), m_records.end(),
                      [](const LevelRecord& entry) { return entry.completed; }));
}

bool Progress::isSkinUnlocked(SkinId skin) const {
    return (m_skinMask & skinBit(skin)) != 0U;
}

bool Progress::selectSkin(SkinId skin) {
    if (!isSkinUnlocked(skin)) {
        return false;
    }
    m_selectedSkin = skin;
    return true;
}

bool Progress::hasSeenTierIntro(Tier tier) const {
    return (m_tierIntroMask & tierBit(tier)) != 0U;
}

void Progress::markTierIntroSeen(Tier tier) {
    m_tierIntroMask |= tierBit(tier);
}

ProgressUpdate Progress::applyResult(int level, int stars, int timeMs) {
    ProgressUpdate update;
    if (!isValidLevel(level) || stars <= QUIT_STARS) {
        return update; // quitting never touches records or unlocks
    }
    LevelRecord& entry = m_records[recordIndex(level)];
    const bool firstCompletion = !entry.completed;
    update.newRecord = updateRecord(entry, std::min(stars, MAX_STARS), timeMs);
    if (level < LEVEL_COUNT && m_highestUnlocked <= level) {
        m_highestUnlocked = level + 1;
        update.levelUnlocked = true;
    }
    const std::optional<SkinId> skin = skinUnlockedByLevel(level);
    if (skin && !isSkinUnlocked(*skin)) {
        m_skinMask |= skinBit(*skin);
        update.skinUnlocked = skin;
    }
    update.campaignCompleted = firstCompletion && level == LEVEL_COUNT;
    return update;
}

bool Progress::restoreHighestUnlocked(int level) {
    if (!isValidLevel(level)) {
        return false;
    }
    m_highestUnlocked = level;
    return true;
}

bool Progress::restoreRecord(int level, const LevelRecord& record) {
    const bool validStars = record.bestStars >= 0 && record.bestStars <= MAX_STARS;
    const bool validTime = record.bestTimeMs >= 0 && record.bestTimeMs <= MAX_RECORD_MILLISECONDS;
    const bool consistent = record.completed == (record.bestStars > 0);
    if (!isValidLevel(level) || !validStars || !validTime || !consistent) {
        return false;
    }
    m_records[recordIndex(level)] = record;
    return true;
}

void Progress::restoreSkinMask(std::uint32_t mask) {
    m_skinMask = (mask & ALL_SKINS_MASK) | skinBit(SkinId::Wisp);
    if (!isSkinUnlocked(m_selectedSkin)) {
        m_selectedSkin = SkinId::Wisp;
    }
}

void Progress::restoreTierIntroMask(std::uint32_t mask) {
    m_tierIntroMask = mask & ALL_TIERS_MASK;
}

} // namespace core
