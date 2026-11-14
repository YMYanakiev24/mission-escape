#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

/// @file Random.h
/// Deterministic, portable random numbers for maze generation.

namespace core {

/// Seeded random number generator built on `std::mt19937`.
///
/// Only the raw engine output is fixed by the C++ standard; the standard distributions and
/// `std::shuffle` may differ between library vendors. To guarantee that a seed regenerates the
/// exact same maze on every compiler, all derived values are computed here by hand.
class Rng {
public:
    /// Creates a generator whose whole sequence is determined by @p seed.
    explicit Rng(std::uint32_t seed);

    /// Next raw 32-bit value of the engine.
    std::uint32_t nextU32();

    /// Uniform integer in [minInclusive, maxInclusive] without modulo bias.
    /// If the bounds are swapped they are treated as [maxInclusive, minInclusive].
    int nextInt(int minInclusive, int maxInclusive);

    /// Uniform float in [0, 1).
    float nextFloat();

    /// True with the given probability (clamped to [0, 1]).
    bool chance(float probability);

    /// In-place Fisher-Yates shuffle with portable index selection.
    template <typename T>
    void shuffle(std::vector<T>& values) {
        for (int index = static_cast<int>(values.size()) - 1; index > 0; --index) {
            const int other = nextInt(0, index);
            std::swap(values[static_cast<std::size_t>(index)],
                      values[static_cast<std::size_t>(other)]);
        }
    }

private:
    std::mt19937 m_engine;
};

/// Draws a fresh non-deterministic seed from `std::random_device`.
std::uint32_t makeRandomSeed();

/// Mixes a base seed with the level number and maze size so that every combination gets an
/// independent, well-distributed generator seed (SplitMix64 finaliser).
std::uint32_t deriveLevelSeed(std::uint32_t baseSeed, int level, int mazeSize);

} // namespace core
