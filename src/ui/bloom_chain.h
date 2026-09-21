#pragma once

#include "ui/framebuffer.h"
#include "ui/fullscreen_triangle.h"
#include "ui/shader.h"

#include <filesystem>
#include <glad/glad.h>

namespace ui {

// A stack of ever blurrier copies of the scene, each half the size of the one before, for the
// glow effect. Level 1 is half the scene's size, level 2 a quarter, and so on up to level 6.
// Reading a high level gives a wide blur for one texture lookup. Needs a current OpenGL context
// for every call, including construction and destruction.
class BloomChain {
  public:
    static constexpr int kLevels = 6;

    BloomChain() = default;

    BloomChain(const BloomChain&) = delete;
    BloomChain& operator=(const BloomChain&) = delete;

    // Loads downsample.frag (and fullscreen.vert) from `shaderDirectory`. Returns false, after
    // printing the reason, if that fails.
    bool Load(const std::filesystem::path& shaderDirectory);

    void Release();

    bool IsValid() const { return shader_.IsValid(); }

    // Fills every level from `sceneTexture`, which is width by height pixels. Leaves the window
    // framebuffer bound and the viewport at its full size.
    void Build(GLuint sceneTexture, int width, int height);

    // The texture of a level, from 1 to kLevels.
    GLuint Texture(int level) const { return levels_[level - 1].Texture(); }

  private:
    Shader shader_;
    FullscreenTriangle triangle_;
    Framebuffer levels_[kLevels] = {Framebuffer(GL_RGBA16F), Framebuffer(GL_RGBA16F),
                                    Framebuffer(GL_RGBA16F), Framebuffer(GL_RGBA16F),
                                    Framebuffer(GL_RGBA16F), Framebuffer(GL_RGBA16F)};
};

} // namespace ui
