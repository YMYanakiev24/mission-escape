#include "core/Cell.h"

namespace core {

std::string_view directionName(Direction direction) {
    switch (direction) {
        case Direction::North:
            return "north";
        case Direction::East:
            return "east";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
    }
    return "north";
}

std::optional<Direction> parseDirection(std::string_view text) {
    for (const Direction direction : ALL_DIRECTIONS) {
        if (directionName(direction) == text) {
            return direction;
        }
    }
    return std::nullopt;
}

} // namespace core
