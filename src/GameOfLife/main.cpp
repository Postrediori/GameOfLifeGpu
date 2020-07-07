#include "stdafx.h"
#include "ScopeGuard.h"
#include "GraphicsUtils.h"
#include "GraphicsLogFormatter.h"
#include "PlanarTextureRenderer.h"
#include "FrameBufferWrapper.h"
#include "LifeContext.h"

static const int Width = 800;
static const int Height = 600;

static const int TextureSize = 512;

static const std::string Title = "Conway's Game of Life";

/*****************************************************************************
 * Graphics-related functions
 ****************************************************************************/
static void Error(int /*error*/, const char* description) {
    LOGE << "Error: " << description;
}

/*****************************************************************************
 * Main program
 ****************************************************************************/
int main(int /*argc*/, char** /*argv*/) {
    static plog::ConsoleAppender<plog::GraphicsLogFormatter> consoleAppender;
#ifdef NDEBUG
    plog::init(plog::info, &consoleAppender);
#else
    plog::init(plog::debug, &consoleAppender);
#endif

    glfwSetErrorCallback(Error);

    if (!glfwInit()) {
        LOGE << "Failed to load GLFW";
        return EXIT_FAILURE;
    }
    ScopeGuard glfwGuard([]() {
        glfwTerminate();
        LOGD << "Cleanup : GLFW context";
    });

    LOGI << "Init window context with OpenGL 3.3";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    auto window = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
    if (!window) {
        LOGE << "Unable to Create OpenGL 3.3 Context";
        return EXIT_FAILURE;
    }
    ScopeGuard windowGuard([window]() {
        glfwDestroyWindow(window);
        LOGD << "Cleanup : GLFW window";
    });

    glfwSetKeyCallback(window, LifeContext::Keyboard);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    glfwSetWindowSizeCallback(window, LifeContext::Reshape);

    glfwSetMouseButtonCallback(window, LifeContext::Mouse);

    glfwMakeContextCurrent(window);
    gladLoadGL();

    LifeContext context;
    if (!context.Init(Width, Height, TextureSize)) {
        LOGE << "Initialization failed";
        return EXIT_FAILURE;
    }
    glfwSetWindowUserPointer(window, static_cast<void *>(&context));

    while (!glfwWindowShouldClose(window)) {
        context.Display();

        context.Update();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
