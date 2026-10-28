#include "ui/SettingsScreen.h"

#include "render/Palette.h"
#include "render/Primitives.h"
#include "render/SkinRenderer.h"
#include "ui/MenuStyle.h"

#include <raylib.h>

#include <initializer_list>
#include <string>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float TAB_ROW_HEIGHT = 46.0f;
constexpr float ROW_WIDTH = 640.0f;
constexpr float ROW_HEIGHT = 56.0f;
constexpr float ROW_GAP = 14.0f;
constexpr float CONTENT_OFFSET = 36.0f;
constexpr float CARD_GAP = 16.0f;
constexpr float CARD_HEIGHT = 250.0f;
constexpr float PREVIEW_RADIUS = 30.0f;
constexpr std::size_t AUDIO_TAB = 0;
constexpr std::size_t SKINS_TAB = 1;

} // namespace

// ------------------------------------------------------------------------------------------
// SkinCard
// ------------------------------------------------------------------------------------------

void SkinCard::configure(core::SkinId skin, bool unlocked, bool selected) {
    m_skin = skin;
    m_unlocked = unlocked;
    m_selected = selected;
}

bool SkinCard::handleInput(const app::InputState& input, bool focused) {
    if (!m_enabled || !m_unlocked) {
        return false;
    }
    return (input.mousePressed && contains(input.mouse)) || (focused && input.confirm);
}

void SkinCard::draw(const render::Theme& theme, bool focused, float time) const {
    const Color border =
        focused ? palette::ACCENT_GREEN : (m_selected ? palette::ACCENT_GREEN_DIM : palette::WALL);
    render::drawPanel(m_bounds, theme.px(12.0f), focused ? palette::PANEL_HI : palette::PANEL,
                      border, theme.px(focused || m_selected ? 3.0f : 1.5f));
    const float centerX = m_bounds.x + m_bounds.width / 2.0f;
    const Vector2 preview{centerX, m_bounds.y + m_bounds.height * 0.36f};
    const float radius = theme.px(PREVIEW_RADIUS);
    if (m_unlocked) {
        render::drawSkin(m_skin, preview, radius, time, time * 1.2f);
    } else {
        DrawCircleV(preview, radius, render::fade(palette::BG_DEEP, 0.9f));
        render::drawLockIcon(preview, radius * 1.1f, render::fade(palette::TEXT_MUTED, 0.7f));
    }
    theme.drawTextCentered(
        std::string(core::skinName(m_skin)), Vector2{centerX, m_bounds.y + m_bounds.height * 0.70f},
        theme.px(render::textsize::BODY), m_unlocked ? palette::TEXT : palette::TEXT_MUTED);
    std::string status = "Select";
    Color statusColor = palette::TEXT_MUTED;
    if (!m_unlocked) {
        status = "Complete level " + std::to_string(core::skinUnlockLevel(m_skin));
    } else if (m_selected) {
        status = "Selected";
        statusColor = palette::ACCENT_GREEN;
    }
    theme.drawTextCentered(status, Vector2{centerX, m_bounds.y + m_bounds.height * 0.86f},
                           theme.px(render::textsize::TINY), statusColor);
}

// ------------------------------------------------------------------------------------------
// SettingsScreen
// ------------------------------------------------------------------------------------------

SettingsScreen::SettingsScreen(app::AppContext& context) : m_context(context) {
    m_focus.setFeedback(&context.uiSounds);
}

void SettingsScreen::onEnter() {
    loadValues();
    refreshSkinCards();
    m_tabs.setActive(AUDIO_TAB);
    rebuildFocus();
    m_focus.focus(m_tabs.button(AUDIO_TAB));
}

void SettingsScreen::loadValues() {
    const core::Settings& settings = m_context.settings;
    m_master.setValue(settings.masterVolume);
    m_music.setValue(settings.musicVolume);
    m_sfx.setValue(settings.sfxVolume);
    m_mute.setValue(settings.muted);
    m_fullscreen.setValue(settings.fullscreen);
    m_showFps.setValue(settings.showFps);
    m_headBob.setValue(settings.headBob);
}

void SettingsScreen::refreshSkinCards() {
    const core::SaveData* profile = m_context.activeProfile();
    for (std::size_t index = 0; index < m_skinCards.size(); ++index) {
        const core::SkinId skin = core::ALL_SKINS[index];
        const bool unlocked = profile != nullptr ? profile->progress.isSkinUnlocked(skin)
                                                 : skin == core::SkinId::Wisp;
        const bool selected = profile != nullptr ? profile->progress.selectedSkin() == skin
                                                 : skin == core::SkinId::Wisp;
        m_skinCards[index].configure(skin, unlocked, selected);
    }
}

void SettingsScreen::rebuildFocus() {
    m_focus.clear();
    m_tabs.addTo(m_focus);
    switch (m_tabs.active()) {
        case AUDIO_TAB:
            for (Widget* widget :
                 std::initializer_list<Widget*>{&m_master, &m_music, &m_sfx, &m_mute}) {
                m_focus.add(*widget);
            }
            break;
        case SKINS_TAB:
            for (SkinCard& card : m_skinCards) {
                m_focus.add(card);
            }
            break;
        default:
            for (Widget* widget :
                 std::initializer_list<Widget*>{&m_fullscreen, &m_showFps, &m_headBob}) {
                m_focus.add(*widget);
            }
            break;
    }
    m_focus.add(m_back);
    m_builtTab = m_tabs.active();
}

void SettingsScreen::layout() {
    const render::Theme& theme = m_context.theme;
    const Rectangle panel = contentPanel(theme);
    m_tabs.layout(Rectangle{panel.x, panel.y - theme.px(TAB_ROW_HEIGHT + 10.0f), panel.width,
                            theme.px(TAB_ROW_HEIGHT)},
                  theme.px(10.0f));
    const float top = (panel.y + theme.px(CONTENT_OFFSET)) / theme.scale();
    int row = 0;
    for (Widget* widget : std::initializer_list<Widget*>{&m_master, &m_music, &m_sfx, &m_mute}) {
        widget->setBounds(stackedItem(theme, ROW_WIDTH, ROW_HEIGHT, top, row++, ROW_GAP));
    }
    row = 0;
    for (Widget* widget : std::initializer_list<Widget*>{&m_fullscreen, &m_showFps, &m_headBob}) {
        widget->setBounds(stackedItem(theme, ROW_WIDTH, ROW_HEIGHT, top, row++, ROW_GAP));
    }
    const float padding = theme.px(CONTENT_OFFSET);
    const float gap = theme.px(CARD_GAP);
    const float cardWidth =
        (panel.width - 2.0f * padding - gap * static_cast<float>(core::SKIN_COUNT - 1)) /
        static_cast<float>(core::SKIN_COUNT);
    for (std::size_t index = 0; index < m_skinCards.size(); ++index) {
        m_skinCards[index].setBounds(
            Rectangle{panel.x + padding + static_cast<float>(index) * (cardWidth + gap),
                      panel.y + padding, cardWidth, theme.px(CARD_HEIGHT)});
    }
    const float width = theme.px(220.0f);
    m_back.setBounds(Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                               panel.y + panel.height + theme.px(18.0f), width, theme.px(48.0f)});
}

void SettingsScreen::applyChanges(const Widget* fired) {
    core::Settings& settings = m_context.settings;
    settings.masterVolume = m_master.value();
    settings.musicVolume = m_music.value();
    settings.sfxVolume = m_sfx.value();
    settings.muted = m_mute.value();
    settings.showFps = m_showFps.value();
    settings.headBob = m_headBob.value();
    if (fired == &m_fullscreen) {
        settings.fullscreen = m_fullscreen.value();
        m_context.applyDisplaySettings();
    }
    for (const SkinCard& card : m_skinCards) {
        core::SaveData* profile = m_context.activeProfile();
        if (fired == &card && profile != nullptr && profile->progress.selectSkin(card.skin())) {
            m_context.saveActiveProfile();
            refreshSkinCards();
        }
    }
}

void SettingsScreen::leave() {
    m_context.saveSettings();
    m_context.navigator.pop();
}

void SettingsScreen::update(float deltaSeconds, const app::InputState& input) {
    m_context.menuBackground.update(deltaSeconds);
    layout();
    const Widget* fired = m_focus.update(input);
    m_tabs.update(m_focus, fired);
    if (m_tabs.active() != m_builtTab) {
        rebuildFocus();
    }
    applyChanges(fired);
    if (fired == &m_back || input.back) {
        leave();
    }
}

void SettingsScreen::draw() {
    const render::Theme& theme = m_context.theme;
    drawMenuBackdrop(m_context);
    drawScreenTitle(theme, "SETTINGS", "");
    drawContentPanel(theme, contentPanel(theme));
    m_focus.draw(theme, m_context.time);
    if (m_tabs.active() == SKINS_TAB && m_context.activeProfile() == nullptr) {
        const Rectangle panel = contentPanel(theme);
        theme.drawTextCentered(
            "Start or load a game to unlock and choose skins.",
            Vector2{panel.x + panel.width / 2.0f, panel.y + panel.height - theme.px(40.0f)},
            theme.px(render::textsize::SMALL), palette::TEXT_MUTED);
    }
    drawFooterHint(theme,
                   "LEFT / RIGHT adjust     ENTER toggle or select     ESC save and go back");
}

} // namespace ui
