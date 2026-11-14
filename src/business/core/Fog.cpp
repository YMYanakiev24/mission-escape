#include "core/Fog.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace core {

namespace {

constexpr int CELLS_PER_HEX_DIGIT = 4;
constexpr int HEX_LETTER_OFFSET = 10;
constexpr const char* HEX_DIGITS = "0123456789abcdef";

int hexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + HEX_LETTER_OFFSET;
    }
    return -1;
}

std::size_t encodedLength(int size) {
    const std::size_t cells = static_cast<std::size_t>(size) * static_cast<std::size_t>(size);
    return (cells + CELLS_PER_HEX_DIGIT - 1) / CELLS_PER_HEX_DIGIT;
}

} // namespace

Fog::Fog(int size) : m_explored(size, size, false), m_visible(size, size, false) {}

void Fog::markSeen(Cell cell) {
    m_visible[cell.y][cell.x] = true;
    m_explored[cell.y][cell.x] = true;
}

void Fog::reveal(const Maze& maze, Cell center, int radius) {
    m_visible.fill(false);
    if (!maze.inBounds(center) || m_visible.width() != maze.size()) {
        return;
    }
    // Depth-limited breadth-first flood fill; m_visible doubles as the visited set.
    std::vector<std::pair<Cell, int>> frontier{{center, 0}};
    markSeen(center);
    for (std::size_t head = 0; head < frontier.size(); ++head) {
        const auto [cell, depth] = frontier[head];
        if (depth >= radius) {
            continue;
        }
        for (const Direction direction : ALL_DIRECTIONS) {
            const Cell next = neighbour(cell, direction);
            if (maze.isOpen(cell, direction) && !m_visible[next.y][next.x]) {
                markSeen(next);
                frontier.emplace_back(next, depth + 1);
            }
        }
    }
}

void Fog::revealCorridors(const Maze& maze, Cell center, int maxDistance) {
    if (!maze.inBounds(center) || m_visible.width() != maze.size()) {
        return;
    }
    for (const Direction direction : ALL_DIRECTIONS) {
        Cell cell = center;
        for (int step = 0; step < maxDistance && maze.isOpen(cell, direction); ++step) {
            cell = neighbour(cell, direction);
            markSeen(cell);
        }
    }
}

bool Fog::isVisible(Cell cell) const {
    return m_visible.inBounds(cell.x, cell.y) && m_visible[cell.y][cell.x];
}

bool Fog::isExplored(Cell cell) const {
    return m_explored.inBounds(cell.x, cell.y) && m_explored[cell.y][cell.x];
}

int Fog::exploredCount() const {
    int count = 0;
    for (int y = 0; y < m_explored.height(); ++y) {
        for (int x = 0; x < m_explored.width(); ++x) {
            count += m_explored[y][x] ? 1 : 0;
        }
    }
    return count;
}

std::string Fog::encodeExplored() const {
    std::string hex;
    const int size = m_explored.width();
    hex.reserve(encodedLength(size));
    int digit = 0;
    int bits = 0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            digit = (digit << 1) | (m_explored[y][x] ? 1 : 0);
            if (++bits == CELLS_PER_HEX_DIGIT) {
                hex.push_back(HEX_DIGITS[digit]);
                digit = 0;
                bits = 0;
            }
        }
    }
    if (bits > 0) {
        hex.push_back(HEX_DIGITS[digit << (CELLS_PER_HEX_DIGIT - bits)]);
    }
    return hex;
}

bool Fog::decodeExplored(const std::string& hex) {
    const int size = m_explored.width();
    if (hex.size() != encodedLength(size)) {
        return false;
    }
    Grid2D<bool> decoded(size, size, false);
    for (int index = 0; index < size * size; ++index) {
        const int value = hexValue(hex[static_cast<std::size_t>(index / CELLS_PER_HEX_DIGIT)]);
        if (value < 0) {
            return false;
        }
        const int shift = CELLS_PER_HEX_DIGIT - 1 - index % CELLS_PER_HEX_DIGIT;
        decoded[index / size][index % size] = ((value >> shift) & 1) != 0;
    }
    m_explored = std::move(decoded);
    return true;
}

} // namespace core
