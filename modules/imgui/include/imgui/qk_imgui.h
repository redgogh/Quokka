#ifndef QK_IMGUI_H_
#define QK_IMGUI_H_

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

void QkImGuiVulkanHInit(ImGui_ImplVulkan_InitInfo* info);
void QkImGuiVulkanHTerminate();

void QkImGuiVulkanHNewFrame([[maybe_unused]] VkCommandBuffer commandBuffer);
void QkImGuiVulkanHEndFrame(VkCommandBuffer commandBuffer);

#endif /* QK_IMGUI_H_ */
