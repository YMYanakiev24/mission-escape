#pragma once

#include <raylib.h>

#include <utility>

/// @file GpuResources.h
/// Move-only RAII owners for raylib GPU resources. They release their resource in the
/// destructor, so they must be destroyed before the window (and its GL context) is closed.

namespace render {

/// Owns a RenderTexture2D (an off-screen framebuffer).
class RenderTarget {
public:
    RenderTarget() = default;

    /// Allocates a @p width x @p height framebuffer.
    RenderTarget(int width, int height) : m_target(LoadRenderTexture(width, height)) {}

    /// Releases the resource.
    ~RenderTarget() { reset(); }

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    /// Takes over the resource of @p other, which becomes empty.
    RenderTarget(RenderTarget&& other) noexcept : m_target(std::exchange(other.m_target, {})) {}

    /// Releases the current resource and takes over the one of @p other.
    RenderTarget& operator=(RenderTarget&& other) noexcept {
        if (this != &other) {
            reset();
            m_target = std::exchange(other.m_target, {});
        }
        return *this;
    }

    /// True when a framebuffer is allocated.
    bool isValid() const { return m_target.id != 0; }

    /// Width in pixels.
    int width() const { return m_target.texture.width; }
    /// Height in pixels.
    int height() const { return m_target.texture.height; }

    /// The raw raylib handle (still owned by this object).
    const RenderTexture2D& get() const { return m_target; }

    /// Frees the framebuffer.
    void reset() {
        if (m_target.id != 0) {
            UnloadRenderTexture(m_target);
            m_target = {};
        }
    }

private:
    RenderTexture2D m_target{};
};

/// Owns a Texture2D.
class TextureResource {
public:
    TextureResource() = default;

    /// Takes ownership of an already loaded texture.
    explicit TextureResource(Texture2D texture) : m_texture(texture) {}

    /// Releases the resource.
    ~TextureResource() { reset(); }

    TextureResource(const TextureResource&) = delete;
    TextureResource& operator=(const TextureResource&) = delete;

    /// Takes over the resource of @p other, which becomes empty.
    TextureResource(TextureResource&& other) noexcept
        : m_texture(std::exchange(other.m_texture, {})) {}

    /// Releases the current resource and takes over the one of @p other.
    TextureResource& operator=(TextureResource&& other) noexcept {
        if (this != &other) {
            reset();
            m_texture = std::exchange(other.m_texture, {});
        }
        return *this;
    }

    /// True when a resource is loaded.
    bool isValid() const { return m_texture.id != 0; }
    /// The raw raylib handle (still owned by this object).
    const Texture2D& get() const { return m_texture; }

    /// Releases the resource now and leaves this object empty.
    void reset() {
        if (m_texture.id != 0) {
            UnloadTexture(m_texture);
            m_texture = {};
        }
    }

private:
    Texture2D m_texture{};
};

/// Owns a Model (its meshes and material maps; shaders and textures stay owned elsewhere).
class ModelResource {
public:
    ModelResource() = default;

    /// Takes ownership of an already loaded model.
    explicit ModelResource(Model model) : m_model(model) {}

    /// Releases the resource.
    ~ModelResource() { reset(); }

    ModelResource(const ModelResource&) = delete;
    ModelResource& operator=(const ModelResource&) = delete;

    /// Takes over the resource of @p other, which becomes empty.
    ModelResource(ModelResource&& other) noexcept : m_model(std::exchange(other.m_model, {})) {}

    /// Releases the current resource and takes over the one of @p other.
    ModelResource& operator=(ModelResource&& other) noexcept {
        if (this != &other) {
            reset();
            m_model = std::exchange(other.m_model, {});
        }
        return *this;
    }

    /// True when a resource is loaded.
    bool isValid() const { return m_model.meshCount > 0; }
    /// The raw raylib handle (still owned by this object).
    Model& get() { return m_model; }
    /// The raw raylib handle (still owned by this object).
    const Model& get() const { return m_model; }

    /// Releases the resource now and leaves this object empty.
    void reset() {
        if (m_model.meshCount > 0) {
            UnloadModel(m_model);
            m_model = {};
        }
    }

private:
    Model m_model{};
};

/// Owns a Shader.
class ShaderResource {
public:
    ShaderResource() = default;

    /// Takes ownership of an already loaded shader. A shader that failed to compile is released
    /// immediately (raylib still allocates its location table) and the resource stays empty.
    explicit ShaderResource(Shader shader) {
        if (IsShaderValid(shader)) {
            m_shader = shader;
        } else {
            UnloadShader(shader);
        }
    }

    /// Releases the resource.
    ~ShaderResource() { reset(); }

    ShaderResource(const ShaderResource&) = delete;
    ShaderResource& operator=(const ShaderResource&) = delete;

    /// Takes over the resource of @p other, which becomes empty.
    ShaderResource(ShaderResource&& other) noexcept : m_shader(std::exchange(other.m_shader, {})) {}

    /// Releases the current resource and takes over the one of @p other.
    ShaderResource& operator=(ShaderResource&& other) noexcept {
        if (this != &other) {
            reset();
            m_shader = std::exchange(other.m_shader, {});
        }
        return *this;
    }

    /// True when a resource is loaded.
    bool isValid() const { return m_shader.id != 0; }
    /// The raw raylib handle (still owned by this object).
    const Shader& get() const { return m_shader; }

    /// Releases the resource now and leaves this object empty.
    void reset() {
        if (m_shader.id != 0) {
            UnloadShader(m_shader);
            m_shader = {};
        }
    }

private:
    Shader m_shader{};
};

} // namespace render
