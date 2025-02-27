#pragma once
//#include <fmt/color.h>
//#include <fmt/core.h> // for fmt::print(). implements c++20 std::format
#include <iostream>

//#include <glbinding/gl/gl.h>
//#include <string>
#include <vector>
#include <map>
#include <GL/glcorearb.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
//using namespace gl;

namespace errorHandler {

static const std::map<GLenum, std::string> errorSourceMap{
    {GL_DEBUG_SOURCE_API, "SOURCE_API"},
    {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "WINDOW_SYSTEM"},
    {GL_DEBUG_SOURCE_SHADER_COMPILER, "SHADER_COMPILER"},
    {GL_DEBUG_SOURCE_THIRD_PARTY, "THIRD_PARTY"},
    {GL_DEBUG_SOURCE_APPLICATION, "APPLICATION"},
    {GL_DEBUG_SOURCE_OTHER, "OTHER"}};

static const std::map<GLenum, std::string> errorTypeMap{
    {GL_DEBUG_TYPE_ERROR, "ERROR"},
    {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DEPRECATED_BEHAVIOR"},
    {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UNDEFINED_BEHAVIOR"},
    {GL_DEBUG_TYPE_PORTABILITY, "PORTABILITY"},
    {GL_DEBUG_TYPE_PERFORMANCE, "PERFORMANCE"},
    {GL_DEBUG_TYPE_OTHER, "OTHER"},
    {GL_DEBUG_TYPE_MARKER, "MARKER"}};

static const std::map<GLenum, std::string> severityMap{
    {GL_DEBUG_SEVERITY_HIGH, "HIGH"},
    {GL_DEBUG_SEVERITY_MEDIUM, "MEDIUM"},
    {GL_DEBUG_SEVERITY_LOW, "LOW"},
    {GL_DEBUG_SEVERITY_NOTIFICATION, "NOTIFICATION"}};

void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                     GLsizei length, const GLchar* message,
                     const void* userParam) {
    std::string src = errorSourceMap.at(source);
    std::string tp = errorTypeMap.at(type);
    std::string sv = severityMap.at(severity);
    // fmt::print(
    //     stderr,
    //     "GL CALLBACK: {0:s} type = {1:s}, severity = {2:s}, message = {3:s}\n",
    //     src, tp, sv, message);
}

bool checkShader(GLuint shaderIn, std::string shaderName, bool forceLog = false) {
    GLboolean fShaderCompiled = GL_FALSE;
    glGetShaderiv(shaderIn, GL_COMPILE_STATUS, &fShaderCompiled);
    if (fShaderCompiled != GL_TRUE || forceLog == true) {
        if(forceLog == false){
            std::cout << "Unable to compile " << shaderName << " " << shaderIn << "\n";
        } else {
            std::cout << "Force log Unable to compile " << shaderName << " " << shaderIn << "\n";
        }
        GLint log_length;

        glGetShaderiv(shaderIn, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> v(log_length);

        glGetShaderInfoLog(shaderIn, log_length, nullptr, v.data());
         std::cout <<  v.data() << "\n";
        //fmt::print(stderr, fmt::fg(fmt::color::light_green), "{}\n", v.data());

        return false;
    }
    return true;
}

} // namespace errorHandler
