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
    /// Stores the launch options; the window is opened in run().
    explicit App(const LaunchOptions& options);

    /// Opens the window, runs the main loop until the user quits and returns the exit code.
    int run();

private:
    LaunchOptions m_options;
};

} // namespace app
