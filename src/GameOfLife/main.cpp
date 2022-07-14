#include "stdafx.h"
#include "ScopeGuard.h"
#include "GraphicsLogger.h"
#include "GraphicsResource.h"
#include "LogFormatter.h"
#include "PlanarTextureRenderer.h"
#include "CellularAutomata.h"
#include "LifeContext.h"

static const int Width = 800;
static const int Height = 600;

static const int TextureSize = 512;

static const std::string Title = "Conway's Game of Life";


/*****************************************************************************
 * GLFW Callbacks
 ****************************************************************************/

void ErrorCallback(int /*error*/, const char* description) {
    LOGE << "Error: " << description;
}


/*****************************************************************************
 *GUI Functions
 ****************************************************************************/

void GuiInit(GLFWwindow* window) {
    assert(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable .ini

    static const std::string glslVersion = "#version 330 core";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion.c_str());
}

void GuiTerminate() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    LOGD << "Cleanup : ImGui";
}

void GuiNewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiRender() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


/*****************************************************************************
 * Main program
 ****************************************************************************/

int main(int /*argc*/, char** /*argv*/) {
    static plog::ConsoleAppender<plog::LogFormatter> logger;
#ifdef NDEBUG
    plog::init(plog::info, &logger);
#else
    plog::init(plog::debug, &logger);
#endif

    glfwSetErrorCallback(ErrorCallback);

    if (!glfwInit()) {
        LOGE << "Failed to load GLFW";
        return EXIT_FAILURE;
    }
    ScopeGuard glfwGuard([]() { glfwTerminate(); });

    LOGI << "Init window context with OpenGL 3.3";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
        glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr),
        &glfwDestroyWindow);
    if (!window) {
        LOGE << "Unable to Create OpenGL 3.3 Context";
        return EXIT_FAILURE;
    }

    glfwSetKeyCallback(window.get(), LifeContext::Keyboard);
    glfwSetInputMode(window.get(), GLFW_STICKY_KEYS, GLFW_TRUE);

    glfwSetWindowSizeCallback(window.get(), LifeContext::Reshape);

    glfwSetMouseButtonCallback(window.get(), LifeContext::Mouse);
    glfwSetInputMode(window.get(), GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    glfwMakeContextCurrent(window.get());
    gladLoadGL();

    glfwSwapInterval(0); // Disable vsync to get maximum number of iterations

    // Setup ImGui
    GuiInit(window.get());
    ScopeGuard imGuiContextGuard([]() { GuiTerminate(); });

    // Setup program objects
    LifeContext context(window.get());
    if (!context.Init(Width, Height, TextureSize)) {
        LOGE << "Initialization failed";
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();

        // Start ImGui frame
        GuiNewFrame();

        context.Display();

        // Render ImGui
        GuiRender();

        context.Update();

        glfwSwapBuffers(window.get());
    }

    return EXIT_SUCCESS;
}
