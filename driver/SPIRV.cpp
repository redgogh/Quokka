#include "SPIRV.h"

// std
#include <fstream>

uint32_t* _SPIRV_LoadSPV(const char* filename, size_t* pWordCount)
{
    std::ifstream spvFile(filename, std::ios::binary | std::ios::ate);

    if (!spvFile.is_open())
        throw std::runtime_error("[SPIRV] Failed to open spv file");

    size_t byteSize = spvFile.tellg();
    if (byteSize % 4 != 0)
        throw std::runtime_error("[SPIRV] File size must be a multiple of 4");

    size_t wordSize = byteSize / 4;
    spvFile.seekg(0);

    uint32_t* buffer = static_cast<uint32_t*>(malloc(byteSize));
    if (!buffer)
        throw std::runtime_error("[SPIRV] Failed to malloc buffer");

    spvFile.read(reinterpret_cast<char*>(buffer), byteSize);
    if (!spvFile) {
        free(buffer);
        throw std::runtime_error("[SPIRV] Failed to read full file");
    }

    *pWordCount = wordSize;
    return buffer;
}

void _SPIRV_FreeSPV(uint32_t* buffer)
{
    if (!buffer)
        free(buffer);
}

void SPIRV_LoadShaderModule(const char* filename, SPIRV_ShaderModule* pShaderModule)
{
    size_t wordSize = 0;
    uint32_t* spvBuffer = _SPIRV_LoadSPV(filename, &wordSize);
    spvReflectCreateShaderModule(wordSize * 4, spvBuffer, pShaderModule);
    _SPIRV_FreeSPV(spvBuffer);
}

void SPIRV_FreeShaderModule(SPIRV_ShaderModule* pShaderModule)
{
    spvReflectDestroyShaderModule(pShaderModule);
}

void SPIRV_EnumerateInterfaceVariables(const SPIRV_ShaderModule* pShaderModule, SPIRV_InterfaceVariables* pInterfaceVariables, SPIRV_StorageClass storageClass)
{
    uint32_t count = 0;

    switch (storageClass) {
        case SPIRV_STORAGE_CLASS_INTERFACE_VARIABLE: {
            spvReflectEnumerateInterfaceVariables(pShaderModule, &count, nullptr);
            pInterfaceVariables->resize(count);
            spvReflectEnumerateInterfaceVariables(pShaderModule, &count, std::data(*pInterfaceVariables));
            break;
        }
        case SPIRV_STORAGE_CLASS_INPUT_VARIABLE: {
            spvReflectEnumerateInputVariables(pShaderModule, &count, nullptr);
            pInterfaceVariables->resize(count);
            spvReflectEnumerateInputVariables(pShaderModule, &count, std::data(*pInterfaceVariables));
            break;
        }
        case SPIRV_STORAGE_CLASS_OUTPUT_VARIABLE: {
            spvReflectEnumerateOutputVariables(pShaderModule, &count, nullptr);
            pInterfaceVariables->resize(count);
            spvReflectEnumerateOutputVariables(pShaderModule, &count, std::data(*pInterfaceVariables));
            break;
        }
    }
}

void SPIRV_EnumerateInputVariables(const SPIRV_ShaderModule* pShaderModule, SPIRV_InterfaceVariables* pInterfaceVariables)
{
    SPIRV_EnumerateInterfaceVariables(pShaderModule, pInterfaceVariables, SPIRV_STORAGE_CLASS_INPUT_VARIABLE);
}

void SPIRV_EnumerateDescriptorSets(const SPIRV_ShaderModule* pShaderModule, SPIRV_DescriptorSets* pDescriptorSets)
{
    uint32_t count = 0;
    spvReflectEnumerateDescriptorSets(pShaderModule, &count, nullptr);

    if (count > 0) {
        pDescriptorSets->resize(count);
        spvReflectEnumerateDescriptorSets(pShaderModule, &count, std::data(*pDescriptorSets));
    }
}