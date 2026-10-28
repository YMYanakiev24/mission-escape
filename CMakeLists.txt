cmake_minimum_required(VERSION 3.20)

project(MissionEscape
    VERSION 1.0.0
    DESCRIPTION "Mission Escape - a C++17 / raylib maze game"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MISSION_ESCAPE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors for project targets" ON)
option(MISSION_ESCAPE_BUILD_TESTS "Build the unit test executable" ON)

# ---------------------------------------------------------------------------
# raylib (pinned stable release, fetched at configure time)
# ---------------------------------------------------------------------------
include(FetchContent)
set(MISSION_ESCAPE_RAYLIB_VERSION "6.2")
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${MISSION_ESCAPE_RAYLIB_VERSION}.tar.gz
    URL_HASH SHA256=df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Treat raylib headers as system headers so our strict warning flags never apply to them.
get_target_property(MISSION_ESCAPE_RAYLIB_INCLUDES raylib INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(raylib PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${MISSION_ESCAPE_RAYLIB_INCLUDES}")

# ---------------------------------------------------------------------------
# Strict warnings, applied only to our own targets
# ---------------------------------------------------------------------------
function(mission_escape_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(MISSION_ESCAPE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
        if(MISSION_ESCAPE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# maze_core: pure C++ game logic and persistence (no raylib, no iostream)
# ---------------------------------------------------------------------------
add_library(maze_core STATIC
    src/core/Cell.cpp
    src/core/Chaser.cpp
    src/core/Fog.cpp
    src/core/Level.cpp
    src/core/LevelRules.cpp
    src/core/Maze.cpp
    src/core/MazeGenerator.cpp
    src/core/Pathfinding.cpp
    src/core/Perks.cpp
    src/core/Player.cpp
    src/core/Progress.cpp
    src/core/Random.cpp
    src/core/Scoring.cpp
    src/core/Settings.cpp
    src/core/TextView.cpp
    src/core/Validation.cpp
    src/io/KeyValueDocument.cpp
    src/io/SaveSystem.cpp
    src/io/SettingsStore.cpp
)
target_include_directories(maze_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
mission_escape_enable_warnings(maze_core)

# ---------------------------------------------------------------------------
# maze_game: the executable (rendering, UI, audio, app state machine)
# ---------------------------------------------------------------------------
add_executable(maze_game
    src/main.cpp
    src/app/App.cpp
    src/audio/AudioManager.cpp
    src/audio/Synth.cpp
    src/app/AppContext.cpp
    src/app/FeedbackPresenter.cpp
    src/app/GameSession.cpp
    src/app/Input.cpp
    src/app/ScreenStack.cpp
    src/app/ScreenshotTour.cpp
    src/render/Effects.cpp
    src/render/Hud.cpp
    src/render/MenuBackground.cpp
    src/render/PerkIcons.cpp
    src/render/Primitives.cpp
    src/render/MazeMesh.cpp
    src/render/Minimap.cpp
    src/render/Renderer2D.cpp
    src/render/Renderer3D.cpp
    src/render/SkinRenderer.cpp
    src/render/TextViewRenderer.cpp
    src/render/Theme.cpp
    src/render/WallRuns.cpp
    src/ui/CaughtScreen.cpp
    src/ui/ConfirmDialog.cpp
    src/ui/GameCompleteScreen.cpp
    src/ui/GameplayScreen.cpp
    src/ui/HelpScreen.cpp
    src/ui/LevelCompleteScreen.cpp
    src/ui/LevelSelectScreen.cpp
    src/ui/LoadGameScreen.cpp
    src/ui/MainMenuScreen.cpp
    src/ui/MenuStyle.cpp
    src/ui/NewGameScreen.cpp
    src/ui/SettingsScreen.cpp
    src/ui/SplashScreen.cpp
    src/ui/Widgets.cpp
)
target_link_libraries(maze_game PRIVATE maze_core raylib)
mission_escape_enable_warnings(maze_game)

if(MSVC)
    # Release builds are windowed applications without a console.
    target_link_options(maze_game PRIVATE
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>)
endif()

# Copy runtime assets next to the executable so the game finds them when launched from the IDE.
add_custom_command(TARGET maze_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:maze_game>/assets
    COMMENT "Copying assets next to maze_game")

# ---------------------------------------------------------------------------
# maze_tests: self-written test harness, one CTest entry per suite
# ---------------------------------------------------------------------------
if(MISSION_ESCAPE_BUILD_TESTS)
    enable_testing()
    add_executable(maze_tests
        tests/test_main.cpp
        tests/test_audio.cpp
        src/audio/Synth.cpp
        tests/test_chaser.cpp
        tests/test_fog.cpp
        tests/test_generation.cpp
        tests/test_tiers.cpp
        tests/test_grid2d.cpp
        tests/test_level.cpp
        tests/test_movement.cpp
        tests/test_pathfinding.cpp
        tests/test_perks.cpp
        tests/test_progress.cpp
        tests/test_saves.cpp
        tests/test_scoring.cpp
        tests/test_textview.cpp
        tests/test_validation.cpp
    )
    target_link_libraries(maze_tests PRIVATE maze_core)
    mission_escape_enable_warnings(maze_tests)

    set(MISSION_ESCAPE_TEST_SUITES
        grid generation validation movement pathfinding textview stars playthrough unlocks
        settings saves fog chaser tiers perks audio)
    foreach(suite IN LISTS MISSION_ESCAPE_TEST_SUITES)
        add_test(NAME ${suite} COMMAND maze_tests --suite ${suite})
    endforeach()
endif()
