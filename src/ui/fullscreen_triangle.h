#pragma once

#include <glad/glad.h>

namespace ui {

// The one oversized triangle that covers the whole screen, drawn by fullscreen.vert, which makes
// its vertices from gl_VertexID. Needs a current OpenGL context for every call, including
// construction and destruction.
class FullscreenTriangle {
  public:
    FullscreenTriangle() = default;
    ~FullscreenTriangle();

    FullscreenTriangle(const FullscreenTriangle&) = delete;
    FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;

    // Core profile requires a vertex array object to draw, even with no vertex data. Safe to
    // call more than once.
    void Create();
    void Release();

    // Draws the triangle with the active shader.
    void Draw() const;

  private:
    GLuint vertexArray_ = 0;
};

} // namespace ui
