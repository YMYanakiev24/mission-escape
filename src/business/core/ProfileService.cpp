#include "core/ProfileService.h"

#include "core/Validation.h"

#include <chrono>
#include <ctime>
#include <utility>

namespace core {

namespace {

constexpr int YEAR_OFFSET = 1900;
constexpr int TWO_DIGITS = 10;

std::string twoDigits(int value) {
    return (value < TWO_DIGITS ? "0" : "") + std::to_string(value);
}

std::size_t slotIndex(int slot) {
    return static_cast<std::size_t>(slot - 1);
}

SaveSlot slotFromLoadResult(LoadResult result) {
    SaveSlot slot;
    switch (result.status) {
        case LoadStatus::Ok:
            slot.status = SlotStatus::Ready;
            slot.data = std::move(result.data);
            break;
        case LoadStatus::Missing:
            slot.status = SlotStatus::Empty;
            break;
        case LoadStatus::Corrupt:
            slot.status = SlotStatus::Corrupt;
            slot.error = std::move(result.error);
            break;
    }
    return slot;
}

} // namespace

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

ProfileService::ProfileService(SaveRepository& saves, SettingsRepository& settingsStore,
                               bool persistenceEnabled)
    : m_saves(saves), m_settingsStore(settingsStore), m_persistenceEnabled(persistenceEnabled) {}

void ProfileService::loadAll() {
    if (!m_persistenceEnabled) {
        return;
    }
    m_settings = m_settingsStore.load();
    for (int slot = 1; slot <= SAVE_SLOT_COUNT; ++slot) {
        m_slots[slotIndex(slot)] = slotFromLoadResult(m_saves.load(slot));
    }
}

SaveData* ProfileService::activeProfile() {
    if (!isValidSlot(m_activeSlot)) {
        return nullptr;
    }
    SaveSlot& slot = m_slots[slotIndex(m_activeSlot)];
    return slot.status == SlotStatus::Ready && slot.data ? &*slot.data : nullptr;
}

bool ProfileService::activateSlot(int slot) {
    if (!isValidSlot(slot) || m_slots[slotIndex(slot)].status != SlotStatus::Ready) {
        return false;
    }
    m_activeSlot = slot;
    m_settings.lastSlot = slot;
    saveSettings();
    return true;
}

void ProfileService::createProfile(int slot, SaveData data) {
    if (!isValidSlot(slot)) {
        return;
    }
    SaveSlot& target = m_slots[slotIndex(slot)];
    target.status = SlotStatus::Ready;
    target.data = std::move(data);
    target.error.clear();
    activateSlot(slot);
    saveActiveProfile();
}

void ProfileService::deleteSlot(int slot) {
    if (!isValidSlot(slot)) {
        return;
    }
    if (m_persistenceEnabled) {
        m_saves.erase(slot);
    }
    m_slots[slotIndex(slot)] = SaveSlot{};
    if (m_activeSlot == slot) {
        m_activeSlot = 0;
    }
    if (m_settings.lastSlot == slot) {
        m_settings.lastSlot = 0;
        saveSettings();
    }
}

std::optional<RecordedResult> ProfileService::recordResult(int level, int stars, int timeMs) {
    SaveData* profile = activeProfile();
    if (profile == nullptr) {
        return std::nullopt;
    }
    RecordedResult result;
    result.update = profile->progress.applyResult(level, stars, timeMs);
    result.record = profile->progress.record(level);
    profile->snapshot.reset(); // the attempt is over, so "Continue" must not bring it back
    saveActiveProfile();       // autosave after every completed level
    return result;
}

void ProfileService::storeSnapshot(const LevelSnapshot& snapshot) {
    if (SaveData* profile = activeProfile()) {
        profile->snapshot = snapshot;
        saveActiveProfile();
    }
}

void ProfileService::discardSavedAttempt() {
    SaveData* profile = activeProfile();
    if (profile != nullptr && profile->snapshot) {
        profile->snapshot.reset();
        saveActiveProfile();
    }
}

void ProfileService::saveActiveProfile() {
    SaveData* profile = activeProfile();
    if (profile == nullptr || !m_persistenceEnabled) {
        return;
    }
    profile->lastPlayed = currentTimestamp();
    std::string error;
    if (!m_saves.save(m_activeSlot, *profile, error)) {
        m_lastError = "Could not save: " + error;
    }
}

void ProfileService::saveSettings() {
    if (!m_persistenceEnabled) {
        return;
    }
    std::string error;
    if (!m_settingsStore.save(m_settings, error)) {
        m_lastError = "Could not save settings: " + error;
    }
}

} // namespace core
