#pragma once

#include "ui/shader.h"

#include <filesystem>
#include <glad/glad.h>

namespace ui {

// Draws a texture over the whole window through a fragment shader. This is the last step of a
// frame: the UI has been rendered into an offscreen texture, and this puts it on screen. Needs
// a current OpenGL context for every call, including construction and destruction.
class ScreenPass {
  public:
    ScreenPass() = default;
    ~ScreenPass();

    ScreenPass(const ScreenPass&) = delete;
    ScreenPass& operator=(const ScreenPass&) = delete;

    // Loads fullscreen.vert and passthrough.frag from `shaderDirectory`. Returns false, after
    // printing the reason, if they cannot be loaded; the caller should then draw the texture
    // some other way.
    bool Load(const std::filesystem::path& shaderDirectory);

    void Release();

    bool IsValid() const { return shader_.IsValid(); }

    // Draws `texture` to the window's framebuffer, which must be width by height pixels.
    void Draw(GLuint texture, int width, int height) const;

  private:
    Shader shader_;
    GLuint vertexArray_ = 0;
};

} // namespace ui
