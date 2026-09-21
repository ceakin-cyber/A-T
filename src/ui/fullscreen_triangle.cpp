#include "ui/fullscreen_triangle.h"

namespace ui {

FullscreenTriangle::~FullscreenTriangle() {
    Release();
}

void FullscreenTriangle::Create() {
    if (vertexArray_ == 0) {
        glGenVertexArrays(1, &vertexArray_);
    }
}

void FullscreenTriangle::Release() {
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
}

void FullscreenTriangle::Draw() const {
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace ui
