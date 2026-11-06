#include "core/Player.h"

#include "core/Config.h"

#include <algorithm>

namespace core {

namespace {

constexpr float MIN_MOVE_DURATION = 0.001f;

} // namespace

Mover::Mover(Cell cell) : m_cell(cell), m_previousCell(cell) {}

Vec2f Mover::visualPosition() const {
    const float t = std::clamp(m_progress, 0.0f, 1.0f);
    return Vec2f{
        static_cast<float>(m_previousCell.x) + static_cast<float>(m_cell.x - m_previousCell.x) * t,
        static_cast<float>(m_previousCell.y) + static_cast<float>(m_cell.y - m_previousCell.y) * t};
}

void Mover::beginMove(Cell target, float durationSeconds) {
    m_previousCell = m_cell;
    m_cell = target;
    m_duration = std::max(durationSeconds, MIN_MOVE_DURATION);
    m_progress = 0.0f;
}

bool Mover::update(float deltaSeconds) {
    if (!isMoving()) {
        return false;
    }
    m_progress = std::min(1.0f, m_progress + deltaSeconds / m_duration);
    if (m_progress >= 1.0f) {
        m_previousCell = m_cell;
        return true;
    }
    return false;
}

void Mover::placeAt(Cell cell) {
    m_cell = cell;
    m_previousCell = cell;
    m_progress = 1.0f;
}

Player::Player(Cell start, Direction facing) : Mover(start), m_facing(facing) {}

bool Player::addBerry() {
    if (m_berries >= BERRY_CAPACITY) {
        return false;
    }
    ++m_berries;
    return true;
}

bool Player::consumeBerry() {
    if (m_berries <= 0) {
        return false;
    }
    --m_berries;
    return true;
}

void Player::setBerries(int count) {
    m_berries = std::clamp(count, 0, BERRY_CAPACITY);
}

} // namespace core
