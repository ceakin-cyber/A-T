#include "ui/shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ui {

namespace {

bool ReadFile(const std::filesystem::path& path, std::string& contents) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open shader file " << path << '\n';
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    contents = buffer.str();
    return true;
}

// Returns 0 and prints the compiler's log on failure.
GLuint Compile(GLenum type, const std::string& source, const std::filesystem::path& path) {
    const GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<std::size_t>(length > 1 ? length : 1));
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        std::cerr << "Shader " << path << " failed to compile:\n" << log.data() << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

Shader::~Shader() {
    Release();
}

void Shader::Release() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

bool Shader::Load(const std::filesystem::path& vertexPath,
                  const std::filesystem::path& fragmentPath) {
    Release();

    std::string vertexSource;
    std::string fragmentSource;
    if (!ReadFile(vertexPath, vertexSource) || !ReadFile(fragmentPath, fragmentSource)) {
        return false;
    }

    const GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const GLuint fragment =
        vertex != 0 ? Compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath) : 0;
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<std::size_t>(length > 1 ? length : 1));
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << "Shader program (" << vertexPath << ", " << fragmentPath
                  << ") failed to link:\n"
                  << log.data() << '\n';
        glDeleteProgram(program);
        return false;
    }

    program_ = program;
    return true;
}

void Shader::Use() const {
    glUseProgram(program_);
}

void Shader::SetInt(const char* name, int value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void Shader::SetFloat(const char* name, float value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void Shader::SetVec2(const char* name, float x, float y) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform2f(location, x, y);
    }
}

} // namespace ui
