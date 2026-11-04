#include "core/Random.h"

#include <algorithm>
#include <limits>

namespace core {

namespace {

constexpr float FLOAT_MANTISSA_SCALE = 1.0f / 16777216.0f; // 2^-24
constexpr int FLOAT_MANTISSA_SHIFT = 8;                    // keep the top 24 bits

/// SplitMix64 finaliser: a cheap, high quality 64-bit integer hash.
std::uint64_t mix64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

} // namespace

Rng::Rng(std::uint32_t seed) : m_engine(seed) {}

std::uint32_t Rng::nextU32() {
    return static_cast<std::uint32_t>(m_engine());
}

int Rng::nextInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
        std::swap(minInclusive, maxInclusive);
    }
    const auto span = static_cast<std::uint64_t>(static_cast<std::int64_t>(maxInclusive) -
                                                 static_cast<std::int64_t>(minInclusive)) +
                      1U;
    constexpr std::uint64_t ENGINE_RANGE =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
    // Reject the top partial bucket so every result is equally likely.
    const std::uint64_t limit = ENGINE_RANGE - (ENGINE_RANGE % span);
    std::uint64_t value = nextU32();
    while (value >= limit) {
        value = nextU32();
    }
    return static_cast<int>(static_cast<std::int64_t>(minInclusive) +
                            static_cast<std::int64_t>(value % span));
}

float Rng::nextFloat() {
    return static_cast<float>(nextU32() >> FLOAT_MANTISSA_SHIFT) * FLOAT_MANTISSA_SCALE;
}

bool Rng::chance(float probability) {
    const float clamped = std::clamp(probability, 0.0f, 1.0f);
    return nextFloat() < clamped;
}

std::uint32_t makeRandomSeed() {
    std::random_device device;
    return static_cast<std::uint32_t>(device());
}

std::uint32_t deriveLevelSeed(std::uint32_t baseSeed, int level, int mazeSize) {
    std::uint64_t state = mix64(baseSeed);
    state = mix64(state ^ static_cast<std::uint64_t>(static_cast<std::uint32_t>(level)));
    state =
        mix64(state ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(mazeSize)) << 32U));
    return static_cast<std::uint32_t>(state ^ (state >> 32U));
}

} // namespace core
