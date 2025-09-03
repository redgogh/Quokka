#ifndef SPIRV_H_
#define SPIRV_H_

#include <spirv/spirv_reflect.h>
// std
#include <vector>

typedef SpvReflectShaderModule SPIRV_ShaderModule;
typedef std::vector<SpvReflectDescriptorSet*> SPIRV_DescriptorSets;
typedef std::vector<SpvReflectInterfaceVariable*> SPIRV_InterfaceVariables;

enum SPIRV_StorageClass
{
    SPIRV_STORAGE_CLASS_INTERFACE_VARIABLE = 0,
    SPIRV_STORAGE_CLASS_INPUT_VARIABLE = 1,
    SPIRV_STORAGE_CLASS_OUTPUT_VARIABLE = 2,
};

void SPIRV_LoadShaderModule(const char* filename, SPIRV_ShaderModule* pShaderModule);
void SPIRV_FreeShaderModule(SPIRV_ShaderModule* pShaderModule);

void SPIRV_EnumerateInterfaceVariables(const SPIRV_ShaderModule* pShaderModule, SPIRV_InterfaceVariables* pInterfaceVariables, SPIRV_StorageClass storageClass = SPIRV_STORAGE_CLASS_INTERFACE_VARIABLE);
void SPIRV_EnumerateInputVariables(const SPIRV_ShaderModule* pShaderModule, SPIRV_InterfaceVariables* pInterfaceVariables);
void SPIRV_EnumerateDescriptorSets(const SPIRV_ShaderModule* pShaderModule, SPIRV_DescriptorSets* pDescriptorSets);

// #include <vulkan/vulkan.h>
#ifdef VK_API_VERSION_1_0
VkFormat SPIRV_ToVkFormat(SpvReflectFormat spvReflectFormat, size_t* pSize)
{
    size_t f_size = sizeof(float);

    switch (spvReflectFormat) {
        case SPV_REFLECT_FORMAT_R32_SFLOAT: {
            *pSize = f_size * 1;
            return VK_FORMAT_R32_SFLOAT;
        }
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:{
            *pSize = f_size * 2;
            return VK_FORMAT_R32G32_SFLOAT;
        }
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: {
            *pSize = f_size * 3;
            return VK_FORMAT_R32G32B32_SFLOAT;
        }
        default:
            throw std::runtime_error("[SPIRV] Invalid SpvReflectFormat -> VkFormat");
    }
}

VkDescriptorType SPIRV_ToVkDescriptorType(SpvReflectDescriptorType spvReflectDescriptorType)
{
    switch (spvReflectDescriptorType) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            throw std::runtime_error("[SPIRV] Invalid SpvReflectDescriptorType -> VkDescriptorType");
    }
}

VkShaderStageFlags SPIRV_ToVkShaderStageFlagBits(SpvReflectShaderStageFlagBits spvReflectShaderStageFlagBits)
{
    switch (spvReflectShaderStageFlagBits) {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
        case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV: return VK_SHADER_STAGE_TASK_BIT_NV;
        case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV: return VK_SHADER_STAGE_MESH_BIT_NV;
        case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR: return VK_SHADER_STAGE_MISS_BIT_KHR;
        case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        default:
            throw std::runtime_error("[SPIRV] Invalid SpvReflectShaderStageFlagBits -> VkShaderStageFlags");
    }
}
#endif /* VK_API_VERSION_1_0 */

#endif /* SPIRV_H_ */
