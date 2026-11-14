#include "ui/ConfirmDialog.h"

#include "render/Palette.h"
#include "render/Primitives.h"

#include <utility>

namespace ui {

namespace palette = render::palette;

namespace {

constexpr float PANEL_WIDTH = 520.0f;
constexpr float PANEL_HEIGHT = 250.0f;
constexpr float BUTTON_WIDTH = 190.0f;
constexpr float BUTTON_HEIGHT = 50.0f;
constexpr float BUTTON_GAP = 24.0f;
constexpr float PADDING = 28.0f;

} // namespace

ConfirmDialog::ConfirmDialog() {
    m_focus.add(m_confirm);
    m_focus.add(m_cancel);
}

void ConfirmDialog::open(std::string title, std::string message, std::string confirmLabel,
                         std::string cancelLabel) {
    m_title = std::move(title);
    m_message = std::move(message);
    m_confirm.setLabel(std::move(confirmLabel));
    m_cancel.setLabel(std::move(cancelLabel));
    m_focus.focus(m_cancel);
    m_open = true;
}

Rectangle ConfirmDialog::panelBounds(const render::Theme& theme) const {
    const float width = theme.px(PANEL_WIDTH);
    const float height = theme.px(PANEL_HEIGHT);
    return Rectangle{(static_cast<float>(GetScreenWidth()) - width) / 2.0f,
                     (static_cast<float>(GetScreenHeight()) - height) / 2.0f, width, height};
}

void ConfirmDialog::layout(const render::Theme& theme) {
    const Rectangle panel = panelBounds(theme);
    const float buttonWidth = theme.px(BUTTON_WIDTH);
    const float buttonHeight = theme.px(BUTTON_HEIGHT);
    const float gap = theme.px(BUTTON_GAP);
    const float y = panel.y + panel.height - theme.px(PADDING) - buttonHeight;
    const float startX = panel.x + (panel.width - 2.0f * buttonWidth - gap) / 2.0f;
    m_confirm.setBounds(Rectangle{startX, y, buttonWidth, buttonHeight});
    m_cancel.setBounds(Rectangle{startX + buttonWidth + gap, y, buttonWidth, buttonHeight});
}

DialogResult ConfirmDialog::update(const render::Theme& theme, const app::InputState& input) {
    if (!m_open) {
        return DialogResult::None;
    }
    layout(theme);
    if (input.back) {
        m_open = false;
        return DialogResult::Cancelled;
    }
    const Widget* fired = m_focus.update(input);
    if (fired == &m_confirm) {
        m_open = false;
        return DialogResult::Confirmed;
    }
    if (fired == &m_cancel) {
        m_open = false;
        return DialogResult::Cancelled;
    }
    return DialogResult::None;
}

void ConfirmDialog::draw(const render::Theme& theme, float time) const {
    if (!m_open) {
        return;
    }
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), render::fade(palette::BG_DEEP, 0.75f));
    const Rectangle panel = panelBounds(theme);
    render::drawPanel(panel, theme.px(14.0f), palette::PANEL, palette::WALL_GLOW, theme.px(2.0f));
    const float padding = theme.px(PADDING);
    theme.drawTextCentered(m_title, Vector2{panel.x + panel.width / 2.0f, panel.y + padding * 1.3f},
                           theme.px(render::textsize::LARGE), palette::TEXT);
    theme.drawWrapped(m_message, Vector2{panel.x + padding, panel.y + padding * 2.6f},
                      panel.width - 2.0f * padding, theme.px(render::textsize::BODY),
                      palette::TEXT_MUTED);
    m_focus.draw(theme, time);
}

} // namespace ui
