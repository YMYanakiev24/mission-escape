#include "core/TextView.h"

#include <cstddef>

namespace core {

namespace {

/// Character grid with the same (column, row) addressing as the final text.
class TextCanvas {
public:
    explicit TextCanvas(int dimension) : m_chars(dimension, dimension, TEXT_WALL) {}

    void put(int column, int row, char symbol) { m_chars[row][column] = symbol; }
    int dimension() const { return m_chars.width(); }

    std::string toString() const {
        std::string text;
        const auto side = static_cast<std::size_t>(dimension());
        text.reserve(side * (side + 1));
        for (int row = 0; row < dimension(); ++row) {
            if (row > 0) {
                text.push_back('\n');
            }
            text.append(m_chars[row], side);
        }
        return text;
    }

private:
    Grid2D<char> m_chars;
};

char wallSymbol(const Maze& maze, Cell cell, Direction direction) {
    if (maze.isOpen(cell, direction)) {
        return TEXT_OPEN;
    }
    return maze.hasSecretDoor(cell, direction) ? TEXT_SECRET_DOOR : TEXT_WALL;
}

void drawLayout(const Maze& maze, TextCanvas& canvas) {
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const Cell cell{x, y};
            canvas.put(2 * x + 1, 2 * y + 1, TEXT_OPEN);
            // East and south walls only: west/north are drawn by the neighbouring cell and
            // the outer boundary keeps its initial '#'.
            if (!maze.isBoundary(cell, Direction::East)) {
                canvas.put(2 * x + 2, 2 * y + 1, wallSymbol(maze, cell, Direction::East));
            }
            if (!maze.isBoundary(cell, Direction::South)) {
                canvas.put(2 * x + 1, 2 * y + 2, wallSymbol(maze, cell, Direction::South));
            }
        }
    }
    canvas.put(2 * maze.start().x + 1, 2 * maze.start().y + 1, TEXT_START);
    canvas.put(2 * maze.exit().x + 1, 2 * maze.exit().y + 1, TEXT_EXIT);
}

/// True if any maze cell touching text position (column, row) has been explored.
bool touchesExploredCell(const Grid2D<bool>& explored, int column, int row) {
    // A text position touches the cells whose interior lies at most one character away.
    for (int cellY = (row - 1) / 2; cellY <= row / 2; ++cellY) {
        for (int cellX = (column - 1) / 2; cellX <= column / 2; ++cellX) {
            if (explored.inBounds(cellX, cellY) && explored[cellY][cellX]) {
                return true;
            }
        }
    }
    return false;
}

void hideUnexplored(const Grid2D<bool>& explored, TextCanvas& canvas) {
    for (int row = 0; row < canvas.dimension(); ++row) {
        for (int column = 0; column < canvas.dimension(); ++column) {
            if (!touchesExploredCell(explored, column, row)) {
                canvas.put(column, row, TEXT_UNKNOWN);
            }
        }
    }
}

} // namespace

std::string mazeToText(const Maze& maze, const TextViewOptions& options) {
    TextCanvas canvas(textViewDimension(maze.size()));
    drawLayout(maze, canvas);
    const bool fogMatchesMaze = options.explored != nullptr &&
                                options.explored->width() == maze.size() &&
                                options.explored->height() == maze.size();
    if (fogMatchesMaze) {
        hideUnexplored(*options.explored, canvas);
    }
    for (const TextMarker& marker : options.markers) {
        if (maze.inBounds(marker.cell)) {
            canvas.put(2 * marker.cell.x + 1, 2 * marker.cell.y + 1, marker.symbol);
        }
    }
    return canvas.toString();
}

} // namespace core
