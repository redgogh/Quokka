#ifndef QK_IMGUI_H_
#define QK_IMGUI_H_

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

void QkImGuiVulkanHInit(GLFWwindow* window, ImGui_ImplVulkan_InitInfo* info);
void QkImGuiVulkanHTerminate();

void QkImGuiVulkanHNewFrame([[maybe_unused]] VkCommandBuffer commandBuffer);
void QkImGuiVulkanHEndFrame(VkCommandBuffer commandBuffer);

bool QkImGuiBegin(const char *title, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
void QkImGuiEnd();

// widgets
bool QkImGuiDragFloat(const char *label, float *v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");
bool QkImGuiDragFloat2(const char *label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");
bool QkImGuiDragFloat3(const char *label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");
bool QkImGuiDragFloat4(const char *label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");
void QkImGuiColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
void QkImGuiSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.2f", ImGuiSliderFlags flags = 0);


#endif /* QK_IMGUI_H_ */
