#version 410 core

// Draws the scene texture to the screen with a CRT look. Each effect is switched off by setting
// its intensity to zero; with every effect off the output is the scene unchanged.
uniform sampler2D uScene;

// Scanlines: dark horizontal bands. 0 = off, 1 = the darkest row is fully black.
uniform float uScanlineIntensity;
// Distance in pixels from one dark band to the next.
uniform float uScanlinePeriod;

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

void main() {
    vec4 scene = texture(uScene, vUv);
    fragColor = vec4(scene.rgb * Scanlines(), scene.a);
}
