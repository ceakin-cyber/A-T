#pragma once

#include "ui/bloom_chain.h"
#include "ui/fullscreen_triangle.h"
#include "ui/shader.h"

#include <filesystem>
#include <glad/glad.h>

namespace ui {

// How the CRT effects look. An intensity of zero turns that effect off.
struct CrtSettings {
    float scanlineIntensity = 0.21F; // 0 to 1: how dark the dark rows get
    float scanlinePeriod = 8.0F;     // rows from one dark band to the next; even values reach
                                     // the full intensity, odd values fall slightly short

    float bloomIntensity = 0.67F; // 0 turns the glow off
    float bloomSpread = 2.0F;     // halo width: 0 is a few pixels, and each step doubles it
    float bloomThreshold = 0.05F; // 0 to 1: how bright something must be to glow

    float vignetteIntensity = 0.2F; // 0 turns the vignette off; 1 takes the far corners to black
    float vignetteRadius = 0.2F;    // 0 to 1: how far out from the centre it starts
};

// Draws a texture over the whole window through a fragment shader. This is the last step of a
// frame: the UI has been rendered into an offscreen texture, and this puts it on screen. Needs
// a current OpenGL context for every call, including construction and destruction.
class ScreenPass {
  public:
    ScreenPass() = default;
    ~ScreenPass();

    ScreenPass(const ScreenPass&) = delete;
    ScreenPass& operator=(const ScreenPass&) = delete;

    // Loads fullscreen.vert and crt.frag from `shaderDirectory`. Returns false, after
    // printing the reason, if they cannot be loaded; the caller should then draw the texture
    // some other way.
    bool Load(const std::filesystem::path& shaderDirectory);

    void Release();

    bool IsValid() const { return shader_.IsValid(); }

    // Draws `texture` to the window's framebuffer, which must be width by height pixels. The
    // glow reads the blurred copies in `bloom`, which must have been built from the same texture
    // (see BloomChain::Build) whenever settings.bloomIntensity is above zero.
    void Draw(GLuint texture, int width, int height, const CrtSettings& settings,
              const BloomChain& bloom) const;

  private:
    Shader shader_;
    FullscreenTriangle triangle_;
};

} // namespace ui
