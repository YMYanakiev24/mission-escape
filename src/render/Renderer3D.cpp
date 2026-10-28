#include "render/Renderer3D.h"

#include "core/Config.h"
#include "render/MazeMesh.h"
#include "render/Palette.h"
#include "render/PerkIcons.h"
#include "render/Primitives.h"
#include "render/SkinRenderer.h"
#include "render/TierStyle.h"
#include "render/WallRuns.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>

namespace render {

namespace {

constexpr float FOV_DEGREES = 75.0f;
constexpr float TURN_RATE = 14.0f;
constexpr float HEAD_BOB_HEIGHT = 0.045f;
constexpr float CLEAR_FOG_NEAR = 7.0f;
constexpr float CLEAR_FOG_FAR = 38.0f;
constexpr float FOG_LEVEL_NEAR_CELLS = 0.4f;
constexpr float FOG_LEVEL_FAR_PADDING = 0.8f;
constexpr int TEXTURE_SIZE = 64;
constexpr int GRID_LINE_PIXELS = 2;
constexpr float PHASE_WALL_ALPHA = 0.35f;
constexpr float PERK_HEIGHT = 0.75f;
constexpr float PERK_SIZE = 0.28f;
constexpr float CHASER_HEIGHT = 0.95f;
constexpr float CHASER_RADIUS = 0.33f;
constexpr int CHASER_SPIKES = 8;
constexpr float DOOR_THICKNESS = 0.06f;
constexpr float EXIT_CORE_HEIGHT = 0.9f;
constexpr int CYLINDER_SLICES = 20;
constexpr float TWO_PI = 6.28318530718f;

/// Vertex shader: passes the world position on so the fragment shader can measure distance.
constexpr const char* FOG_VERTEX_SHADER = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragWorld;
void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragWorld = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

/// Fragment shader: linear distance fog towards the background colour.
constexpr const char* FOG_FRAGMENT_SHADER = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragWorld;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform vec4 fogColor;
uniform float fogNear;
uniform float fogFar;
out vec4 finalColor;
void main()
{
    vec4 texel = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    float distanceToEye = length(viewPos - fragWorld);
    float fogAmount = clamp((distanceToEye - fogNear) / (fogFar - fogNear), 0.0, 1.0);
    finalColor = vec4(mix(texel.rgb, fogColor.rgb, fogAmount), texel.a);
}
)";

float wrapAngle(float angle) {
    while (angle > PI) {
        angle -= TWO_PI;
    }
    while (angle <= -PI) {
        angle += TWO_PI;
    }
    return angle;
}

/// View angle of a compass direction (0 = north / -z, clockwise seen from above).
float facingYaw(core::Direction direction) {
    switch (direction) {
        case core::Direction::North:
            return 0.0f;
        case core::Direction::East:
            return PI / 2.0f;
        case core::Direction::South:
            return PI;
        case core::Direction::West:
            return -PI / 2.0f;
    }
    return 0.0f;
}

Vector3 add(Vector3 a, Vector3 b) {
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 scaled(Vector3 vector, float factor) {
    return Vector3{vector.x * factor, vector.y * factor, vector.z * factor};
}

TextureResource makeRepeatingTexture(Image image) {
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    return TextureResource(texture);
}

/// A tile with grid lines on two edges, so repeating it draws one square per cell.
Image gridTile(Color fill, Color line) {
    Image image = GenImageColor(TEXTURE_SIZE, TEXTURE_SIZE, fill);
    ImageDrawRectangle(&image, 0, 0, TEXTURE_SIZE, GRID_LINE_PIXELS, line);
    ImageDrawRectangle(&image, 0, 0, GRID_LINE_PIXELS, TEXTURE_SIZE, line);
    return image;
}

/// A white wall panel with darker seams; multiplied by the vertex colours it gives the walls
/// one visible panel per cell, which makes distances and movement much easier to judge.
Image wallPanelTile() {
    Image image = GenImageColor(TEXTURE_SIZE, TEXTURE_SIZE, WHITE);
    const Color seam{120, 120, 140, 255};
    const Color bevel{215, 215, 230, 255};
    ImageDrawRectangle(&image, 0, 0, 3, TEXTURE_SIZE, seam);
    ImageDrawRectangle(&image, 3, 0, 2, TEXTURE_SIZE, bevel);
    ImageDrawRectangle(&image, 0, TEXTURE_SIZE / 2, TEXTURE_SIZE, 1, bevel);
    return image;
}

// The perk shapes below are drawn around the origin; the caller sets up the transform.

void drawBerryShape(Color berry, Color darkBerry, Color leaf) {
    for (int index = 0; index < 3; ++index) {
        const float angle = static_cast<float>(index) * TWO_PI / 3.0f;
        DrawSphere(Vector3{std::cos(angle) * 0.11f, -0.04f, std::sin(angle) * 0.11f}, 0.1f,
                   index == 0 ? darkBerry : berry);
    }
    DrawCube(Vector3{0.0f, 0.12f, 0.0f}, 0.05f, 0.1f, 0.05f, leaf);
}

void drawHourglassShape(Color glass, Color frame) {
    DrawCylinderEx(Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.26f, 0.0f}, 0.0f, 0.2f, 10, glass);
    DrawCylinderEx(Vector3{0.0f, -0.26f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.2f, 0.0f, 10, glass);
    DrawCylinder(Vector3{0.0f, 0.26f, 0.0f}, 0.24f, 0.24f, 0.03f, 10, frame);
    DrawCylinder(Vector3{0.0f, -0.29f, 0.0f}, 0.24f, 0.24f, 0.03f, 10, frame);
}

void drawCompassShape(Color core, Color outline) {
    rlRotatef(45.0f, 1.0f, 0.0f, 1.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, PERK_SIZE, PERK_SIZE, PERK_SIZE, core);
    const float wire = PERK_SIZE * 1.15f;
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, wire, wire, wire, outline);
}

void drawLanternShape(Color body, Color flame) {
    DrawCylinder(Vector3{0.0f, -0.2f, 0.0f}, 0.12f, 0.14f, 0.34f, 8, body);
    DrawSphere(Vector3{0.0f, -0.03f, 0.0f}, 0.1f, flame);
}

void drawFrostShape(Color crystal, Color edge) {
    rlRotatef(25.0f, 0.0f, 0.0f, 1.0f);
    DrawCubeV(Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.12f, 0.46f, 0.12f}, crystal);
    DrawCubeWiresV(Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.14f, 0.48f, 0.14f}, edge);
}

void assignMaterial(Model& model, const Texture2D* texture, const ShaderResource& shader) {
    if (texture != nullptr) {
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *texture;
    }
    if (shader.isValid()) {
        model.materials[0].shader = shader.get();
    }
}

} // namespace

void Renderer3D::ensureShader() {
    if (m_shaderTried) {
        return;
    }
    m_shaderTried = true;
    m_fogShader = ShaderResource(LoadShaderFromMemory(FOG_VERTEX_SHADER, FOG_FRAGMENT_SHADER));
    if (!m_fogShader.isValid()) {
        return; // without the shader the scene still renders, just without distance fog
    }
    const Shader& shader = m_fogShader.get();
    m_viewPosLocation = GetShaderLocation(shader, "viewPos");
    m_fogColorLocation = GetShaderLocation(shader, "fogColor");
    m_fogNearLocation = GetShaderLocation(shader, "fogNear");
    m_fogFarLocation = GetShaderLocation(shader, "fogFar");
}

void Renderer3D::buildTextures(core::Tier tier) {
    const TierStyle style = tierStyle(tier);
    const Color floorFill = mix(style.floor, palette::PANEL, 0.3f);
    m_floorTexture = makeRepeatingTexture(gridTile(floorFill, mix(floorFill, style.accent, 0.35f)));
    m_ceilingTexture = makeRepeatingTexture(
        gridTile(palette::BG_DEEP, mix(palette::BG_DEEP, palette::WALL, 0.3f)));
    m_wallTexture = makeRepeatingTexture(wallPanelTile());
}

void Renderer3D::beginLevel(const core::Level& level) {
    ensureShader();
    buildTextures(level.spec().tier);
    const TierStyle style = tierStyle(level.spec().tier);
    const WallMeshColors colors{mix(palette::BG_DEEP, palette::WALL, 0.25f),
                                mix(palette::WALL, palette::BG, 0.3f),
                                mix(palette::WALL_GLOW, palette::TEXT, 0.15f),
                                mix(style.accent, palette::WALL_GLOW, 0.4f), palette::WALL_GLOW};
    m_walls =
        ModelResource(LoadModelFromMesh(buildWallMesh(computeWallRuns(level.maze()), colors)));
    const auto cells = static_cast<float>(level.maze().size());
    const float extent = cells * world3d::CELL_SIZE;
    m_floor = ModelResource(LoadModelFromMesh(buildPlaneMesh(extent, 0.0f, cells, true)));
    m_ceiling = ModelResource(
        LoadModelFromMesh(buildPlaneMesh(extent, world3d::WALL_HEIGHT, cells, false)));
    assignMaterial(m_walls.get(), &m_wallTexture.get(), m_fogShader);
    assignMaterial(m_floor.get(), &m_floorTexture.get(), m_fogShader);
    assignMaterial(m_ceiling.get(), &m_ceilingTexture.get(), m_fogShader);
    m_cameraReady = false;
}

void Renderer3D::update(float deltaSeconds, const core::Level& level, bool headBob) {
    const float target = facingYaw(level.player().facing());
    if (!m_cameraReady) {
        m_yaw = target;
    } else {
        m_yaw =
            wrapAngle(m_yaw + wrapAngle(target - m_yaw) * std::min(1.0f, deltaSeconds * TURN_RATE));
    }
    const core::Player& player = level.player();
    float bob = 0.0f;
    if (headBob && player.isMoving()) {
        bob = std::sin(player.moveProgress() * PI) * HEAD_BOB_HEIGHT;
    }
    Vector3 eye = cellToWorld(player.visualPosition(), world3d::EYE_HEIGHT + bob + m_shake.y);
    eye.x += std::cos(m_yaw) * m_shake.x; // sideways relative to the view direction
    eye.z += std::sin(m_yaw) * m_shake.x;
    m_camera.position = eye;
    m_camera.target = Vector3{eye.x + std::sin(m_yaw), eye.y, eye.z - std::cos(m_yaw)};
    m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
    m_camera.fovy = FOV_DEGREES;
    m_camera.projection = CAMERA_PERSPECTIVE;
    m_cameraReady = true;
    updateFog(level);
}

void Renderer3D::updateFog(const core::Level& level) {
    const TierStyle style = tierStyle(level.spec().tier);
    if (level.spec().hasFog) {
        m_fogNear = FOG_LEVEL_NEAR_CELLS * world3d::CELL_SIZE;
        m_fogFar =
            (static_cast<float>(level.fogRadius()) + FOG_LEVEL_FAR_PADDING) * world3d::CELL_SIZE;
        m_fogColor = style.fog;
    } else {
        m_fogNear = CLEAR_FOG_NEAR;
        m_fogFar = CLEAR_FOG_FAR;
        m_fogColor = palette::BG_DEEP;
    }
}

void Renderer3D::applyFogUniforms() const {
    if (!m_fogShader.isValid()) {
        return;
    }
    const Shader& shader = m_fogShader.get();
    const float viewPosition[3] = {m_camera.position.x, m_camera.position.y, m_camera.position.z};
    const float color[4] = {static_cast<float>(m_fogColor.r) / 255.0f,
                            static_cast<float>(m_fogColor.g) / 255.0f,
                            static_cast<float>(m_fogColor.b) / 255.0f, 1.0f};
    SetShaderValue(shader, m_viewPosLocation, viewPosition, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, m_fogColorLocation, color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, m_fogNearLocation, &m_fogNear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, m_fogFarLocation, &m_fogFar, SHADER_UNIFORM_FLOAT);
}

Color Renderer3D::fogged(Color color, Vector3 position) const {
    // Immediate-mode shapes do not go through the fog shader, so fade them on the CPU.
    const float distance =
        std::hypot(position.x - m_camera.position.x, position.y - m_camera.position.y,
                   position.z - m_camera.position.z);
    const float amount = std::clamp((distance - m_fogNear) / (m_fogFar - m_fogNear), 0.0f, 1.0f);
    const Color target{m_fogColor.r, m_fogColor.g, m_fogColor.b, color.a};
    return mix(color, target, amount);
}

void Renderer3D::draw(const core::Level& level, float time) {
    ClearBackground(m_fogColor);
    if (!m_walls.isValid()) {
        return;
    }
    BeginMode3D(m_camera);
    applyFogUniforms();
    rlDisableBackfaceCulling();
    const Vector3 origin{0.0f, 0.0f, 0.0f};
    DrawModel(m_floor.get(), origin, 1.0f, WHITE);
    DrawModel(m_ceiling.get(), origin, 1.0f, WHITE);
    const bool phasing = level.isPhasing();
    if (!phasing) {
        DrawModel(m_walls.get(), origin, 1.0f, WHITE);
    }
    drawExit(level, time);
    drawPerks(level, time);
    drawChaser(level, time);
    drawSecretDoors(level, time);
    if (phasing) {
        // Translucent walls are drawn last and do not write depth, so the scene shows through.
        rlDisableDepthMask();
        DrawModel(m_walls.get(), origin, 1.0f, Fade(WHITE, PHASE_WALL_ALPHA));
        rlEnableDepthMask();
    }
    rlEnableBackfaceCulling();
    EndMode3D();
}

void Renderer3D::drawSecretDoors(const core::Level& level, float time) const {
    const float glow = 0.6f + 0.4f * pulse(time, 0.8f);
    for (const core::SecretDoor& door : level.secretDoors()) {
        const core::Cell other = core::neighbour(door.cell, door.side);
        if (level.spec().hasFog && !level.fog().isExplored(door.cell) &&
            !level.fog().isExplored(other)) {
            continue;
        }
        const float height = world3d::WALL_HEIGHT / 2.0f;
        const Vector3 a = cellToWorld(
            core::Vec2f{static_cast<float>(door.cell.x), static_cast<float>(door.cell.y)}, height);
        const Vector3 b = cellToWorld(
            core::Vec2f{static_cast<float>(other.x), static_cast<float>(other.y)}, height);
        const Vector3 center{(a.x + b.x) / 2.0f, height, (a.z + b.z) / 2.0f};
        const bool alongZ =
            door.side == core::Direction::East || door.side == core::Direction::West;
        const Vector3 size =
            alongZ ? Vector3{DOOR_THICKNESS, world3d::WALL_HEIGHT * 0.96f, world3d::CELL_SIZE}
                   : Vector3{world3d::CELL_SIZE, world3d::WALL_HEIGHT * 0.96f, DOOR_THICKNESS};
        DrawCubeV(center, size, fade(fogged(palette::ACCENT_GREEN_DIM, center), 0.55f));
        DrawCubeWiresV(center, size, fogged(palette::ACCENT_GREEN, center));
        // The rune (a diamond with a vertical stroke) on both faces of the panel.
        const Vector3 normal = alongZ ? Vector3{1.0f, 0.0f, 0.0f} : Vector3{0.0f, 0.0f, 1.0f};
        const Vector3 across = alongZ ? Vector3{0.0f, 0.0f, 1.0f} : Vector3{1.0f, 0.0f, 0.0f};
        const Color rune = fade(fogged(palette::ACCENT_GREEN, center), glow);
        for (const float face : {-1.0f, 1.0f}) {
            const Vector3 middle =
                add(center, scaled(normal, face * (DOOR_THICKNESS / 2.0f + 0.01f)));
            const Vector3 top = add(middle, Vector3{0.0f, 0.32f, 0.0f});
            const Vector3 bottom = add(middle, Vector3{0.0f, -0.32f, 0.0f});
            const Vector3 left = add(middle, scaled(across, -0.32f));
            const Vector3 right = add(middle, scaled(across, 0.32f));
            DrawLine3D(top, right, rune);
            DrawLine3D(right, bottom, rune);
            DrawLine3D(bottom, left, rune);
            DrawLine3D(left, top, rune);
            DrawLine3D(add(top, Vector3{0.0f, 0.2f, 0.0f}), add(bottom, Vector3{0.0f, -0.2f, 0.0f}),
                       fade(palette::TEXT, glow));
        }
    }
}

void Renderer3D::drawExit(const core::Level& level, float time) const {
    const core::Cell exit = level.maze().exit();
    const Vector3 base =
        cellToWorld(core::Vec2f{static_cast<float>(exit.x), static_cast<float>(exit.y)}, 0.0f);
    const Vector3 core{base.x, EXIT_CORE_HEIGHT, base.z};
    const float beat = pulse(time, 0.9f);
    const bool hintOnly = level.spec().hasFog && !level.isExitSeen();
    // Until a fog level's exit has been seen it is only a faint beacon that ignores the fog.
    const Color green = hintOnly ? fade(palette::ACCENT_GREEN, 0.25f + 0.2f * beat)
                                 : fogged(palette::ACCENT_GREEN, core);
    DrawCylinder(base, 0.55f, 0.55f, world3d::WALL_HEIGHT, CYLINDER_SLICES,
                 fade(green, 0.16f + 0.08f * beat));
    if (hintOnly) {
        return;
    }
    for (int ring = 0; ring < 3; ++ring) {
        DrawCircle3D(core, 0.5f + 0.06f * static_cast<float>(ring), Vector3{0.0f, 1.0f, 0.0f},
                     time * 90.0f + static_cast<float>(ring) * 60.0f, green);
    }
    DrawCircle3D(Vector3{base.x, 0.03f, base.z}, 0.72f, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, green);
    DrawSphere(core, 0.12f + 0.03f * beat, fogged(palette::TEXT, core));
}

void Renderer3D::drawPerkShape(core::PerkType type, Vector3 position, float spin) const {
    const Color color = fogged(perkColor(type), position);
    const Color light = fogged(palette::TEXT, position);
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(spin, 0.0f, 1.0f, 0.0f);
    switch (type) {
        case core::PerkType::MagicBerry:
            drawBerryShape(color, fogged(palette::WALL, position),
                           fogged(palette::ACCENT_GREEN, position));
            break;
        case core::PerkType::Hourglass:
            drawHourglassShape(color, light);
            break;
        case core::PerkType::CompassSpark:
            drawCompassShape(color, light);
            break;
        case core::PerkType::Lantern:
            drawLanternShape(fogged(palette::ACCENT_GREEN_DIM, position), light);
            break;
        case core::PerkType::FrostShard:
            drawFrostShape(color, light);
            break;
    }
    rlPopMatrix();
}

void Renderer3D::drawPerks(const core::Level& level, float time) const {
    const bool fogLevel = level.spec().hasFog;
    for (const core::PerkItem& perk : level.perks()) {
        if (perk.collected || (fogLevel && !level.fog().isExplored(perk.cell))) {
            continue;
        }
        const float phase = time * 2.0f + static_cast<float>(perk.id);
        const Vector3 position = cellToWorld(
            core::Vec2f{static_cast<float>(perk.cell.x), static_cast<float>(perk.cell.y)},
            PERK_HEIGHT + std::sin(phase) * 0.08f);
        drawPerkShape(perk.type, position, time * 80.0f + static_cast<float>(perk.id) * 40.0f);
        DrawSphere(position, PERK_SIZE * 1.5f, fade(fogged(perkColor(perk.type), position), 0.16f));
    }
}

void Renderer3D::drawChaser(const core::Level& level, float time) const {
    const core::Chaser& chaser = level.chaser();
    const bool hidden = level.spec().hasFog && !level.fog().isVisible(chaser.cell());
    if (!chaser.isActive() || hidden) {
        return;
    }
    const bool frozen = level.effects().isActive(core::EffectType::ChaserFreeze);
    const float motion = frozen ? 0.0f : time;
    const Vector3 position =
        cellToWorld(chaser.visualPosition(), CHASER_HEIGHT + std::sin(motion * 5.0f) * 0.08f);
    const Color base = frozen ? mix(palette::DANGER, palette::ACCENT_BLUE, 0.75f) : palette::DANGER;
    const Color color = fogged(base, position);
    for (int spike = 0; spike < CHASER_SPIKES; ++spike) {
        const float angle = motion * 2.0f + static_cast<float>(spike) * TWO_PI / CHASER_SPIKES;
        const float tilt = (spike % 2 == 0) ? 0.45f : -0.45f;
        const Vector3 direction{std::cos(angle), tilt, std::sin(angle)};
        DrawCylinderEx(position, add(position, scaled(direction, CHASER_RADIUS * 1.9f)),
                       CHASER_RADIUS * 0.28f, 0.0f, 6, color);
    }
    DrawSphere(position, CHASER_RADIUS, color);
    DrawSphere(position, CHASER_RADIUS * 1.8f, fade(color, 0.18f + 0.1f * pulse(time, 2.0f)));
}

void Renderer3D::drawHandLight(core::SkinId skin, const core::Level& level, float time) const {
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const Color color = skinColor(skin);
    drawGlow(Vector2{width / 2.0f, height * 1.02f}, height * 0.34f, fade(color, 0.28f));
    const core::Player& player = level.player();
    const float sway = player.isMoving()
                           ? std::sin(player.moveProgress() * TWO_PI) * height * 0.006f
                           : std::sin(time * 2.0f) * height * 0.004f;
    drawSkin(skin, Vector2{width / 2.0f, height * 0.9f + sway}, height * 0.038f, time, -PI / 2.0f);
}

} // namespace render
