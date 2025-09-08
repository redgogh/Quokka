#ifndef WINDOW_H_
#define WINDOW_H_

#ifdef VK_VERSION_1_0
#define GLFW_INCLUDE_VULKAN
#endif /* VK_VERSION_1_0 */

#include <GLFW/glfw3.h>

// std
#include <unordered_map>
#include <vector>
#include <string>

class Window;

typedef void(*PFN_WindowKeyCallback)(Window* window, int key, int scancode, int action, int mods);
typedef void(*PFN_WindowMouseButtonCallback)(Window* window, int button, int action, int mods);
typedef void(*PFN_ScrollKeyCallback)(Window* window, double xOffset, double yOffset);
typedef void(*PFN_CursorPosCallback)(Window* window, double xOffset, double yOffset);

class Window
{
public:
    Window(const char* title, uint32_t w, uint32_t h);
   ~Window();

    /* Window 类通用方法 */
    bool GetKey(int key) const;
    bool GetMouseButton(int button) const;
    bool ShouldClose() const;
    void GetCursorPos(double *x, double *y) const;

    /* 设置用户窗口上下文数据用于回调中使用 */
    void SetUserContextData(const std::string& name, void* data);
    void* GetUserContextData(const std::string& name);

    /* 注册各个事件回调函数 */
    void RegisterKeyCallback(PFN_WindowKeyCallback callback);
    void RegisterMouseButtonCallback(PFN_WindowMouseButtonCallback callback);
    void RegisterScrollCallback(PFN_ScrollKeyCallback callback);
    void RegisterCursorPosCallback(PFN_CursorPosCallback callback);

    GLFWwindow* GetWindowHandle() const { return hwindow; }

#ifdef VK_VERSION_1_0
    VkResult CreateWindowSurface(VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
    {
        return glfwCreateWindowSurface(instance, hwindow, allocator, surface);
    }
#endif /* VK_VERSION_1_0 */

    static double GetTime()
    {
        return glfwGetTime();
    }

    static void PollEvents()
    {
        glfwPollEvents();
    }

private:
    GLFWwindow* hwindow = nullptr;
    std::unordered_map<std::string, void*> userData;
    std::vector<PFN_WindowKeyCallback> keyCallbacks;
    std::vector<PFN_WindowMouseButtonCallback> mouseButtonCallbacks;
    std::vector<PFN_ScrollKeyCallback> scrollCallbacks;
    std::vector<PFN_CursorPosCallback> cursorPosCallbacks;
};

#endif /* WINDOW_H_ */
