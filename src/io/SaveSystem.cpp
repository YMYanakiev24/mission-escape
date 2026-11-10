#include "io/SaveSystem.h"

#include "core/Config.h"
#include "core/Validation.h"
#include "io/KeyValueDocument.h"

#include <chrono>
#include <ctime>
#include <limits>
#include <system_error>

namespace io {

namespace {

constexpr int MAX_TIMESTAMP_LENGTH = 32;
constexpr int MAX_COLLECTED_PERKS = 4096;
constexpr int MILLISECONDS_PER_SECOND = 1000;
constexpr long long MAX_SEED = std::numeric_limits<std::uint32_t>::max();
constexpr int YEAR_OFFSET = 1900;
constexpr int TWO_DIGITS = 10;
constexpr int BITS_PER_HEX_DIGIT = 4;
/// Generous upper bound for the step counter of a saved attempt.
constexpr long long MAX_SNAPSHOT_STEPS = 10'000'000;

/// Signals the first invalid field while parsing; the message becomes the corruption reason.
struct ParseFailure {
    std::string message;
};

std::string levelKey(int level) {
    return "level." + std::to_string(level);
}

std::string effectKeyName(core::EffectType type) {
    return "snapshot.effect." + std::string(core::effectKey(type));
}

std::string twoDigits(int value) {
    return (value < TWO_DIGITS ? "0" : "") + std::to_string(value);
}

std::string joinInts(const std::vector<int>& values) {
    std::string text;
    for (const int value : values) {
        text += (text.empty() ? "" : ",") + std::to_string(value);
    }
    return text;
}

// ----------------------------------------------------------------------------------------
// Writing
// ----------------------------------------------------------------------------------------

void writeProfile(KeyValueDocument& document, const core::SaveData& data) {
    document.setInt("version", core::SAVE_FORMAT_VERSION);
    document.set("name", data.playerName);
    document.setInt("size", data.mazeSize);
    document.set("mode", std::string(core::viewModeKey(data.viewMode)));
    document.set("lastPlayed", data.lastPlayed);
}

void writeProgress(KeyValueDocument& document, const core::Progress& progress) {
    document.setInt("highestUnlocked", progress.highestUnlockedLevel());
    std::string skins;
    for (const core::SkinId skin : core::ALL_SKINS) {
        if (progress.isSkinUnlocked(skin)) {
            skins += (skins.empty() ? "" : ",") + std::string(core::skinKey(skin));
        }
    }
    document.set("skins", skins);
    document.set("selectedSkin", std::string(core::skinKey(progress.selectedSkin())));
    document.setInt("tierIntros", progress.tierIntroMask());
    for (int level = 1; level <= core::LEVEL_COUNT; ++level) {
        const core::LevelRecord& record = progress.record(level);
        document.set(levelKey(level),
                     std::to_string(record.bestStars) + "," + std::to_string(record.bestTimeMs));
    }
}

void writeSnapshot(KeyValueDocument& document, const core::LevelSnapshot& snapshot) {
    document.setBool("snapshot", true);
    document.setInt("snapshot.level", snapshot.level);
    document.setInt("snapshot.seed", snapshot.seed);
    document.setInt("snapshot.playerX", snapshot.playerCell.x);
    document.setInt("snapshot.playerY", snapshot.playerCell.y);
    document.set("snapshot.facing", std::string(core::directionName(snapshot.facing)));
    document.setInt("snapshot.elapsedMs", snapshot.elapsedMs);
    document.setBool("snapshot.timerStarted", snapshot.timerStarted);
    document.setInt("snapshot.berries", snapshot.berries);
    for (const core::EffectType type : core::ALL_EFFECT_TYPES) {
        document.setInt(effectKeyName(type), snapshot.effectMs[static_cast<std::size_t>(type)]);
    }
    document.set("snapshot.collected", joinInts(snapshot.collectedPerkIds));
    document.set("snapshot.explored", snapshot.exploredHex);
    document.setBool("snapshot.chaserActive", snapshot.chaserActive);
    document.setInt("snapshot.chaserX", snapshot.chaserCell.x);
    document.setInt("snapshot.chaserY", snapshot.chaserCell.y);
    document.setInt("snapshot.chaserActiveMs", snapshot.chaserActiveMs);
    document.setInt("snapshot.graceMs", snapshot.graceMs);
    document.setInt("snapshot.steps", snapshot.steps);
}

// ----------------------------------------------------------------------------------------
// Reading (every helper throws ParseFailure on the first invalid field)
// ----------------------------------------------------------------------------------------

long long requireInt(const KeyValueDocument& document, const std::string& key, long long minimum,
                     long long maximum) {
    const std::optional<long long> value = document.getInt(key, minimum, maximum);
    if (!value) {
        throw ParseFailure{"missing or invalid '" + key + "'"};
    }
    return *value;
}

bool requireBool(const KeyValueDocument& document, const std::string& key) {
    const std::optional<bool> value = document.getBool(key);
    if (!value) {
        throw ParseFailure{"missing or invalid '" + key + "'"};
    }
    return *value;
}

std::string requireText(const KeyValueDocument& document, const std::string& key) {
    std::optional<std::string> value = document.get(key);
    if (!value) {
        throw ParseFailure{"missing '" + key + "'"};
    }
    return *value;
}

void readProfile(const KeyValueDocument& document, core::SaveData& data) {
    requireInt(document, "version", core::SAVE_FORMAT_VERSION, core::SAVE_FORMAT_VERSION);
    const std::string name = requireText(document, "name");
    const core::NameValidation nameCheck = core::validatePlayerName(name);
    if (!nameCheck.valid || nameCheck.name != name) {
        throw ParseFailure{"invalid player name"};
    }
    data.playerName = name;
    data.mazeSize =
        static_cast<int>(requireInt(document, "size", core::MIN_MAZE_SIZE, core::MAX_MAZE_SIZE));
    const std::string mode = requireText(document, "mode");
    if (mode != core::viewModeKey(core::ViewMode::Classic2D) &&
        mode != core::viewModeKey(core::ViewMode::FirstPerson3D)) {
        throw ParseFailure{"invalid view mode"};
    }
    data.viewMode = mode == core::viewModeKey(core::ViewMode::FirstPerson3D)
                        ? core::ViewMode::FirstPerson3D
                        : core::ViewMode::Classic2D;
    data.lastPlayed = requireText(document, "lastPlayed");
    if (data.lastPlayed.size() > static_cast<std::size_t>(MAX_TIMESTAMP_LENGTH)) {
        throw ParseFailure{"invalid lastPlayed"};
    }
}

core::LevelRecord readRecord(const KeyValueDocument& document, int level) {
    const std::vector<std::string> parts = splitList(requireText(document, levelKey(level)), ',');
    const std::optional<long long> stars =
        parts.size() == 2 ? parseLongLong(parts[0]) : std::nullopt;
    const std::optional<long long> time =
        parts.size() == 2 ? parseLongLong(parts[1]) : std::nullopt;
    if (!stars || !time || *stars < 0 || *stars > core::MAX_STARS || *time < 0 ||
        *time > core::MAX_RECORD_MILLISECONDS) {
        throw ParseFailure{"invalid record for level " + std::to_string(level)};
    }
    return core::LevelRecord{static_cast<int>(*stars), static_cast<int>(*time), *stars > 0};
}

void readSkins(const KeyValueDocument& document, core::Progress& progress) {
    std::uint32_t mask = 0;
    for (const std::string& key : splitList(requireText(document, "skins"), ',')) {
        const std::optional<core::SkinId> skin = core::parseSkinKey(key);
        if (!skin) {
            throw ParseFailure{"unknown skin '" + key + "'"};
        }
        mask |= 1U << static_cast<unsigned>(*skin);
    }
    progress.restoreSkinMask(mask);
    const std::optional<core::SkinId> selected =
        core::parseSkinKey(requireText(document, "selectedSkin"));
    if (!selected || !progress.selectSkin(*selected)) {
        throw ParseFailure{"invalid selected skin"};
    }
}

void readProgress(const KeyValueDocument& document, core::Progress& progress) {
    progress.restoreHighestUnlocked(
        static_cast<int>(requireInt(document, "highestUnlocked", 1, core::LEVEL_COUNT)));
    for (int level = 1; level <= core::LEVEL_COUNT; ++level) {
        const core::LevelRecord record = readRecord(document, level);
        if (record.completed && level >= progress.highestUnlockedLevel() &&
            level < core::LEVEL_COUNT) {
            throw ParseFailure{"level " + std::to_string(level) + " completed but next is locked"};
        }
        progress.restoreRecord(level, record);
    }
    readSkins(document, progress);
    progress.restoreTierIntroMask(static_cast<std::uint32_t>(
        requireInt(document, "tierIntros", 0, (1 << core::TIER_COUNT) - 1)));
}

bool isHexText(const std::string& text) {
    for (const char character : text) {
        const bool digit = character >= '0' && character <= '9';
        const bool letter = character >= 'a' && character <= 'f';
        if (!digit && !letter) {
            return false;
        }
    }
    return true;
}

std::vector<int> readCollected(const KeyValueDocument& document) {
    std::vector<int> ids;
    for (const std::string& part : splitList(requireText(document, "snapshot.collected"), ',')) {
        const std::optional<long long> id = parseLongLong(part);
        if (!id || *id < 0 || *id >= MAX_COLLECTED_PERKS) {
            throw ParseFailure{"invalid collected perk id"};
        }
        ids.push_back(static_cast<int>(*id));
    }
    return ids;
}

void readSnapshotPositions(const KeyValueDocument& document, core::LevelSnapshot& snapshot,
                           int size) {
    snapshot.playerCell =
        core::Cell{static_cast<int>(requireInt(document, "snapshot.playerX", 0, size - 1)),
                   static_cast<int>(requireInt(document, "snapshot.playerY", 0, size - 1))};
    const std::optional<core::Direction> facing =
        core::parseDirection(requireText(document, "snapshot.facing"));
    if (!facing) {
        throw ParseFailure{"invalid facing"};
    }
    snapshot.facing = *facing;
    snapshot.chaserActive = requireBool(document, "snapshot.chaserActive");
    snapshot.chaserCell =
        core::Cell{static_cast<int>(requireInt(document, "snapshot.chaserX", 0, size - 1)),
                   static_cast<int>(requireInt(document, "snapshot.chaserY", 0, size - 1))};
}

core::LevelSnapshot readSnapshot(const KeyValueDocument& document, int size) {
    core::LevelSnapshot snapshot;
    snapshot.level = static_cast<int>(requireInt(document, "snapshot.level", 1, core::LEVEL_COUNT));
    snapshot.seed = static_cast<std::uint32_t>(requireInt(document, "snapshot.seed", 0, MAX_SEED));
    readSnapshotPositions(document, snapshot, size);
    snapshot.elapsedMs = static_cast<int>(
        requireInt(document, "snapshot.elapsedMs", 0, core::MAX_RECORD_MILLISECONDS));
    snapshot.timerStarted = requireBool(document, "snapshot.timerStarted");
    snapshot.berries =
        static_cast<int>(requireInt(document, "snapshot.berries", 0, core::BERRY_CAPACITY));
    for (const core::EffectType type : core::ALL_EFFECT_TYPES) {
        const auto limit =
            static_cast<long long>(core::effectDuration(type) * MILLISECONDS_PER_SECOND);
        snapshot.effectMs[static_cast<std::size_t>(type)] =
            static_cast<int>(requireInt(document, effectKeyName(type), 0, limit));
    }
    snapshot.collectedPerkIds = readCollected(document);
    snapshot.exploredHex = requireText(document, "snapshot.explored");
    const std::size_t expectedLength =
        (static_cast<std::size_t>(size) * static_cast<std::size_t>(size) + BITS_PER_HEX_DIGIT - 1) /
        BITS_PER_HEX_DIGIT;
    if (!isHexText(snapshot.exploredHex) ||
        (!snapshot.exploredHex.empty() && snapshot.exploredHex.size() != expectedLength)) {
        throw ParseFailure{"invalid explored map"};
    }
    snapshot.chaserActiveMs = static_cast<int>(
        requireInt(document, "snapshot.chaserActiveMs", 0, core::MAX_RECORD_MILLISECONDS));
    snapshot.graceMs = static_cast<int>(
        requireInt(document, "snapshot.graceMs", 0,
                   static_cast<long long>(core::CHASER_GRACE_SECONDS * MILLISECONDS_PER_SECOND)));
    // Saves written before the step counter existed simply resume counting from zero.
    if (document.has("snapshot.steps")) {
        snapshot.steps =
            static_cast<int>(requireInt(document, "snapshot.steps", 0, MAX_SNAPSHOT_STEPS));
    }
    return snapshot;
}

} // namespace

std::string serializeSave(const core::SaveData& data) {
    KeyValueDocument document;
    writeProfile(document, data);
    writeProgress(document, data.progress);
    if (data.snapshot) {
        writeSnapshot(document, *data.snapshot);
    } else {
        document.setBool("snapshot", false);
    }
    return document.serialize();
}

LoadResult parseSave(std::string_view text) {
    LoadResult result;
    std::string error;
    const std::optional<KeyValueDocument> document = KeyValueDocument::parse(text, error);
    if (!document) {
        result.status = LoadStatus::Corrupt;
        result.error = error;
        return result;
    }
    try {
        readProfile(*document, result.data);
        readProgress(*document, result.data.progress);
        if (requireBool(*document, "snapshot")) {
            result.data.snapshot = readSnapshot(*document, result.data.mazeSize);
        }
        result.status = LoadStatus::Ok;
    } catch (const ParseFailure& failure) {
        result = LoadResult{};
        result.status = LoadStatus::Corrupt;
        result.error = failure.message;
    }
    return result;
}

std::string currentTimestamp() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return std::to_string(local.tm_year + YEAR_OFFSET) + "-" + twoDigits(local.tm_mon + 1) + "-" +
           twoDigits(local.tm_mday) + " " + twoDigits(local.tm_hour) + ":" +
           twoDigits(local.tm_min);
}

SaveSystem::SaveSystem(std::filesystem::path directory) : m_directory(std::move(directory)) {}

std::filesystem::path SaveSystem::slotPath(int slot) const {
    return m_directory / ("slot" + std::to_string(slot) + ".sav");
}

LoadResult SaveSystem::load(int slot) const {
    LoadResult result;
    if (!core::isValidSlot(slot)) {
        result.status = LoadStatus::Corrupt;
        result.error = "invalid slot number";
        return result;
    }
    std::error_code code;
    if (!std::filesystem::exists(slotPath(slot), code)) {
        result.status = LoadStatus::Missing;
        return result;
    }
    std::string error;
    const std::optional<std::string> text =
        readTextFile(slotPath(slot), static_cast<std::uintmax_t>(core::MAX_SAVE_FILE_BYTES), error);
    if (!text) {
        result.status = LoadStatus::Corrupt;
        result.error = error;
        return result;
    }
    return parseSave(*text);
}

bool SaveSystem::save(int slot, const core::SaveData& data, std::string& error) const {
    if (!core::isValidSlot(slot)) {
        error = "invalid slot number";
        return false;
    }
    return writeTextFileAtomically(slotPath(slot), serializeSave(data), error);
}

bool SaveSystem::erase(int slot) const {
    if (!core::isValidSlot(slot)) {
        return false;
    }
    std::error_code code;
    std::filesystem::remove(slotPath(slot), code);
    return !std::filesystem::exists(slotPath(slot), code);
}

} // namespace io
