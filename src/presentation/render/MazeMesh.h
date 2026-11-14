#pragma once

#include "core/Cell.h"
#include "render/WallRuns.h"

#include <raylib.h>

#include <vector>

/// @file MazeMesh.h
/// Builds the static 3D geometry of a maze once per level: one mesh for all walls and simple
/// planes for floor and ceiling. Drawing a 50 x 50 maze then costs a handful of draw calls.

namespace render {

/// Dimensions of the 3D maze in world units (1 cell = CELL_SIZE x CELL_SIZE).
namespace world3d {
inline constexpr float CELL_SIZE = 2.0f;
inline constexpr float WALL_HEIGHT = 1.8f;
inline constexpr float WALL_THICKNESS = 0.14f;
inline constexpr float EYE_HEIGHT = 0.92f;
} // namespace world3d

/// Vertex colours of the neon walls.
struct WallMeshColors {
    Color base;      ///< Bottom of the wall body.
    Color body;      ///< Top of the wall body.
    Color glowBand;  ///< Bright strip along the top edge.
    Color floorBand; ///< Thin strip where the wall meets the floor.
    Color top;       ///< Top face.
};

/// Centre of a cell (in cell units, so tweened positions work) at height @p height.
Vector3 cellToWorld(core::Vec2f cell, float height);

/// One box per wall run, every face split into a floor strip, a gradient body and a glowing
/// top strip. The mesh is uploaded to the GPU and owned by the caller (wrap it in a Model).
Mesh buildWallMesh(const std::vector<WallRun>& runs, const WallMeshColors& colors);

/// A square plane covering [0, extent] x [0, extent] at @p height, with texture coordinates
/// repeating @p textureRepeats times (one grid tile per cell).
Mesh buildPlaneMesh(float extent, float height, float textureRepeats, bool facingUp);

} // namespace render
