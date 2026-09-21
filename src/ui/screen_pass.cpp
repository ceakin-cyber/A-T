#include "ui/screen_pass.h"

namespace ui {

ScreenPass::~ScreenPass() {
    Release();
}

void ScreenPass::Release() {
    shader_.Release();
    triangle_.Release();
}

bool ScreenPass::Load(const std::filesystem::path& shaderDirectory) {
    Release();
    if (!shader_.Load(shaderDirectory / "fullscreen.vert", shaderDirectory / "crt.frag")) {
        return false;
    }
    triangle_.Create();
    return true;
}

void ScreenPass::Draw(GLuint texture, int width, int height, const CrtSettings& settings,
                      const BloomChain& bloom) const {
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

    // The blurred copies go on texture units 1 to 6.
    static const char* const kNames[BloomChain::kLevels] = {"uBloom1", "uBloom2", "uBloom3",
                                                            "uBloom4", "uBloom5", "uBloom6"};
    for (int level = 1; level <= BloomChain::kLevels; ++level) {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(level));
        glBindTexture(GL_TEXTURE_2D, bloom.Texture(level));
        shader_.SetInt(kNames[level - 1], level);
    }

    shader_.SetFloat("uScanlineIntensity", settings.scanlineIntensity);
    shader_.SetFloat("uScanlinePeriod", settings.scanlinePeriod);
    shader_.SetFloat("uBloomIntensity", settings.bloomIntensity);
    shader_.SetFloat("uBloomSpread", settings.bloomSpread);
    shader_.SetFloat("uBloomThreshold", settings.bloomThreshold);

    triangle_.Draw();

    for (int level = BloomChain::kLevels; level >= 0; --level) {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(level));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glUseProgram(0);
}

} // namespace ui
