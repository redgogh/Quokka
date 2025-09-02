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

#ifdef VK_API_VERSION_1_0
VkFormat SPIRV_AsVulkanFormatType(SpvReflectFormat spvReflectFormat, size_t* pSize)
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
            throw std::runtime_error("[SPIRV] Invalid spvReflectFormat -> VkFormat");
    }
}
#endif /* VK_API_VERSION_1_0 */

#endif /* SPIRV_H_ */
