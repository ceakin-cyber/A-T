#pragma once

#include <glad/glad.h>

namespace ui {

// An offscreen render target: a framebuffer object with an RGBA color texture. The UI is drawn
// into it, and the texture can then be drawn to the screen through shaders. Needs a current
// OpenGL context for every call, including construction and destruction.
class Framebuffer {
  public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Creates the target at this size in pixels, or recreates it if the size has changed. Does
    // nothing if the size is unchanged. Returns false, and leaves nothing bound, if the size is
    // not positive or the framebuffer could not be completed.
    bool Resize(int width, int height);

    // Makes this the target of subsequent drawing, and sets the viewport to cover it.
    void Bind() const;

    // Goes back to drawing into the window.
    static void Unbind();

    // Copies the whole target to the window's framebuffer, which must be the same size.
    void BlitToScreen() const;

    GLuint Texture() const { return texture_; }
    int Width() const { return width_; }
    int Height() const { return height_; }

  private:
    void Release();

    GLuint framebuffer_ = 0;
    GLuint texture_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace ui
