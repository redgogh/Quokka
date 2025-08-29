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

#include "rendering/camera/camera.h"

#include <imgui/qk_imgui.h>

struct Vertex
{
    float pos[2];
    float color[3];
};

Vertex vertices[] = {
    {{  0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }}, // 上
    {{  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }}, // 左
    {{ -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }}  // 右
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
        glfwCreateWindow(800, 600, "Quokka", nullptr, nullptr);

    if (hwindow == nullptr)
        throw std::runtime_error("Failed to create GLFW window");

    const std::unique_ptr<RenderDriver> driver = std::make_unique<RenderDriver>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(driver->GetInstance(), hwindow, VK_NULL_HANDLE, &surface);
    assert(!err);
    driver->Initialize(surface);

    ImGui_ImplVulkan_InitInfo _ImGuiVulkanInitInfo = {};
    _ImGuiVulkanInitInfo.Instance = driver->GetInstance();
    _ImGuiVulkanInitInfo.PhysicalDevice = driver->GetPhysicalDevice();
    _ImGuiVulkanInitInfo.Device = driver->GetDevice();
    _ImGuiVulkanInitInfo.QueueFamily = driver->GetQueueFamilyIndex();
    _ImGuiVulkanInitInfo.Queue = driver->GetGraphicsQueue();
    _ImGuiVulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    _ImGuiVulkanInitInfo.DescriptorPool = p_initialize_info->DescriptorPool;
    _ImGuiVulkanInitInfo.Subpass = 0;
    _ImGuiVulkanInitInfo.MinImageCount = driver->GetMinImageCount();
    _ImGuiVulkanInitInfo.ImageCount = driver->GetMinImageCount();
    _ImGuiVulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    QkImGuiVulkanHInit(&_ImGuiVulkanInitInfo);

    Pipeline pipeline;
    driver->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    size_t vertexBufferSize = sizeof(vertices);
    driver->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &vertexBuffer);
    driver->WriteBuffer(vertexBuffer, vertexBufferSize, vertices);

    glm::vec3 position(0.0f, 0.0f, 3.0f);
    float aspectRatio = driver->GetSwapchainAspectRatio();
    Camera camera(position, aspectRatio);

    while (!glfwWindowShouldClose(hwindow)) {
        glfwPollEvents();

        camera.Update();

        /* 计算 MVP 矩阵 */
        glm::mat4 PC_MVP = camera.GetProjectionMatrix() * camera.GetViewMatrix() * glm::mat4(1.0f);

        VkCommandBuffer cmd;
        driver->AcquiredNextFrame(&cmd);

        driver->BeginCommandBuffer(cmd);
        driver->CmdBeginRendering(cmd);
        driver->CmdBindPipeline(cmd, pipeline);
        driver->CmdPushConstants(cmd, pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), glm::value_ptr(PC_MVP));
        driver->CmdBindVertexBuffer(cmd, vertexBuffer, 0);
        driver->CmdDraw(cmd, ARRAY_SIZE(vertices));
        driver->CmdEndRendering(cmd);
        driver->EndCommandBuffer(cmd);
        driver->SubmitAndPresentFrame(cmd);
    }

    driver->DeviceWaitIdle();
    driver->DestroyPipeline(pipeline);
    driver->DestroyBuffer(vertexBuffer);

    glfwDestroyWindow(hwindow);
    glfwTerminate();

    return 0;
}