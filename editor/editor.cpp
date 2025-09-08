#include "editor.h"

static RenderDriver* DRV = VK_NULL_HANDLE;
static Window* WND = VK_NULL_HANDLE;
static VkSampler sampler2D = VK_NULL_HANDLE;

void GameEditor::Initialize(RenderDriver* pDriver, Window* pWindow)
{
    DRV = pDriver;
    WND = pWindow;
    
    const VkFormat colorAttachmentFormats[] = {
        DRV->GetSwapchainFormat()
    };

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo = {};
    ImGuiVulkanInitInfo.Instance = DRV->GetInstance();
    ImGuiVulkanInitInfo.PhysicalDevice = DRV->GetPhysicalDevice();
    ImGuiVulkanInitInfo.Device = DRV->GetDevice();
    ImGuiVulkanInitInfo.QueueFamily = DRV->GetQueueFamilyIndex();
    ImGuiVulkanInitInfo.Queue = DRV->GetGraphicsQueue();
    ImGuiVulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    ImGuiVulkanInitInfo.DescriptorPool = DRV->GetDescriptorPool();
    ImGuiVulkanInitInfo.UseDynamicRendering = VK_TRUE;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    ImGuiVulkanInitInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    ImGuiVulkanInitInfo.MinImageCount = DRV->GetMinImageCount();
    ImGuiVulkanInitInfo.ImageCount = DRV->GetMinImageCount();
    ImGuiVulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    QkImGuiVulkanHInit(WND->GetWindowHandle(), &ImGuiVulkanInitInfo);

    /* 初始化默认采样器 */
    DRV->CreateSampler(&sampler2D);
}

void GameEditor::Terminate()
{
    QkImGuiVulkanHTerminate();

    /* 销毁默认采样器 */
    DRV->DestroySampler(sampler2D);
}

void GameEditor::BeginNewFrame(VkCommandBuffer commandBuffer)
{
    QkImGuiVulkanHNewFrame(commandBuffer);
}

void GameEditor::EndNewFrame(VkCommandBuffer commandBuffer)
{
    QkImGuiVulkanHEndFrame(commandBuffer);
}

void GameEditor::CreateImTextureID(Texture texture, ImTextureID* pImTextureId)
{
    *pImTextureId = QkImGuiAddTexture(sampler2D, dGetVkImageView(texture), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GameEditor::DestroyImTextureID(ImTextureID textureId)
{
    QkImGuiRemoveTexture(textureId);
}

void GameEditor::DrawImage(ImTextureID textureId, const ImVec2& size)
{
    ImGui::Image(textureId, size);
}

void GameEditor::ShowDemoWindow()
{
    static bool open = true;
    ImGui::ShowDemoWindow(&open);
}

