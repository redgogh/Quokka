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

    glfwSetWindowUserPointer(hwindow, this);

    // 注册键盘事件回调函数
    glfwSetKeyCallback(hwindow, [](GLFWwindow* hwind, int key, int scancode, int action, int mods) {
        Window* window = static_cast<Window *>(glfwGetWindowUserPointer(hwind));
        std::vector<PFN_WindowKeyCallback> callbacks = window->keyCallbacks;
        for (auto keyCallback : callbacks)
            keyCallback(window, key, scancode, action, mods);
    });

    // 注册滚轮回调事件
    glfwSetScrollCallback(hwindow, [](GLFWwindow* hwind, double xOffset, double yOffset) {
        Window* window = static_cast<Window *>(glfwGetWindowUserPointer(hwind));
    });

    g_WindowCreatedCount++;
}

Window::~Window()
{
    glfwDestroyWindow(hwindow);
    g_WindowCreatedCount--;

    if (g_WindowCreatedCount <= 0)
        glfwTerminate();
}

bool Window::GetKey(int key) const
{
    return glfwGetKey(hwindow, key);
}

bool Window::GetMouseButton(int button) const
{
    return glfwGetMouseButton(hwindow, button);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(hwindow);
}

void Window::GetCursorPos(double *x, double *y) const
{
    glfwGetCursorPos(hwindow, x, y);
}

void Window::RegisterKeyCallback(PFN_WindowKeyCallback callback)
{
    keyCallbacks.push_back(callback);
}