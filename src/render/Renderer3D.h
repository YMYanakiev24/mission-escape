#pragma once

#include "core/Level.h"
#include "core/Progress.h"
#include "render/GpuResources.h"

#include <raylib.h>

#include <cstdint>

/// @file Renderer3D.h
/// First-person rendering of a level: same core state as 2D, different presentation.

namespace render {

/// Draws a level from the player's eyes.
///
/// The walls are built once per level into a single mesh (one draw call for the whole maze),
/// floor and ceiling are textured planes, and a small GLSL shader adds distance fog (the
/// renderer falls back to the default shader if it cannot be compiled). Perks, the exit portal,
/// secret doors and the chaser are drawn as simple animated shapes on top.
class Renderer3D {
public:
    /// Builds the static geometry for @p level (call when a level starts or restarts).
    void beginLevel(const core::Level& level);

    /// Moves the camera with the tweened player and smooths 90 degree turns.
    void update(float deltaSeconds, const core::Level& level, bool headBob);

    /// Draws the 3D scene (clears the screen first).
    void draw(const core::Level& level, float time);

    /// Draws the light / skin the player "holds" at the bottom of the screen.
    void drawHandLight(core::SkinId skin, const core::Level& level, float time) const;

    /// Offsets the camera this frame (shake when caught), in world units.
    void setShake(Vector2 offset) noexcept { m_shake = offset; }

    /// Displayed view direction in radians (0 = north, clockwise).
    float yaw() const noexcept { return m_yaw; }

private:
    void ensureShader();
    void buildTextures(core::Tier tier);
    void updateFog(const core::Level& level);
    void applyFogUniforms() const;
    Color fogged(Color color, Vector3 position) const;
    void drawSecretDoors(const core::Level& level, float time) const;
    void drawExit(const core::Level& level, float time) const;
    void drawPerks(const core::Level& level, float time) const;
    void drawPerkShape(core::PerkType type, Vector3 position, float spin) const;
    void drawChaser(const core::Level& level, float time) const;

    ShaderResource m_fogShader;
    int m_viewPosLocation = -1;
    int m_fogColorLocation = -1;
    int m_fogNearLocation = -1;
    int m_fogFarLocation = -1;
    bool m_shaderTried = false;
    ModelResource m_walls;
    ModelResource m_floor;
    ModelResource m_ceiling;
    TextureResource m_floorTexture;
    TextureResource m_ceilingTexture;
    TextureResource m_wallTexture;
    Camera3D m_camera{};
    float m_yaw = 0.0f;
    bool m_cameraReady = false;
    float m_fogNear = 1.0f;
    float m_fogFar = 30.0f;
    Color m_fogColor{};
    Vector2 m_shake{};
};

} // namespace render
