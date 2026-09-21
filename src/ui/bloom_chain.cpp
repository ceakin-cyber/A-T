#include "ui/bloom_chain.h"

#include <algorithm>

namespace ui {

bool BloomChain::Load(const std::filesystem::path& shaderDirectory) {
    Release();
    if (!shader_.Load(shaderDirectory / "fullscreen.vert", shaderDirectory / "downsample.frag")) {
        return false;
    }
    triangle_.Create();
    return true;
}

void BloomChain::Release() {
    shader_.Release();
    triangle_.Release();
    for (Framebuffer& level : levels_) {
        level.Resize(0, 0);
    }
}

void BloomChain::Build(GLuint sceneTexture, int width, int height) {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    shader_.Use();
    shader_.SetInt("uSrc", 0);
    glActiveTexture(GL_TEXTURE0);

    GLuint source = sceneTexture;
    int levelWidth = width;
    int levelHeight = height;
    for (Framebuffer& level : levels_) {
        levelWidth = std::max(1, levelWidth / 2);
        levelHeight = std::max(1, levelHeight / 2);
        if (!level.Resize(levelWidth, levelHeight)) {
            break;
        }
        level.Bind();
        glBindTexture(GL_TEXTURE_2D, source);
        triangle_.Draw();
        source = level.Texture();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    Framebuffer::Unbind();
    glViewport(0, 0, width, height);
}

} // namespace ui
