#include "render/MenuBackground.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <algorithm>
#include <cstddef>

namespace render {

namespace {

constexpr int BACKGROUND_MAZE_SIZE = 36;
constexpr float STEPS_PER_SECOND = 150.0f;
constexpr float HOLD_SECONDS = 2.5f;
constexpr float FADE_SECONDS = 1.5f;
constexpr float PASSAGE_THICKNESS = 0.22f;
constexpr float STACK_THICKNESS = 0.16f;
constexpr float BACKDROP_OPACITY = 0.30f;
constexpr std::size_t TRAIL_CELLS = 28;

} // namespace

MenuBackground::MenuBackground() : m_rng(core::makeRandomSeed()) {
    restart();
}

void MenuBackground::restart() {
    m_builder = std::make_unique<core::DfsMazeBuilder>(BACKGROUND_MAZE_SIZE, m_rng);
    m_stepBudget = 0.0f;
    m_finishedTime = 0.0f;
}

void MenuBackground::update(float deltaSeconds) {
    if (m_builder->finished()) {
        m_finishedTime += deltaSeconds;
        if (m_finishedTime > HOLD_SECONDS + FADE_SECONDS) {
            restart();
        }
        return;
    }
    m_stepBudget += deltaSeconds * STEPS_PER_SECOND;
    while (m_stepBudget >= 1.0f && m_builder->step()) {
        m_stepBudget -= 1.0f;
    }
}

void MenuBackground::draw(float time) const {
    ClearBackground(palette::BG_DEEP);
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    // Square cells sized to cover the wider screen dimension; the maze is centred.
    const float cellSize = std::max(width, height) / static_cast<float>(BACKGROUND_MAZE_SIZE);
    const float extent = cellSize * static_cast<float>(BACKGROUND_MAZE_SIZE);
    const Layout layout{Vector2{(width - extent) / 2.0f, (height - extent) / 2.0f}, cellSize};
    const float fadeOut =
        m_builder->finished()
            ? 1.0f - std::clamp((m_finishedTime - HOLD_SECONDS) / FADE_SECONDS, 0.0f, 1.0f)
            : 1.0f;
    drawPassages(layout, BACKDROP_OPACITY * fadeOut);
    drawStackTrail(layout, fadeOut, time);
    drawVignette(width, height);
}

Vector2 MenuBackground::Layout::centerOf(core::Cell cell) const {
    return Vector2{origin.x + (static_cast<float>(cell.x) + 0.5f) * cellSize,
                   origin.y + (static_cast<float>(cell.y) + 0.5f) * cellSize};
}

void MenuBackground::drawPassages(const Layout& layout, float opacity) const {
    const core::Maze& maze = m_builder->maze();
    const Color passage = fade(palette::WALL, opacity);
    for (int y = 0; y < maze.size(); ++y) {
        for (int x = 0; x < maze.size(); ++x) {
            const core::Cell here{x, y};
            for (const core::Direction direction :
                 {core::Direction::East, core::Direction::South}) {
                if (maze.isOpen(here, direction)) {
                    DrawLineEx(layout.centerOf(here),
                               layout.centerOf(core::neighbour(here, direction)),
                               layout.cellSize * PASSAGE_THICKNESS, passage);
                }
            }
        }
    }
}

void MenuBackground::drawStackTrail(const Layout& layout, float fadeOut, float time) const {
    // Only the newest part of the DFS stack is highlighted, fading towards its tail.
    const std::vector<core::Cell>& stack = m_builder->stack();
    const std::size_t first = stack.size() > TRAIL_CELLS ? stack.size() - TRAIL_CELLS : 1;
    for (std::size_t index = std::max<std::size_t>(first, 1); index < stack.size(); ++index) {
        const float age =
            static_cast<float>(stack.size() - index) / static_cast<float>(TRAIL_CELLS);
        DrawLineEx(layout.centerOf(stack[index - 1]), layout.centerOf(stack[index]),
                   layout.cellSize * STACK_THICKNESS,
                   fade(palette::ACCENT_GREEN, 0.55f * (1.0f - age) * fadeOut));
    }
    if (!stack.empty()) {
        const Vector2 head = layout.centerOf(stack.back());
        drawGlow(head, layout.cellSize * (1.6f + 0.4f * pulse(time, 2.0f)),
                 fade(palette::ACCENT_GREEN, 0.5f * fadeOut));
        DrawCircleV(head, layout.cellSize * 0.18f, fade(palette::TEXT, fadeOut));
    }
}

void MenuBackground::drawVignette(float width, float height) const {
    // Darker centre and edges keep the menu in the middle readable.
    const Color clear = fade(palette::BG_DEEP, 0.0f);
    DrawRectangleGradientH(0, 0, static_cast<int>(width * 0.5f), static_cast<int>(height),
                           fade(palette::BG_DEEP, 0.35f), clear);
    DrawRectangleGradientH(static_cast<int>(width * 0.5f), 0, static_cast<int>(width * 0.5f) + 1,
                           static_cast<int>(height), clear, fade(palette::BG_DEEP, 0.35f));
    drawGlow(Vector2{width / 2.0f, height / 2.0f}, std::max(width, height) * 0.45f,
             fade(palette::BG_DEEP, 0.75f));
}

} // namespace render
