#version 410 core

// Draws the scene texture to the screen unchanged.
uniform sampler2D uScene;

in vec2 vUv;
out vec4 fragColor;

void main() {
    fragColor = texture(uScene, vUv);
}
