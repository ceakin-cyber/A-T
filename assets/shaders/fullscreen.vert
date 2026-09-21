#version 410 core

// Draws one triangle that covers the whole screen, with no vertex buffer. The three vertices
// come from gl_VertexID: (0,0), (2,0) and (0,2) in texture space, mapped to clip space.
out vec2 vUv;

void main() {
    vec2 uv = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vUv = uv;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
