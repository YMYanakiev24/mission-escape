#include "app/App.h"

#include "app/GameSession.h"
#include "app/Input.h"
#include "app/ScreenStack.h"
#include "app/ScreenshotTour.h"
#include "render/Palette.h"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace app {

namespace {

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int WINDOW_MIN_WIDTH = 960;
constexpr int WINDOW_MIN_HEIGHT = 540;
constexpr int TARGET_FPS = 60;
/// Longest frame step fed to the simulation, so a stall (e.g. dragging the window) can never
/// teleport the player or the chaser.
constexpr float MAX_FRAME_SECONDS = 0.1f;
constexpr const char* FONT_FILE = "assets/fonts/ChakraPetch-SemiBold.ttf";
constexpr const char* AUDIO_FOLDER = "assets/audio";

/// Opens the window in its constructor and closes it in its destructor (RAII), so every
/// object declared after it is destroyed while the OpenGL context still exists.
class WindowGuard {
public:
    WindowGuard() {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mission Escape");
        SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
        SetExitKey(KEY_NULL); // Esc opens the pause menu instead of closing the window
        SetTargetFPS(TARGET_FPS);
    }
    ~WindowGuard() { CloseWindow(); }
    WindowGuard(const WindowGuard&) = delete;
    WindowGuard& operator=(const WindowGuard&) = delete;
};

/// Opens the audio device for the lifetime of the object. The game keeps running silently if
/// no device is available (AudioManager checks IsAudioDeviceReady()).
class AudioDeviceGuard {
public:
    AudioDeviceGuard() { InitAudioDevice(); }
    ~AudioDeviceGuard() {
        if (IsAudioDeviceReady()) {
            CloseAudioDevice();
        }
    }
    AudioDeviceGuard(const AudioDeviceGuard&) = delete;
    AudioDeviceGuard& operator=(const AudioDeviceGuard&) = delete;
};

/// The ambient loop that fits what is on screen: the tier of the running level, else the menu.
audio::MusicId musicFor(const AppContext& context) {
    if (!context.session) {
        return audio::MusicId::Menu;
    }
    switch (context.session->level().spec().tier) {
        case core::Tier::Awakening:
            return audio::MusicId::Awakening;
        case core::Tier::Labyrinth:
            return audio::MusicId::Labyrinth;
        case core::Tier::Fog:
            return audio::MusicId::Fog;
        case core::Tier::Hunted:
            return audio::MusicId::Hunted;
    }
    return audio::MusicId::Menu;
}

void drawFpsCounter(const render::Theme& theme) {
    theme.drawText(
        "FPS " + std::to_string(GetFPS()),
        Vector2{theme.px(10.0f), static_cast<float>(GetScreenHeight()) - theme.px(26.0f)},
        theme.px(render::textsize::TINY), render::palette::ACCENT_GREEN);
}

/// Finds a bundled asset next to the executable, falling back to the working directory.
std::string assetPath(const std::string& relativePath) {
    const std::string besideExecutable = std::string(GetApplicationDirectory()) + relativePath;
    return FileExists(besideExecutable.c_str()) ? besideExecutable : relativePath;
}

} // namespace

LaunchOptions parseLaunchOptions(int argc, char* argv[]) {
    LaunchOptions options;
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--screenshots") {
            options.screenshotTour = true;
        }
    }
    return options;
}

App::App(const LaunchOptions& options) : m_options(options) {}

int App::run() {
    const WindowGuard window;
    const AudioDeviceGuard audioDevice;
    AppContext context(m_options, std::filesystem::path(GetApplicationDirectory()));
    context.loadPersistentData();
    context.applyDisplaySettings();
    context.theme.loadFont(assetPath(FONT_FILE));
    context.audio.initialize(assetPath(AUDIO_FOLDER));
    ScreenStack screens(context, ScreenId::Splash);
    std::unique_ptr<ScreenshotTour> tour;
    if (m_options.screenshotTour) {
        tour = std::make_unique<ScreenshotTour>(context, screens);
    }

    while (!WindowShouldClose() && !screens.quitRequested()) {
        const float deltaSeconds = std::min(GetFrameTime(), MAX_FRAME_SECONDS);
        context.time += deltaSeconds;
        context.theme.updateScale(GetScreenWidth(), GetScreenHeight());
        InputState input = pollInput();
        if (tour) {
            input = tour->beforeUpdate(input);
        }
        screens.update(deltaSeconds, input);
        context.audio.applySettings(context.settings);
        context.audio.playMusic(musicFor(context));
        context.audio.update(deltaSeconds);
        BeginDrawing();
        screens.draw();
        if (context.settings.showFps) {
            drawFpsCounter(context.theme);
        }
        if (tour) {
            tour->afterDraw();
        }
        EndDrawing();
        if (tour && tour->finished()) {
            break;
        }
    }
    return 0;
}

} // namespace app
