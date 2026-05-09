#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// 日志级别
enum class LogLevel { INFO, WARNING, ERROR };

static void log(LogLevel level, const std::string& message) {
    const char* prefix;
    switch (level) {
        case LogLevel::INFO:    prefix = "[INFO]"; break;
        case LogLevel::WARNING: prefix = "[WARN]"; break;
        case LogLevel::ERROR:   prefix = "[ERROR]"; break;
    }
    std::cerr << prefix << " Shader: " << message << std::endl;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) : ID(0) {
    // 验证文件路径
    if (!vertexPath || !fragmentPath) {
        log(LogLevel::ERROR, "Shader paths cannot be null");
        return;
    }

    // 检查文件是否存在
    if (!fs::exists(vertexPath)) {
        log(LogLevel::ERROR, "Vertex shader file not found: " + std::string(vertexPath));
        return;
    }
    if (!fs::exists(fragmentPath)) {
        log(LogLevel::ERROR, "Fragment shader file not found: " + std::string(fragmentPath));
        return;
    }

    log(LogLevel::INFO, "Loading vertex shader: " + std::string(vertexPath));
    log(LogLevel::INFO, "Loading fragment shader: " + std::string(fragmentPath));

    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        
        log(LogLevel::INFO, "Shader files loaded successfully");
    } catch (std::ifstream::failure& e) {
        log(LogLevel::ERROR, "Failed to read shader file: " + std::string(e.what()));
        return;
    }

    // 验证着色器代码不为空
    if (vertexCode.empty()) {
        log(LogLevel::ERROR, "Vertex shader code is empty");
        return;
    }
    if (fragmentCode.empty()) {
        log(LogLevel::ERROR, "Fragment shader code is empty");
        return;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    if (!checkCompileErrors(vertex, "VERTEX")) {
        glDeleteShader(vertex);
        return;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    if (!checkCompileErrors(fragment, "FRAGMENT")) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    if (!checkCompileErrors(ID, "PROGRAM")) {
        glDeleteProgram(ID);
        ID = 0;
    } else {
        log(LogLevel::INFO, "Shader program linked successfully (ID: " + std::to_string(ID) + ")");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

bool Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            log(LogLevel::ERROR, "Shader compilation failed (" + type + "):\n" + std::string(infoLog));
            return false;
        }
        log(LogLevel::INFO, type + " shader compiled successfully");
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            log(LogLevel::ERROR, "Program linking failed:\n" + std::string(infoLog));
            return false;
        }
    }
    return true;
}
