#version 410 core

// Copies a texture into a target half its size. The linear filter averages each 2x2 block of the
// source, so each pass gives a blurrier picture at half the width and height.
uniform sampler2D uSrc;

in vec2 vUv;
out vec4 fragColor;

void main() {
    fragColor = texture(uSrc, vUv);
}
