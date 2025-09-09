#include "editor.h"

#include <quokka/qk_format.h>

GameEditor::GameEditor(RenderDevice* pDevice, Window* pWindow)
    : device(pDevice), hwnd(pWindow)
{
    const VkFormat colorAttachmentFormats[] = {
        device->GetSwapchainFormat()
    };

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo = {};
    ImGuiVulkanInitInfo.Instance = device->GetInstance();
    ImGuiVulkanInitInfo.PhysicalDevice = device->GetPhysicalDevice();
    ImGuiVulkanInitInfo.Device = device->GetDevice();
    ImGuiVulkanInitInfo.QueueFamily = device->GetQueueFamilyIndex();
    ImGuiVulkanInitInfo.Queue = device->GetGraphicsQueue();
    ImGuiVulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    ImGuiVulkanInitInfo.DescriptorPool = device->GetDescriptorPool();
    ImGuiVulkanInitInfo.UseDynamicRendering = VK_TRUE;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    ImGuiVulkanInitInfo.MinImageCount = device->GetMinImageCount();
    ImGuiVulkanInitInfo.ImageCount = device->GetMinImageCount();
    ImGuiVulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    QkImGuiVulkanHInit(hwnd->GetWindowHandle(), &ImGuiVulkanInitInfo);
}

GameEditor::~GameEditor()
{
    QkImGuiVulkanHTerminate();
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
    return QkImGuiAddTexture(device->GetLinearRepeatSampler(), dGetVkImageView(texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GameEditor::DestroyTextureId(ImTextureID textureId)
{
    QkImGuiRemoveTexture(textureId);
}

void GameEditor::ShowDemoWindow()
{
    static bool open = true;
    ImGui::ShowDemoWindow(&open);
}

void GameEditor::DrawFPS(uint32_t fps)
{
    ImVec2 wpos = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    std::string _text = qk_format("FPS: %u", fps).c_str();
    const char* fpsText = _text.c_str();
    ImVec2 textSize = ImGui::CalcTextSize(fpsText);

    ImU32 textColor = IM_COL32(0, 255, 0, 255);
    float xOffset = wpos.x + wsize.x - textSize.x - 25;
    float yOffset = wpos.y + textSize.y + 20;
    drawList->AddText(ImVec2(xOffset, yOffset), textColor, fpsText);
}