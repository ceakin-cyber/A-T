#pragma once

#include <filesystem>
#include <glad/glad.h>

namespace ui {

// A linked GLSL program made from a vertex and a fragment shader file. Needs a current OpenGL
// context for every call, including construction and destruction.
class Shader {
  public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Reads, compiles and links the two files. On any failure, prints the reason (including the
    // compiler's log) to stderr, leaves the shader empty and returns false.
    bool Load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);

    void Release();

    bool IsValid() const { return program_ != 0; }
    GLuint Id() const { return program_; }

    // Makes this the active program.
    void Use() const;

    // Uniform setters. They do nothing if the shader has no uniform of that name, for instance
    // because the compiler removed an unused one. The program must be active (see Use).
    void SetInt(const char* name, int value) const;
    void SetFloat(const char* name, float value) const;
    void SetVec2(const char* name, float x, float y) const;

  private:
    GLuint program_ = 0;
};

} // namespace ui
