#include "app/ScreenStack.h"

#include "render/Palette.h"
#include "render/Primitives.h"
#include "ui/CaughtScreen.h"
#include "ui/GameCompleteScreen.h"
#include "ui/GameplayScreen.h"
#include "ui/HelpScreen.h"
#include "ui/LevelCompleteScreen.h"
#include "ui/LevelSelectScreen.h"
#include "ui/LoadGameScreen.h"
#include "ui/MainMenuScreen.h"
#include "ui/NewGameScreen.h"
#include "ui/SettingsScreen.h"
#include "ui/SplashScreen.h"

#include <raylib.h>

#include <algorithm>
#include <cstddef>

namespace app {

namespace {

constexpr float FADE_OUT_SECONDS = 0.18f;
constexpr float FADE_IN_SECONDS = 0.24f;

/// Temporary stand-in for screens that are not implemented yet; Esc returns.
class PlaceholderScreen : public ui::Screen {
public:
    explicit PlaceholderScreen(AppContext& context) : m_context(context) {}

    void update(float /*deltaSeconds*/, const InputState& input) override {
        if (input.back || input.confirm) {
            m_context.navigator.pop();
        }
    }

    void draw() override {
        ClearBackground(render::palette::BG_DEEP);
        m_context.theme.drawTextCentered("Coming soon - press Esc",
                                         Vector2{static_cast<float>(GetScreenWidth()) / 2.0f,
                                                 static_cast<float>(GetScreenHeight()) / 2.0f},
                                         m_context.theme.px(render::textsize::LARGE),
                                         render::palette::TEXT_MUTED);
    }

private:
    AppContext& m_context;
};

std::unique_ptr<ui::Screen> createScreen(ScreenId id, AppContext& context) {
    switch (id) {
        case ScreenId::Splash:
            return std::make_unique<ui::SplashScreen>(context);
        case ScreenId::MainMenu:
            return std::make_unique<ui::MainMenuScreen>(context);
        case ScreenId::NewGame:
            return std::make_unique<ui::NewGameScreen>(context);
        case ScreenId::LoadGame:
            return std::make_unique<ui::LoadGameScreen>(context);
        case ScreenId::LevelSelect:
            return std::make_unique<ui::LevelSelectScreen>(context);
        case ScreenId::Settings:
            return std::make_unique<ui::SettingsScreen>(context);
        case ScreenId::Help:
            return std::make_unique<ui::HelpScreen>(context);
        case ScreenId::Caught:
            return std::make_unique<ui::CaughtScreen>(context);
        case ScreenId::GameComplete:
            return std::make_unique<ui::GameCompleteScreen>(context);
        case ScreenId::Gameplay:
            return std::make_unique<ui::GameplayScreen>(context);
        case ScreenId::LevelComplete:
            return std::make_unique<ui::LevelCompleteScreen>(context);
        default:
            return std::make_unique<PlaceholderScreen>(context);
    }
}

} // namespace

ScreenStack::ScreenStack(AppContext& context, ScreenId initial) : m_context(context) {
    for (int index = 0; index < SCREEN_COUNT; ++index) {
        m_screens[static_cast<std::size_t>(index)] =
            createScreen(static_cast<ScreenId>(index), context);
    }
    m_stack.push_back(initial);
    screen(initial).onEnter();
}

ui::Screen& ScreenStack::screen(ScreenId id) {
    return *m_screens[static_cast<std::size_t>(id)];
}

void ScreenStack::update(float deltaSeconds, const InputState& input) {
    const bool acceptInput = m_phase == Phase::Idle;
    screen(top()).update(deltaSeconds, acceptInput ? input : emptyInput(input));
    switch (m_phase) {
        case Phase::Idle:
            startTransition();
            break;
        case Phase::FadingOut:
            m_fade = std::min(1.0f, m_fade + deltaSeconds / FADE_OUT_SECONDS);
            if (m_fade >= 1.0f && m_request) {
                applyRequest(*m_request);
                m_request.reset();
                m_phase = Phase::FadingIn;
            }
            break;
        case Phase::FadingIn:
            m_fade = std::max(0.0f, m_fade - deltaSeconds / FADE_IN_SECONDS);
            if (m_fade <= 0.0f) {
                m_phase = Phase::Idle;
            }
            break;
    }
}

void ScreenStack::startTransition() {
    std::optional<NavigationRequest> request = m_context.navigator.take();
    if (!request) {
        return;
    }
    if (request->kind == NavigationKind::Quit) {
        m_quit = true;
        return;
    }
    if (request->kind == NavigationKind::Pop && m_stack.size() <= 1) {
        return; // nothing to return to
    }
    m_request = request;
    m_phase = Phase::FadingOut;
}

void ScreenStack::applyRequest(const NavigationRequest& request) {
    m_context.applyPendingSessionChange();
    switch (request.kind) {
        case NavigationKind::Replace:
            m_stack.clear();
            m_stack.push_back(request.target);
            screen(request.target).onEnter();
            break;
        case NavigationKind::Push:
            m_stack.push_back(request.target);
            screen(request.target).onEnter();
            break;
        case NavigationKind::Pop:
            m_stack.pop_back();
            screen(top()).onResume();
            break;
        case NavigationKind::Quit:
            m_quit = true;
            break;
    }
}

void ScreenStack::draw() {
    // Find the lowest screen that is visible through the overlays stacked above it.
    std::size_t first = m_stack.size() - 1;
    while (first > 0 && screen(m_stack[first]).isOverlay()) {
        --first;
    }
    for (std::size_t index = first; index < m_stack.size(); ++index) {
        screen(m_stack[index]).draw();
    }
    if (m_fade > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      render::fade(render::palette::BG_DEEP, m_fade));
    }
}

} // namespace app
