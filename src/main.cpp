#include "app/App.h"
#include "io/SaveSystem.h"
#include "io/SettingsStore.h"

#include <raylib.h>

#include <filesystem>

/// Composition root: the only file that sees all three layers. It creates the data access
/// layer's file storage and hands it to the presentation layer, which reaches it only through
/// the business layer's repository interfaces (core::SaveRepository, core::SettingsRepository).
int main(int argc, char* argv[]) {
    const std::filesystem::path dataDirectory(GetApplicationDirectory());
    io::SaveSystem saves(dataDirectory / "saves");
    io::SettingsStore settingsStore(dataDirectory / "settings.cfg");
    app::App application(app::parseLaunchOptions(argc, argv), saves, settingsStore);
    return application.run();
}
