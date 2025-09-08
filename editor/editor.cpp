#include "editor.h"

GameEditor::GameEditor(RenderDriver* pDriver, Window* pWindow)
    : driver(pDriver), hwnd(pWindow)
{
    const VkFormat colorAttachmentFormats[] = {
        driver->GetSwapchainFormat()
    };

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo = {};
    ImGuiVulkanInitInfo.Instance = driver->GetInstance();
    ImGuiVulkanInitInfo.PhysicalDevice = driver->GetPhysicalDevice();
    ImGuiVulkanInitInfo.Device = driver->GetDevice();
    ImGuiVulkanInitInfo.QueueFamily = driver->GetQueueFamilyIndex();
    ImGuiVulkanInitInfo.Queue = driver->GetGraphicsQueue();
    ImGuiVulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    ImGuiVulkanInitInfo.DescriptorPool = driver->GetDescriptorPool();
    ImGuiVulkanInitInfo.UseDynamicRendering = VK_TRUE;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    ImGuiVulkanInitInfo.MinImageCount = driver->GetMinImageCount();
    ImGuiVulkanInitInfo.ImageCount = driver->GetMinImageCount();
    ImGuiVulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    QkImGuiVulkanHInit(hwnd->GetWindowHandle(), &ImGuiVulkanInitInfo);

    /* 初始化默认采样器 */
    driver->CreateSampler(&sampler2D);
}

GameEditor::~GameEditor()
{
    QkImGuiVulkanHTerminate();

    /* 销毁默认采样器 */
    driver->DestroySampler(sampler2D);
}

void GameEditor::BeginNewFrame(VkCommandBuffer commandBuffer)
{
    QkImGuiVulkanHNewFrame(commandBuffer);

    /* 渲染 viewport 窗口 */
    for (const auto &viewport: viewports) {
        if (QkImGuiBeginViewport(viewport.first.c_str())) {
            viewport.second.drawFunc();
            QkImGuiEndViewport();
        }
    }

    /* 渲染普通窗口 */
    for (const auto &window: windows) {
        if (QkImGuiBegin(window.first.c_str())) {
            window.second.drawFunc();
            QkImGuiEnd();
        }
    }
}

void GameEditor::EndFrame(VkCommandBuffer commandBuffer)
{
    QkImGuiVulkanHEndFrame(commandBuffer);
}

void GameEditor::ShowDemoWindow()
{
    static bool open = true;
    ImGui::ShowDemoWindow(&open);
}

void GameEditor::RegisterWindow(const std::string &name, std::function<void()> drawFunc)
{
    windows[name] = { name, drawFunc, true };
}

void GameEditor::RegisterViewport(const std::string &name, std::function<void()> drawFunc)
{
    viewports[name] = { name, drawFunc, true };
}

ImTextureID GameEditor::CreateTextureId(Texture texture)
{
    return QkImGuiAddTexture(sampler2D, dGetVkImageView(texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GameEditor::DestroyTextureId(ImTextureID textureId)
{
    QkImGuiRemoveTexture(textureId);
}