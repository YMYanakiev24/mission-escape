#pragma once

#include "app/AppContext.h"

/// @file App.h
/// Application entry object: owns the window, the screen state machine and the main loop.

namespace app {

/// Parses command line arguments; unknown arguments are ignored.
LaunchOptions parseLaunchOptions(int argc, char* argv[]);

/// Owns the window and drives the screen state machine at a fixed target frame rate.
class App {
public:
    /// Stores the launch options and the storage the business layer should use; the window is
    /// opened in run().
    App(const LaunchOptions& options, core::SaveRepository& saves,
        core::SettingsRepository& settingsStore);

    /// Opens the window, runs the main loop until the user quits and returns the exit code.
    int run();

private:
    LaunchOptions m_options;
    core::SaveRepository& m_saves;
    core::SettingsRepository& m_settingsStore;
};

} // namespace app
