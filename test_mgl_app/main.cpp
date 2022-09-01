//#include "error_handling.hpp"
#include "obj_loader_simple_split_cpp.hpp"

#include <array>
#include <chrono>     // current time
#include <cmath>      // sin & cos
#include <cstdlib>    // for std::exit()
#include <fmt/core.h> // for fmt::print(). implements c++20 std::format

// this is really important to make sure that glbindings does not clash with
// glfw's opengl includes. otherwise we get ambigous overloads.
// #define GLFW_INCLUDE_NONE
// #include <GLFW/glfw3.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#if !defined(TEST_MGL_GLFW) && !defined(TEST_MGL_SDL)
#define TEST_MGL_GLFW 1
#endif

//#include <glbinding/gl/gl.h>
//#include <glbinding/glbinding.h>

//#include <glbinding-aux/debug.h>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C"
{
#include "MGLContext.h"
}
#include "MGLRenderer.h"

#if TEST_MGL_GLFW
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#define SWAP_BUFFERS MGLswapBuffers((GLMContext)glfwGetWindowUserPointer(window));
#endif

// using namespace gl;
using namespace std::chrono;

int main(int argc, char *argv[])
{

    auto startTime = system_clock::now();

    const int width = 1600;
    const int height = 900;

    // fmt::print("Program Location {}\n", argv[0]);

    auto window = [&]()
    {
        if (!glfwInit())
        {
            // fmt::print("glfw didnt initialize!\n");
            std::cout << "glfw didnt initialize!\n";
            std::exit(EXIT_FAILURE);
        }
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        // glfwWindowHint(GLFW_WIN32_KEYBOARD_MENU, GLFW_TRUE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);

        /* Create a windowed mode window and its OpenGL context */
        auto window = glfwCreateWindow(
            width, height, "Chapter 12 - Shader Transforms", nullptr, nullptr);

        GLMContext glm_ctx = createGLMContext(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        void *renderer = CppCreateMGLRendererAndBindToContext(glfwGetCocoaWindow(window), glm_ctx); // FIXME should do something later with the renderer
        if (!renderer)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        MGLsetCurrentContext(glm_ctx);
        glfwSetWindowUserPointer(window, glm_ctx);

        if (!window)
        {
            // fmt::print("window doesn't exist\n");
            std::cout << "window doesn't exist\n";

            glfwTerminate();
            std::exit(EXIT_FAILURE);
        }

        glfwSetWindowPos(window, 160, 90);
        glfwMakeContextCurrent(window);

        // glbinding::initialize(glfwGetProcAddress, false);
        return window;
    }();

    // debugging
    // {
    //     glEnable(GL_DEBUG_OUTPUT);
    //     glDebugMessageCallback(errorHandler::MessageCallback, 0);
    //     glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //     glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER,
    //                           GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
    //                           false);
    // }

    auto createProgram = [](const char *vertexShaderSource,
                            const char *fragmentShaderSource) -> GLuint
    {
        auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        // errorHandler::checkShader(vertexShader, "Vertex");

        auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        // errorHandler::checkShader(fragmentShader, "Fragment");

        auto program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);
        return program;
    };

    const char *fragmentShaderSource = R"(
        #version 460 core

        layout (location = 0) in vec3 colour;
        layout (location = 0) out vec4 finalColor;

        void main() {
            finalColor = vec4(colour, 1.0);
        }
        )";

    auto programBG = createProgram(R"(
        #version 460 core
        layout (location = 0) out vec3 colour;

        const vec4 vertices[] = vec4[]( vec4(-1.f, -1.f, 0.9999, 1.0),
                                        vec4( 3.f, -1.f, 0.9999, 1.0),    
                                        vec4(-1.f,  3.f, 0.9999, 1.0));   
        const vec3 colours[]   = vec3[](vec3(0.12f, 0.14f, 0.16f),
                                        vec3(0.12f, 0.14f, 0.16f),
                                        vec3(0.80f, 0.80f, 0.82f));
        

        void main(){
            colour = colours[gl_VertexID];
            gl_Position = vertices[gl_VertexID];  
        }
    )",
                                   fragmentShaderSource);

    auto program = createProgram(R"(
            #version 460 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec3 normal;
            layout(location = 2) uniform mat4 modelViewProjection;

            layout (location = 0) out vec3 colour;


            vec3 remappedColour = (normal + vec3(1.f)) / 2.f;

            void main(){
                colour = remappedColour;
                gl_Position = modelViewProjection * vec4(position, 1.0f);
            }
        )",
                                 fragmentShaderSource);

    

    auto meshData = objLoader::readObjSplit("rubberToy.obj");

    auto createBuffer =
        [&program](const std::vector<vertex3D> &vertices) -> GLuint
    {
        GLuint bufferObject;
        glCreateBuffers(1, &bufferObject);

        // upload immediately
        glNamedBufferStorage(bufferObject, vertices.size() * sizeof(vertex3D),
                             vertices.data(),
                             GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

        return bufferObject;
    };

    auto backGroundBuffer = createBuffer(meshData.vertices);

    auto createVertexArrayObject = [](GLuint program) -> GLuint
    {
        GLuint vao;
        glCreateVertexArrays(1, &vao);

        glEnableVertexArrayAttrib(vao, 0);
        glEnableVertexArrayAttrib(vao, 1);

        glVertexArrayAttribBinding(vao,
                                   0,
                                   /*buffer index*/ 0);
        glVertexArrayAttribBinding(vao, 1,
                                   /*buffs idx*/ 0);

        glVertexArrayAttribFormat(vao, 0, glm::vec3::length(), GL_FLOAT,
                                  GL_FALSE, offsetof(vertex3D, position));
        glVertexArrayAttribFormat(vao, 1, glm::vec3::length(), GL_FLOAT,
                                  GL_FALSE, offsetof(vertex3D, normal));

        return vao;
    };

    auto meshVao = createVertexArrayObject(program);
    glVertexArrayVertexBuffer(meshVao, 0, backGroundBuffer,
                              /*offset*/ 0,
                              /*stride*/ sizeof(vertex3D));
    glBindVertexArray(meshVao);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::array<GLfloat, 4> clearColour{0.f, 0.f, 0.f, 1.f};
    GLfloat clearDepth{1.0f};
    glClearColor(0.2, 0.2, 0.2, 0.0);

    const glm::mat4 projection = glm::perspective(
        glm::radians(65.0f),
        static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    const glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 mvp;

    //int mvpLocation = glGetUniformLocation(program, "modelViewProjection");
    //int mvpgridLocation = glGetUniformLocation(gridProgram, "modelViewProjection");

    while (!glfwWindowShouldClose(window))
    {
        auto renderStart = system_clock::now();

        //glClearBufferfv(GL_DEPTH, 0, &clearDepth);

        glUseProgram(programBG);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(program);
        auto currentTime =
            duration<float>(system_clock::now() - startTime).count();

        glm::mat4 view = glm::lookAt(
            glm::vec3(std::sin(currentTime * 0.5f) * 2,
                      ((std::sin(currentTime * 0.62f))), // + 0.5f) / 2.0f) * 2,
                      std::cos(currentTime * 0.5f) *
                          2),    // Camera is at (4,3,3), in World Space
            glm::vec3(0, .4, 0), // and looks at the origin
            glm::vec3(0, 1, 0)   // Head is up (set to 0,-1,0 to look upside-down)
        );

        mvp = projection * view * model;

        glProgramUniformMatrix4fv(program, 2, 1, GL_FALSE,
                                  glm::value_ptr(mvp));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)meshData.vertices.size());

        SWAP_BUFFERS(window);
        auto timeTaken =
            duration<float>(system_clock::now() - renderStart).count();
        // fmt::print(stderr, "render time: {}\n", timeTaken);
        glfwPollEvents();
    }

    glfwTerminate();
}
