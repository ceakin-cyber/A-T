#include "ui/screen_pass.h"

namespace ui {

ScreenPass::~ScreenPass() {
    Release();
}

void ScreenPass::Release() {
    shader_.Release();
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
}

bool ScreenPass::Load(const std::filesystem::path& shaderDirectory) {
    Release();
    if (!shader_.Load(shaderDirectory / "fullscreen.vert", shaderDirectory / "passthrough.frag")) {
        return false;
    }
    // Core profile requires a vertex array object to draw, even with no vertex data.
    glGenVertexArrays(1, &vertexArray_);
    return true;
}

void ScreenPass::Draw(GLuint texture, int width, int height) const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    // The UI renderer leaves its own state behind; put back what a plain full-screen copy needs.
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    shader_.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader_.SetInt("uScene", 0);

    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

} // namespace ui
