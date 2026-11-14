#include "core/Maze.h"

namespace core {

Maze::Maze(int size) : m_cells(size, size, cellbits::ALL_WALLS) {}

bool Maze::hasWall(Cell cell, Direction direction) const {
    return (cellBits(cell) & wallBit(direction)) != 0U;
}

bool Maze::hasSecretDoor(Cell cell, Direction direction) const {
    return (cellBits(cell) & secretBit(direction)) != 0U;
}

bool Maze::isBoundary(Cell cell, Direction direction) const {
    return !inBounds(neighbour(cell, direction));
}

bool Maze::isOpen(Cell cell, Direction direction) const {
    return !isBoundary(cell, direction) && !hasWall(cell, direction);
}

bool Maze::canPass(Cell cell, Direction direction, const PassRules& rules) const {
    if (!inBounds(cell) || isBoundary(cell, direction)) {
        return false;
    }
    if (!hasWall(cell, direction)) {
        return true;
    }
    if (rules.phaseThroughWalls) {
        return true;
    }
    return rules.useSecretDoors && hasSecretDoor(cell, direction);
}

void Maze::carvePassage(Cell cell, Direction direction) {
    if (!inBounds(cell) || isBoundary(cell, direction)) {
        return;
    }
    const Cell other = neighbour(cell, direction);
    const Direction back = opposite(direction);
    clearBits(cell, static_cast<std::uint8_t>(wallBit(direction) | secretBit(direction)));
    clearBits(other, static_cast<std::uint8_t>(wallBit(back) | secretBit(back)));
}

void Maze::addWall(Cell cell, Direction direction) {
    if (!inBounds(cell)) {
        return;
    }
    clearBits(cell, secretBit(direction));
    setBits(cell, wallBit(direction));
    if (!isBoundary(cell, direction)) {
        const Cell other = neighbour(cell, direction);
        const Direction back = opposite(direction);
        clearBits(other, secretBit(back));
        setBits(other, wallBit(back));
    }
}

bool Maze::setSecretDoor(Cell cell, Direction direction) {
    if (!inBounds(cell) || isBoundary(cell, direction) || !hasWall(cell, direction)) {
        return false;
    }
    const Cell other = neighbour(cell, direction);
    setBits(cell, secretBit(direction));
    setBits(other, secretBit(opposite(direction)));
    return true;
}

int Maze::openingCount(Cell cell) const {
    int count = 0;
    for (const Direction direction : ALL_DIRECTIONS) {
        if (isOpen(cell, direction)) {
            ++count;
        }
    }
    return count;
}

int Maze::passageCount() const {
    int count = 0;
    for (int y = 0; y < size(); ++y) {
        for (int x = 0; x < size(); ++x) {
            // Only look east and south so every shared wall is counted exactly once.
            if (isOpen(Cell{x, y}, Direction::East)) {
                ++count;
            }
            if (isOpen(Cell{x, y}, Direction::South)) {
                ++count;
            }
        }
    }
    return count;
}

int Maze::deadEndCount() const {
    int count = 0;
    for (int y = 0; y < size(); ++y) {
        for (int x = 0; x < size(); ++x) {
            if (isDeadEnd(Cell{x, y})) {
                ++count;
            }
        }
    }
    return count;
}

void Maze::clearBits(Cell cell, std::uint8_t bits) {
    std::uint8_t& value = m_cells.at(cell.x, cell.y);
    value = static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~bits));
}

} // namespace core
