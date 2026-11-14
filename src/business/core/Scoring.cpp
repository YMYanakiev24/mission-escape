#include "core/Scoring.h"

#include "core/Config.h"

#include <cmath>
#include <limits>

namespace core {

namespace {

constexpr int MILLISECONDS_PER_SECOND = 1000;
constexpr int TENTHS_PER_SECOND = 10;
constexpr int SECONDS_PER_MINUTE = 60;
constexpr int TWO_DIGITS = 10;

} // namespace

float tierTimeMultiplier(Tier tier) {
    switch (tier) {
        case Tier::Awakening:
            return TIME_MULTIPLIER_AWAKENING;
        case Tier::Labyrinth:
            return TIME_MULTIPLIER_LABYRINTH;
        case Tier::Fog:
            return TIME_MULTIPLIER_FOG;
        case Tier::Hunted:
            return TIME_MULTIPLIER_HUNTED;
    }
    return TIME_MULTIPLIER_AWAKENING;
}

float modeTimeMultiplier(ViewMode mode) {
    return mode == ViewMode::FirstPerson3D ? TIME_MULTIPLIER_3D : TIME_MULTIPLIER_2D;
}

float parTime(int shortestPathLength, Tier tier, ViewMode mode) {
    const int length = shortestPathLength > 0 ? shortestPathLength : 0;
    return static_cast<float>(length) * SECONDS_PER_CELL * tierTimeMultiplier(tier) *
           modeTimeMultiplier(mode);
}

float starTimeLimit(int stars, float par) {
    if (stars >= MAX_STARS) {
        return par * THREE_STAR_FACTOR;
    }
    if (stars == 2) {
        return par * TWO_STAR_FACTOR;
    }
    return std::numeric_limits<float>::infinity();
}

int starsForTime(float seconds, float par) {
    if (seconds <= starTimeLimit(MAX_STARS, par)) {
        return MAX_STARS;
    }
    if (seconds <= starTimeLimit(2, par)) {
        return 2;
    }
    return 1;
}

StarTarget starTargetAt(float elapsed, float par) {
    for (int stars = MAX_STARS; stars > 1; --stars) {
        const float limit = starTimeLimit(stars, par);
        if (elapsed <= limit) {
            return StarTarget{stars, limit - elapsed};
        }
    }
    return StarTarget{1, 0.0f};
}

int toMilliseconds(float seconds) {
    if (!(seconds > 0.0f)) {
        return 0;
    }
    const double milliseconds = std::round(static_cast<double>(seconds) * MILLISECONDS_PER_SECOND);
    if (milliseconds >= static_cast<double>(MAX_RECORD_MILLISECONDS)) {
        return MAX_RECORD_MILLISECONDS;
    }
    return static_cast<int>(milliseconds);
}

bool updateRecord(LevelRecord& record, int stars, int timeMs) {
    if (stars <= QUIT_STARS) {
        return false;
    }
    const bool firstCompletion = !record.completed;
    const bool moreStars = stars > record.bestStars;
    const bool fasterTime = record.bestTimeMs <= 0 || timeMs < record.bestTimeMs;
    record.completed = true;
    if (moreStars) {
        record.bestStars = stars;
    }
    if (fasterTime) {
        record.bestTimeMs = timeMs;
    }
    return firstCompletion || moreStars || fasterTime;
}

std::string formatDuration(float seconds) {
    const int totalTenths =
        seconds > 0.0f ? static_cast<int>(std::floor(seconds * TENTHS_PER_SECOND)) : 0;
    const int tenths = totalTenths % TENTHS_PER_SECOND;
    const int totalSeconds = totalTenths / TENTHS_PER_SECOND;
    const int minutes = totalSeconds / SECONDS_PER_MINUTE;
    const int secondsPart = totalSeconds % SECONDS_PER_MINUTE;
    std::string text = std::to_string(minutes) + ":";
    if (secondsPart < TWO_DIGITS) {
        text += "0";
    }
    text += std::to_string(secondsPart) + "." + std::to_string(tenths);
    return text;
}

} // namespace core
