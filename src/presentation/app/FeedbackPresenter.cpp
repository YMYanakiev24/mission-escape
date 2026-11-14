#include "app/FeedbackPresenter.h"

#include "app/AppContext.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"

#include <string>

namespace app {

namespace palette = render::palette;

namespace {

constexpr int PICKUP_PARTICLES = 34;
constexpr int DOOR_PARTICLES = 22;
constexpr int WIN_PARTICLES = 90;
constexpr int PHASE_RING_PARTICLES = 28;
constexpr float PARTICLE_SPEED_CELLS = 4.0f;
constexpr float STEP_VOLUME = 0.45f;
constexpr float BUMP_VOLUME = 0.8f;

std::string perkToast(core::PerkType perk) {
    switch (perk) {
        case core::PerkType::MagicBerry:
            return "Magic Berry stored - press SHIFT to phase";
        case core::PerkType::Hourglass:
            return "Hourglass - timer frozen for 5 s";
        case core::PerkType::CompassSpark:
            return "Compass Spark - follow the trail";
        case core::PerkType::Lantern:
            return "Lantern - sight doubled for 10 s";
        case core::PerkType::FrostShard:
            return "Frost Shard - the Hunter is frozen";
    }
    return "";
}

} // namespace

FeedbackPresenter::FeedbackPresenter(AppContext& context, render::Effects& effects)
    : m_context(context), m_effects(effects) {}

void FeedbackPresenter::presentPerk(core::PerkType perk, Vector2 screenPosition, float cellPixels) {
    m_context.audio.play(audio::SoundId::Pickup);
    const Color color = render::perkColor(perk);
    m_effects.burst(screenPosition, color, PICKUP_PARTICLES, cellPixels * PARTICLE_SPEED_CELLS);
    m_effects.toast(perkToast(perk), color);
}

void FeedbackPresenter::present(const core::LevelEvent& event, Vector2 screenPosition,
                                float cellPixels) {
    const float speed = cellPixels * PARTICLE_SPEED_CELLS;
    switch (event.type) {
        case core::LevelEventType::PerkCollected:
            presentPerk(event.perk, screenPosition, cellPixels);
            break;
        case core::LevelEventType::InventoryFull:
            m_context.audio.play(audio::SoundId::Bump, 0.6f, 1.4f);
            m_effects.toast("Inventory full - you can carry 3 Magic Berries", palette::WALL_GLOW);
            break;
        case core::LevelEventType::PhaseStarted:
            m_context.audio.play(audio::SoundId::Phase);
            m_effects.ring(screenPosition, palette::WALL_GLOW, cellPixels, PHASE_RING_PARTICLES);
            break;
        case core::LevelEventType::PhaseEnded:
            m_effects.burst(screenPosition, palette::WALL, DOOR_PARTICLES / 2, speed * 0.5f);
            break;
        case core::LevelEventType::NoBerries:
            m_context.audio.play(audio::SoundId::Bump, 0.6f, 1.6f);
            m_effects.toast("No Magic Berries - find one first", palette::WALL_GLOW);
            break;
        case core::LevelEventType::SecretDoorUsed:
            m_context.audio.play(audio::SoundId::SecretDoor);
            m_effects.burst(screenPosition, palette::ACCENT_GREEN, DOOR_PARTICLES, speed);
            break;
        case core::LevelEventType::ChaserSpawned:
            m_context.audio.play(audio::SoundId::Caught, 0.45f, 1.5f);
            m_effects.toast("The Hunter is awake!", palette::DANGER);
            break;
        case core::LevelEventType::Won:
            m_context.audio.play(audio::SoundId::LevelComplete);
            m_effects.burst(screenPosition, palette::ACCENT_GREEN, WIN_PARTICLES, speed * 1.6f);
            break;
        case core::LevelEventType::Moved:
            // Alternate the pitch slightly so repeated footsteps do not sound mechanical.
            m_stepToggle = !m_stepToggle;
            m_context.audio.play(audio::SoundId::Step, STEP_VOLUME, m_stepToggle ? 1.0f : 1.08f);
            break;
        case core::LevelEventType::Caught:
            m_context.audio.play(audio::SoundId::Caught);
            break;
        case core::LevelEventType::TimerStarted:
            break;
    }
}

void FeedbackPresenter::bump() {
    m_context.audio.play(audio::SoundId::Bump, BUMP_VOLUME);
}

} // namespace app
