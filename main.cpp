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

#include <qk_imgui/qk_imgui.h>

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

    setbuf(stdout, NULL);

    glfwInit();

//    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* hwindow =
        glfwCreateWindow(1480, 890, "Quokka", nullptr, nullptr);

    if (hwindow == nullptr)
        throw std::runtime_error("Failed to create GLFW window");

    const std::unique_ptr<RenderDriver> driver = std::make_unique<RenderDriver>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(driver->GetInstance(), hwindow, VK_NULL_HANDLE, &surface);
    assert(!err);
    driver->Initialize(surface);

    VkFormat colorAttachmentFormats[] = {
        driver->GetSwapchainFormat()
    };

    ImGui_ImplVulkan_InitInfo _ImGuiVulkanInitInfo = {};
    _ImGuiVulkanInitInfo.Instance = driver->GetInstance();
    _ImGuiVulkanInitInfo.PhysicalDevice = driver->GetPhysicalDevice();
    _ImGuiVulkanInitInfo.Device = driver->GetDevice();
    _ImGuiVulkanInitInfo.QueueFamily = driver->GetQueueFamilyIndex();
    _ImGuiVulkanInitInfo.Queue = driver->GetGraphicsQueue();
    _ImGuiVulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    _ImGuiVulkanInitInfo.DescriptorPool = driver->GetDescriptorPool();
    _ImGuiVulkanInitInfo.UseDynamicRendering = VK_TRUE;
    _ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    _ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    _ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    _ImGuiVulkanInitInfo.MinImageCount = driver->GetMinImageCount();
    _ImGuiVulkanInitInfo.ImageCount = driver->GetMinImageCount();
    _ImGuiVulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    QkImGuiVulkanHInit(hwindow, &_ImGuiVulkanInitInfo);

    Pipeline pipeline;
    driver->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    size_t vertexBufferSize = sizeof(vertices);
    driver->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &vertexBuffer);
    driver->WriteBuffer(vertexBuffer, vertexBufferSize, vertices);

    glm::vec3 position(0.0f, 0.0f, 3.0f);
    float aspectRatio = driver->GetSwapchainAspectRatio();
    Camera camera(position, aspectRatio);

    bool showDemoWindow = true;

    // 离屏渲染
    VkCommandBuffer v2CommandBuffer;
    driver->CreateCommandBuffer(&v2CommandBuffer);

    Texture2D v2Texture;
    ImVec2 watchWSize(32, 32);
    ImVec2 viewportWSize(32, 32);
    driver->CreateTexture2D(watchWSize.x, watchWSize.y, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);

    VkSampler sampler;
    driver->CreateSampler(&sampler);

    ImTextureID imTextureId = QkImGuiAddTexture(sampler, dGetVkImageView(v2Texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    while (!glfwWindowShouldClose(hwindow)) {
        glfwPollEvents();

        camera.Update();

        /* 计算 MVP 矩阵 */
        glm::mat4 PC_MVP = camera.GetProjectionMatrix() * camera.GetViewMatrix() * glm::mat4(1.0f);

        if (watchWSize.x != viewportWSize.x || watchWSize.y != viewportWSize.y) {
            watchWSize = viewportWSize;
            camera.SetAspectRatio(watchWSize.x / watchWSize.y);
            driver->DeviceWaitIdle();
            QkImGuiRemoveTexture(imTextureId);
            driver->DestroyTexture2D(v2Texture);
            driver->CreateTexture2D(watchWSize.x, watchWSize.y, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);
            imTextureId = QkImGuiAddTexture(sampler, dGetVkImageView(v2Texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        driver->BeginCommandBuffer(v2CommandBuffer);
        driver->CmdTextureMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        driver->CmdBeginRendering(v2CommandBuffer, v2Texture);

        driver->CmdBindPipeline(v2CommandBuffer, pipeline, watchWSize.x, watchWSize.y);
        driver->CmdPushConstants(v2CommandBuffer, pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),glm::value_ptr(PC_MVP));
        driver->CmdBindVertexBuffer(v2CommandBuffer, vertexBuffer, 0);
        driver->CmdDraw(v2CommandBuffer, ARRAY_SIZE(vertices));

        driver->CmdEndRendering(v2CommandBuffer);
        driver->CmdTextureMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        driver->EndCommandBuffer(v2CommandBuffer);
        driver->SubmitQueue(v2CommandBuffer, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
        driver->DeviceWaitIdle();

        VkCommandBuffer cmd;
        Texture2D swapChainTexture;
        driver->AcquiredNextFrame(&cmd, &swapChainTexture);
        driver->BeginCommandBuffer(cmd);
        driver->CmdTextureMemoryBarrier(cmd, swapChainTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        driver->CmdBeginRendering(cmd, swapChainTexture);

        {
            QkImGuiVulkanHNewFrame(cmd);
            ImGui::ShowDemoWindow(&showDemoWindow);
            if (QkImGuiBeginViewport("视口")) {
                ImVec2 currentRegion = ImGui::GetContentRegionAvail();
                if (viewportWSize.x != currentRegion.x && viewportWSize.y != currentRegion.y) {
                    viewportWSize.x = currentRegion.x;
                    viewportWSize.y = currentRegion.y;
                }
                ImGui::Image(imTextureId, currentRegion);
                QkImGuiEndViewport();
            }

            if (QkImGuiBegin("调试")) {

                if (QkImGuiDragFloat3("位置", glm::value_ptr(position), 0.01f)) {
                    camera.SetPosition(position);
                }

                QkImGuiEnd();
            }
            QkImGuiVulkanHEndFrame(cmd);
        }

        driver->CmdEndRendering(cmd);
        driver->CmdTextureMemoryBarrier(cmd, swapChainTexture, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        driver->EndCommandBuffer(cmd);
        driver->SubmitAndPresentFrame(cmd);
    }

    driver->DeviceWaitIdle();

    QkImGuiVulkanHTerminate();

    driver->DestroyTexture2D(v2Texture);
    driver->DestroySampler(sampler);
    driver->DestroyPipeline(pipeline);
    driver->DestroyBuffer(vertexBuffer);

    glfwDestroyWindow(hwindow);
    glfwTerminate();

    return 0;
}