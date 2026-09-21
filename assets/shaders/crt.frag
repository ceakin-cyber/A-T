#version 410 core

// Draws the scene texture to the screen with a CRT look. Each effect is switched off by setting
// its intensity to zero; with every effect off the output is the scene unchanged.
uniform sampler2D uScene;

// Scanlines: dark horizontal bands. 0 = off, 1 = the darkest row is fully black.
uniform float uScanlineIntensity;
// Distance in pixels from one dark band to the next.
uniform float uScanlinePeriod;

// Glow: bright areas spill a soft halo onto their surroundings. 0 = off.
uniform float uBloomIntensity;
// How wide the halo is. The scene is kept at six blurrier and blurrier sizes (uBloom1 is half the
// size, uBloom2 a quarter, ... uBloom6 a sixty-fourth); spread 0 blends the smallest blurs and
// each step up moves to wider ones, so 5 is the widest.
uniform float uBloomSpread;
// Brightness (0 to 1) below which nothing glows, so dim backgrounds do not.
uniform float uBloomThreshold;
uniform sampler2D uBloom1;
uniform sampler2D uBloom2;
uniform sampler2D uBloom3;
uniform sampler2D uBloom4;
uniform sampler2D uBloom5;
uniform sampler2D uBloom6;

in vec2 vUv;
out vec4 fragColor;

const float TAU = 6.28318530718;

// Brightness factor for scanlines, from 1 - uScanlineIntensity (darkest) up to 1. The wave is
// taken over whole pixel rows, not pixel centres, so the first row of every period is always at
// full brightness and a period of 2 alternates bright and dark rows cleanly. With an odd period no
// row lands exactly on the lowest point of the wave, so the darkest row is a little lighter than
// 1 - uScanlineIntensity (for a period of 3 it is 1 - 0.75 * uScanlineIntensity).
float Scanlines() {
    if (uScanlineIntensity <= 0.0 || uScanlinePeriod <= 0.0) {
        return 1.0;
    }
    float darkness = 0.5 - 0.5 * cos(floor(gl_FragCoord.y) * TAU / uScanlinePeriod);
    return 1.0 - uScanlineIntensity * darkness;
}

// Blend weight of blur level `level` (1 to 6) for the current spread: a triangle centred on the
// level the spread points at, so moving the spread slides smoothly from one level to the next.
float BloomWeight(float level) {
    return max(0.0, 1.5 - abs(level - (uBloomSpread + 1.0)));
}

// The halo to add to the picture: a weighted blend of the blurred copies, with the dim part cut
// off by the threshold.
vec3 Glow() {
    if (uBloomIntensity <= 0.0) {
        return vec3(0.0);
    }
    float w1 = BloomWeight(1.0);
    float w2 = BloomWeight(2.0);
    float w3 = BloomWeight(3.0);
    float w4 = BloomWeight(4.0);
    float w5 = BloomWeight(5.0);
    float w6 = BloomWeight(6.0);
    vec3 blurred = w1 * texture(uBloom1, vUv).rgb + w2 * texture(uBloom2, vUv).rgb +
                   w3 * texture(uBloom3, vUv).rgb + w4 * texture(uBloom4, vUv).rgb +
                   w5 * texture(uBloom5, vUv).rgb + w6 * texture(uBloom6, vUv).rgb;
    blurred /= (w1 + w2 + w3 + w4 + w5 + w6);
    return max(blurred - vec3(uBloomThreshold), vec3(0.0)) * uBloomIntensity;
}

void main() {
    vec4 scene = texture(uScene, vUv);
    fragColor = vec4(scene.rgb * Scanlines() + Glow(), scene.a);
}
