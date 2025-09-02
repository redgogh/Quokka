#ifndef VKUTILS_H_
#define VKUTILS_H_

#ifndef VULKAN_H_
#include <vulkan/vulkan.h>
#endif /* VULKAN_H_ */

#include <vector>
#include <assert.h>

#include "SPIRV.h"

namespace VkUtils
{
    struct VertexInputState {
        VkPipelineVertexInputStateCreateInfo createInfo;
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkVertexInputBindingDescription binding;
    };

    static VkPhysicalDevice PickBestPhysicalDevice(const VkInstance instance)
    {
        VkResult err = VK_SUCCESS;

        uint32_t count = 0;
        err = vkEnumeratePhysicalDevices(instance, &count, VK_NULL_HANDLE);
        assert(!err);

        std::vector<VkPhysicalDevice> gpuList(count);
        err = vkEnumeratePhysicalDevices(instance, &count, std::data(gpuList));
        assert(!err);

        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        int bestScore = -1;

        for (const VkPhysicalDevice& device : gpuList) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            int score = 0;

            switch (properties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 500; break;
                default:;
            }

            if (score > bestScore) {
                bestScore = score;
                bestDevice = device;
            }
        }

        return bestDevice;
    }

    static uint32_t FindQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, VK_NULL_HANDLE);

        std::vector<VkQueueFamilyProperties> queueFamilies(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, std::data(queueFamilies));

        for (uint32_t i = 0; i < std::size(queueFamilies); i++) {
            VkBool32 isSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isSupport);

            VkQueueFamilyProperties& queueFamily = queueFamilies[i];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && isSupport)
                return i;
        }

        return UINT32_MAX;
    }

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        VkSurfaceFormatKHR chosenSurfaceFormat = {};

        if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
            chosenSurfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
            chosenSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        } else {
            chosenSurfaceFormat = formats[0];
        }

        return chosenSurfaceFormat;
    }

    static void LoadVertexInputState(const char* filename, VertexInputState* state)
    {
        state->createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        state->createInfo.pNext = VK_NULL_HANDLE;

        SPIRV_ShaderModule shaderModule;
        SPIRV_LoadShaderModule(filename, &shaderModule);
        SPIRV_InterfaceVariables vertexInputVariables;
        SPIRV_EnumerateInputVariables(&shaderModule, &vertexInputVariables);

        uint32_t stride = 0;

        for (const auto& vertexInputVariable : vertexInputVariables) {
            size_t typeSize = 0;
            VkVertexInputAttributeDescription vertexInputAttributeDescription = {
                .location = vertexInputVariable->location,
                .binding = 0,
                .format = SPIRV_AsVulkanFormatType(vertexInputVariable->format, &typeSize),
                .offset = stride,
            };
            state->attributes.push_back(vertexInputAttributeDescription);
            stride += static_cast<uint32_t>(typeSize);
        }

        state->binding.binding = 0;
        state->binding.stride = stride;
        state->binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        state->createInfo.vertexBindingDescriptionCount = 1;
        state->createInfo.pVertexBindingDescriptions = &state->binding;
        state->createInfo.vertexAttributeDescriptionCount = std::size(state->attributes);
        state->createInfo.pVertexAttributeDescriptions = std::data(state->attributes);

        SPIRV_FreeShaderModule(&shaderModule);
    }

}

#endif /* VKUTILS_H_ */
