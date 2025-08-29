#include <memory>
#include "driver/render_driver.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <direct.h>
#endif

#include <stb/stb_image.h>

struct Vertex
{
    float pos[2];
    float color[3];
};

Vertex vertices[] = {
    {{0.0, -0.5}, {1.0f, 0.0f, 0.0f}}, // 上
    {{0.5, 0.5}, {0.0f, 1.0f, 0.0f}}, // 左
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} // 右
};

int main()
{
#ifdef WIN32
    char _cwd[PATH_MAX];
    system("chcp 65001");
    getcwd(_cwd, sizeof(_cwd));
    _chdir("../shaders");
    system("spvc.bat");
    _chdir(_cwd);
#endif

#ifdef __APPLE__
    char _cwd[PATH_MAX];
    getcwd(_cwd, sizeof(_cwd));
    chdir("../shaders");
    system("./spvc");
    chdir(_cwd);
#endif

    glfwInit();

//    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* hwindow =
        glfwCreateWindow(800, 600, "capybara", nullptr, nullptr);

    if (hwindow == nullptr)
        throw std::runtime_error("Failed to create GLFW window");

    const std::unique_ptr<RenderDriver> driver = std::make_unique<RenderDriver>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(driver->GetInstance(), hwindow, VK_NULL_HANDLE, &surface);
    assert(!err);
    driver->Initialize(surface);

    Pipeline pipeline;
    driver->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    size_t vertexBufferSize = sizeof(vertices);
    driver->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &vertexBuffer);
    driver->WriteBuffer(vertexBuffer, vertexBufferSize, vertices);

    VkCommandBuffer commandBuffer;

    while (!glfwWindowShouldClose(hwindow)) {
        glfwPollEvents();

        driver->AcquiredNextFrame(&commandBuffer);

        driver->BeginCommandBuffer(commandBuffer);
        driver->CmdBeginRendering(commandBuffer);
        driver->CmdBindPipeline(commandBuffer, pipeline);
        driver->CmdBindVertexBuffer(commandBuffer, vertexBuffer, 0);
        driver->CmdDraw(commandBuffer, ARRAY_SIZE(vertices));
        driver->CmdEndRendering(commandBuffer);
        driver->EndCommandBuffer(commandBuffer);
        driver->SubmitAndPresentFrame(commandBuffer);
    }

    driver->DeviceWaitIdle();
    driver->DestroyPipeline(pipeline);
    driver->DestroyBuffer(vertexBuffer);

    glfwDestroyWindow(hwindow);
    glfwTerminate();

    return 0;
}