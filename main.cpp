#include <memory>
#include "driver/render_driver.h"
#include "platform/glfw3/window.h"

#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <direct.h>
#endif

#include <iostream>

#include <stb/stb_image.h>
#include <qk_imgui/qk_imgui.h>

#include "rendering/camera/camera.h"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 uv;
};

Vertex vertices[] = {
    {{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }}, // 左下
    {{  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }}, // 右下
    {{  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }}, // 右上
    {{ -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }}  // 左上
};

uint32_t indices[] = {
    0, 1, 2, // 第一个三角形
    2, 3, 0  // 第二个三角形
};

void InitQkImGui(const std::unique_ptr<RenderDriver>& driver, const std::unique_ptr<Window>& window)
{
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

    QkImGuiVulkanHInit(window->GetWindowHandle(), &_ImGuiVulkanInitInfo);
}

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

    const std::unique_ptr<Window> window = std::make_unique<Window>("Quokka", 1450, 850);
    const std::unique_ptr<RenderDriver> driver = std::make_unique<RenderDriver>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = window->CreateWindowSurface(driver->GetInstance(), VK_NULL_HANDLE, &surface);
    assert(!err);
    driver->Initialize(surface);

    InitQkImGui(driver, window);

    Pipeline pipeline;
    driver->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    size_t vertexBufferSize = sizeof(vertices);
    driver->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &vertexBuffer);
    driver->WriteBuffer(vertexBuffer, vertexBufferSize, vertices);

    Buffer indexBuffer;
    size_t indexBufferSize = sizeof(indices);
    driver->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &indexBuffer);
    driver->WriteBuffer(indexBuffer, indexBufferSize, indices);

    float aspectRatio = driver->GetSwapchainAspectRatio();
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), aspectRatio);

    bool showDemoWindow = true;

    // 离屏渲染
    VkCommandBuffer v2CommandBuffer;
    driver->CreateCommandBuffer(&v2CommandBuffer);

    Texture2D v2Texture;
    ImVec2 watchWSize(32, 32);
    ImVec2 watchVWSize(32, 32);
    driver->CreateTexture2D(watchWSize.x, watchWSize.y, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);

    VkSampler sampler;
    driver->CreateSampler(&sampler);

    VkFence drawFence;
    driver->CreateFence(&drawFence);

    Texture2D quokkaLogoTex0;
    driver->LoadTextureFromFile("../misc/quokka.png", &quokkaLogoTex0);
    driver->WriteTextureDescriptor(pipeline, "tex0", quokkaLogoTex0, sampler);

    Texture2D quokkaLogoTex1;
    driver->LoadTextureFromFile("../misc/quokka_1.png", &quokkaLogoTex1);
    driver->WriteTextureDescriptor(pipeline, "tex1", quokkaLogoTex1, sampler);

    ImTextureID imTextureId = QkImGuiAddTexture(sampler, dGetVkImageView(v2Texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    while (!window->ShouldClose()) {
        glfwPollEvents();

        camera.Update();

        /* 计算 MVP 矩阵 */
        glm::mat4 PC_MVP = camera.GetProjectionMatrix() * camera.GetViewMatrix() * glm::mat4(1.0f);

        if (watchWSize.x != watchVWSize.x || watchWSize.y != watchVWSize.y) {
            watchWSize = watchVWSize;
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
        driver->CmdBindIndexBuffer(v2CommandBuffer, indexBuffer, 0);
        // driver->CmdDraw(v2CommandBuffer, ARRAY_SIZE(vertices));
        driver->CmdDrawIndexed(v2CommandBuffer, 6);

        driver->CmdEndRendering(v2CommandBuffer);
        driver->CmdTextureMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        driver->EndCommandBuffer(v2CommandBuffer);
        driver->SubmitQueue(v2CommandBuffer, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, drawFence);
        driver->WaitForFences(1, &drawFence);

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
                ImVec2 currentVWSize = ImGui::GetContentRegionAvail();
                if (watchVWSize.x != currentVWSize.x || watchVWSize.y != currentVWSize.y)
                    watchVWSize = currentVWSize;
                ImGui::Image(imTextureId, currentVWSize);
                QkImGuiEndViewport();
            }

            if (QkImGuiBegin("调试")) {
                QkImGuiDragFloat3("位置", camera.GetPositionPtr(), 0.01f);
                QkImGuiDragFloat3("方向", camera.GetDirectionPtr(), 0.01f);
                QkImGuiEnd();
            }
            QkImGuiVulkanHEndFrame(cmd);
        }

        driver->CmdEndRendering(cmd);
        driver->CmdTextureMemoryBarrier(cmd, swapChainTexture, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        driver->EndCommandBuffer(cmd);
        driver->SubmitAndPresentFrame(cmd, 0, VK_NULL_HANDLE);
    }

    driver->DeviceWaitIdle();

    QkImGuiVulkanHTerminate();

    driver->DestroyTexture2D(quokkaLogoTex1);
    driver->DestroyTexture2D(quokkaLogoTex0);
    driver->DestroyFence(drawFence);
    driver->DestroyTexture2D(v2Texture);
    driver->DestroySampler(sampler);
    driver->DestroyPipeline(pipeline);
    driver->DestroyBuffer(vertexBuffer);
    driver->DestroyBuffer(indexBuffer);

    return 0;
}