#include "render/MazeMesh.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace render {

namespace {

constexpr float FLOOR_BAND_HEIGHT = 0.05f;
constexpr float GLOW_BAND_HEIGHT = 0.12f;
constexpr float SIDE_FACE_SHADE = 0.8f; ///< Faces along X are darker, so corners read clearly.

Color shade(Color color, float factor) {
    return Color{static_cast<unsigned char>(static_cast<float>(color.r) * factor),
                 static_cast<unsigned char>(static_cast<float>(color.g) * factor),
                 static_cast<unsigned char>(static_cast<float>(color.b) * factor), color.a};
}

/// Collects triangles with positions, texture coordinates, normals and colours.
class MeshBuilder {
public:
    /// Adds a quad given counter-clockwise corners; @p lower colours a and b, @p upper c and d.
    /// The texture spans @p uSpan tiles horizontally and [vTop, vBottom] vertically.
    void addQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 normal, Color lower,
                 Color upper, float uSpan = 1.0f, float vBottom = 1.0f, float vTop = 0.0f) {
        addVertex(a, Vector2{0.0f, vBottom}, normal, lower);
        addVertex(b, Vector2{uSpan, vBottom}, normal, lower);
        addVertex(c, Vector2{uSpan, vTop}, normal, upper);
        addVertex(a, Vector2{0.0f, vBottom}, normal, lower);
        addVertex(c, Vector2{uSpan, vTop}, normal, upper);
        addVertex(d, Vector2{0.0f, vTop}, normal, upper);
    }

    /// Adds a quad with explicit texture coordinates and a single colour.
    void addTexturedQuad(const Vector3 (&corners)[4], const Vector2 (&uvs)[4], Vector3 normal,
                         Color color) {
        const int order[6] = {0, 1, 2, 0, 2, 3};
        for (const int index : order) {
            addVertex(corners[index], uvs[index], normal, color);
        }
    }

    /// Copies the data into raylib-owned memory and uploads it to the GPU.
    Mesh build() const {
        Mesh mesh{};
        mesh.vertexCount = static_cast<int>(m_positions.size() / 3);
        mesh.triangleCount = mesh.vertexCount / 3;
        mesh.vertices = copyToRaylib(m_positions);
        mesh.texcoords = copyToRaylib(m_texcoords);
        mesh.normals = copyToRaylib(m_normals);
        mesh.colors =
            static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(m_colors.size())));
        std::memcpy(mesh.colors, m_colors.data(), m_colors.size());
        UploadMesh(&mesh, false);
        return mesh;
    }

private:
    void addVertex(Vector3 position, Vector2 uv, Vector3 normal, Color color) {
        m_positions.insert(m_positions.end(), {position.x, position.y, position.z});
        m_texcoords.insert(m_texcoords.end(), {uv.x, uv.y});
        m_normals.insert(m_normals.end(), {normal.x, normal.y, normal.z});
        m_colors.insert(m_colors.end(), {color.r, color.g, color.b, color.a});
    }

    /// raylib frees mesh arrays with its own allocator, so they must come from MemAlloc().
    static float* copyToRaylib(const std::vector<float>& values) {
        auto* memory =
            static_cast<float*>(MemAlloc(static_cast<unsigned int>(values.size() * sizeof(float))));
        std::memcpy(memory, values.data(), values.size() * sizeof(float));
        return memory;
    }

    std::vector<float> m_positions;
    std::vector<float> m_texcoords;
    std::vector<float> m_normals;
    std::vector<unsigned char> m_colors;
};

/// Adds one vertical wall face between two bottom corners, split into three height bands.
void addWallFace(MeshBuilder& builder, Vector3 left, Vector3 right, Vector3 normal,
                 const WallMeshColors& colors, float shadeFactor) {
    const float heights[4] = {0.0f, FLOOR_BAND_HEIGHT, world3d::WALL_HEIGHT - GLOW_BAND_HEIGHT,
                              world3d::WALL_HEIGHT};
    const Color lower[3] = {colors.floorBand, colors.base, colors.glowBand};
    const Color upper[3] = {colors.floorBand, colors.body, colors.glowBand};
    // One texture tile per cell of wall length, so the panel seams mark every cell.
    const float tiles = std::hypot(right.x - left.x, right.z - left.z) / world3d::CELL_SIZE;
    for (int band = 0; band < 3; ++band) {
        const float bottom = heights[band];
        const float top = heights[band + 1];
        builder.addQuad(Vector3{left.x, bottom, left.z}, Vector3{right.x, bottom, right.z},
                        Vector3{right.x, top, right.z}, Vector3{left.x, top, left.z}, normal,
                        shade(lower[band], shadeFactor), shade(upper[band], shadeFactor), tiles,
                        1.0f - bottom / world3d::WALL_HEIGHT, 1.0f - top / world3d::WALL_HEIGHT);
    }
}

/// Adds a closed box (without bottom face) for one wall run.
void addWallBox(MeshBuilder& builder, const WallRun& run, const WallMeshColors& colors) {
    const float half = world3d::WALL_THICKNESS / 2.0f;
    const float size = world3d::CELL_SIZE;
    const float x0 = std::min(run.start.x, run.end.x) * size - half;
    const float x1 = std::max(run.start.x, run.end.x) * size + half;
    const float z0 = std::min(run.start.y, run.end.y) * size - half;
    const float z1 = std::max(run.start.y, run.end.y) * size + half;
    addWallFace(builder, Vector3{x1, 0.0f, z0}, Vector3{x0, 0.0f, z0}, Vector3{0.0f, 0.0f, -1.0f},
                colors, 1.0f);
    addWallFace(builder, Vector3{x0, 0.0f, z1}, Vector3{x1, 0.0f, z1}, Vector3{0.0f, 0.0f, 1.0f},
                colors, 1.0f);
    addWallFace(builder, Vector3{x0, 0.0f, z0}, Vector3{x0, 0.0f, z1}, Vector3{-1.0f, 0.0f, 0.0f},
                colors, SIDE_FACE_SHADE);
    addWallFace(builder, Vector3{x1, 0.0f, z1}, Vector3{x1, 0.0f, z0}, Vector3{1.0f, 0.0f, 0.0f},
                colors, SIDE_FACE_SHADE);
    const float top = world3d::WALL_HEIGHT;
    builder.addQuad(Vector3{x0, top, z1}, Vector3{x1, top, z1}, Vector3{x1, top, z0},
                    Vector3{x0, top, z0}, Vector3{0.0f, 1.0f, 0.0f}, colors.top, colors.top);
}

} // namespace

Vector3 cellToWorld(core::Vec2f cell, float height) {
    return Vector3{(cell.x + 0.5f) * world3d::CELL_SIZE, height,
                   (cell.y + 0.5f) * world3d::CELL_SIZE};
}

Mesh buildWallMesh(const std::vector<WallRun>& runs, const WallMeshColors& colors) {
    MeshBuilder builder;
    for (const WallRun& run : runs) {
        addWallBox(builder, run, colors);
    }
    return builder.build();
}

Mesh buildPlaneMesh(float extent, float height, float textureRepeats, bool facingUp) {
    MeshBuilder builder;
    const Vector3 normal{0.0f, facingUp ? 1.0f : -1.0f, 0.0f};
    const Vector3 corners[4] = {Vector3{0.0f, height, extent}, Vector3{extent, height, extent},
                                Vector3{extent, height, 0.0f}, Vector3{0.0f, height, 0.0f}};
    const Vector2 uvs[4] = {Vector2{0.0f, textureRepeats}, Vector2{textureRepeats, textureRepeats},
                            Vector2{textureRepeats, 0.0f}, Vector2{0.0f, 0.0f}};
    builder.addTexturedQuad(corners, uvs, normal, WHITE);
    return builder.build();
}

} // namespace render
