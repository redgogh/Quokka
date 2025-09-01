#include "window.h"

#include <stdexcept>

static uint32_t g_WindowCreatedCount = 0;

Window::Window(const char *title, uint32_t w, uint32_t h)
{
    if (g_WindowCreatedCount == 0)
        glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    hwindow = glfwCreateWindow(w, h, title, NULL, NULL);
    if (!hwindow)
        throw std::runtime_error("[GLFW] create GLFW window failed!");

    g_WindowCreatedCount++;
}

Window::~Window()
{
    glfwDestroyWindow(hwindow);
    g_WindowCreatedCount--;

    if (g_WindowCreatedCount <= 0)
        glfwTerminate();
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(hwindow);
}