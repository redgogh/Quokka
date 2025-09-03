#ifndef WINDOW_H_
#define WINDOW_H_

#ifdef VK_VERSION_1_0
#define GLFW_INCLUDE_VULKAN
#endif /* VK_VERSION_1_0 */

#include <GLFW/glfw3.h>

class Window
{
public:
    Window(const char* title, uint32_t w, uint32_t h);
   ~Window();

    bool ShouldClose();

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
};

#endif /* WINDOW_H_ */
