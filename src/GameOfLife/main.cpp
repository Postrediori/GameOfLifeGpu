#include "stdafx.h"
#include "GraphicsLogger.h"
#include "GraphicsResource.h"
#include "LogFormatter.h"
#include "PlanarTextureRenderer.h"
#include "CellularAutomata.h"
#include "GlfwWrapper.h"
#include "ImGuiWrapper.h"
#include "LifeContext.h"

static const int Width = 800;
static const int Height = 600;

static const int TextureSize = 512;

static const std::string Title = "Conway's Game of Life";


/*****************************************************************************
 * Main program
 ****************************************************************************/

int main(int argc, const char* argv[]) {
    plog::ConsoleAppender<plog::LogFormatter> logger;
#ifdef NDEBUG
    plog::init(plog::info, &logger);
#else
    plog::init(plog::debug, &logger);
#endif

    GraphicsUtils::GlfwWrapper glfwWrapper;
    if (glfwWrapper.Init(Title, Width, Height) != 0) {
        LOGE << "Failed to load GLFW";
        return EXIT_FAILURE;
    }

    glfwSwapInterval(0); // Disable vsync to get maximum number of iterations

    // Setup program objects
    LifeContext context(glfwWrapper.GetWindow());
    if (!context.Init(argc, argv, Width, Height, TextureSize)) {
        LOGE << "Initialization failed";
        return EXIT_FAILURE;
    }

    // Setup ImGui
    GraphicsUtils::ImGuiWrapper imguiWrapper;
    imguiWrapper.Init(glfwWrapper.GetWindow());

    while (!glfwWindowShouldClose(glfwWrapper.GetWindow())) {
        glfwPollEvents();

        // Start ImGui frame
        imguiWrapper.StartFrame();

        context.Display();

        // Render ImGui
        imguiWrapper.Render();

        context.Update();

        glfwSwapBuffers(glfwWrapper.GetWindow());
    }

    return EXIT_SUCCESS;
}
