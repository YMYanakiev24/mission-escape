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
# Business logic layer (src/business): the game rules, no raylib and no files
# ---------------------------------------------------------------------------
add_library(maze_business STATIC
    src/business/core/Cell.cpp
    src/business/core/Chaser.cpp
    src/business/core/Fog.cpp
    src/business/core/Level.cpp
    src/business/core/LevelRules.cpp
    src/business/core/Maze.cpp
    src/business/core/MazeGenerator.cpp
    src/business/core/Pathfinding.cpp
    src/business/core/Perks.cpp
    src/business/core/Player.cpp
    src/business/core/Progress.cpp
    src/business/core/Random.cpp
    src/business/core/Scoring.cpp
    src/business/core/Settings.cpp
    src/business/core/TextView.cpp
    src/business/core/Validation.cpp
)
target_include_directories(maze_business PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/business)
mission_escape_enable_warnings(maze_business)

# ---------------------------------------------------------------------------
# Data access layer (src/data): save slots and settings as key=value text files
# ---------------------------------------------------------------------------
add_library(maze_data STATIC
    src/data/io/KeyValueDocument.cpp
    src/data/io/SaveSystem.cpp
    src/data/io/SettingsStore.cpp
)
target_include_directories(maze_data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/data)
target_link_libraries(maze_data PUBLIC maze_business)
mission_escape_enable_warnings(maze_data)

# ---------------------------------------------------------------------------
# Presentation layer (src/presentation): window, input, screens, rendering, audio
# ---------------------------------------------------------------------------
add_executable(maze_game
    src/main.cpp
    src/presentation/app/App.cpp
    src/presentation/app/AppContext.cpp
    src/presentation/app/FeedbackPresenter.cpp
    src/presentation/app/GameSession.cpp
    src/presentation/app/Input.cpp
    src/presentation/app/ScreenStack.cpp
    src/presentation/app/ScreenshotTour.cpp
    src/presentation/audio/AudioManager.cpp
    src/presentation/audio/Synth.cpp
    src/presentation/render/Effects.cpp
    src/presentation/render/Hud.cpp
    src/presentation/render/MazeMesh.cpp
    src/presentation/render/MenuBackground.cpp
    src/presentation/render/Minimap.cpp
    src/presentation/render/PerkIcons.cpp
    src/presentation/render/Primitives.cpp
    src/presentation/render/Renderer2D.cpp
    src/presentation/render/Renderer3D.cpp
    src/presentation/render/SkinRenderer.cpp
    src/presentation/render/TextViewRenderer.cpp
    src/presentation/render/Theme.cpp
    src/presentation/render/WallRuns.cpp
    src/presentation/ui/CaughtScreen.cpp
    src/presentation/ui/ConfirmDialog.cpp
    src/presentation/ui/GameCompleteScreen.cpp
    src/presentation/ui/GameplayScreen.cpp
    src/presentation/ui/HelpScreen.cpp
    src/presentation/ui/LevelCompleteScreen.cpp
    src/presentation/ui/LevelSelectScreen.cpp
    src/presentation/ui/LoadGameScreen.cpp
    src/presentation/ui/MainMenuScreen.cpp
    src/presentation/ui/MenuStyle.cpp
    src/presentation/ui/NewGameScreen.cpp
    src/presentation/ui/SettingsScreen.cpp
    src/presentation/ui/SplashScreen.cpp
    src/presentation/ui/Widgets.cpp
)
target_include_directories(maze_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation)
target_link_libraries(maze_game PRIVATE maze_business maze_data raylib)
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
        src/presentation/audio/Synth.cpp
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
    # Synth.cpp is pure (no raylib), so the tests compile it from the presentation layer.
    target_include_directories(maze_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation)
    target_link_libraries(maze_tests PRIVATE maze_business maze_data)
    mission_escape_enable_warnings(maze_tests)

    set(MISSION_ESCAPE_TEST_SUITES
        grid generation validation movement pathfinding textview stars playthrough unlocks
        settings saves fog chaser tiers perks audio)
    foreach(suite IN LISTS MISSION_ESCAPE_TEST_SUITES)
        add_test(NAME ${suite} COMMAND maze_tests --suite ${suite})
    endforeach()
endif()
