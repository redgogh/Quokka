#define VK_NO_PROTOTYPES
#define VMA_IMPLEMENTATION

#include "render_device.h"

#include <stdio.h>
#include "vkutils.h"
#include "stb/stb_image.h"
#include "utils/ioutils.h"
#include <quokka/qk_format.h>

#define VK_VERSION_1_3_216

#define VK_CHECK_ERROR(err) \
    if (err != VK_SUCCESS) \
        return err;

/* volk 全局只初始化一次 */
static bool volkInitialized = false;

struct QVkTexture {
    VkImage vkImage = VK_NULL_HANDLE;
    VkImageView vkImageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct QVkBuffer {
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
    VkDeviceSize size = 0;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
    VmaAllocationInfo allocationInfo;
};

struct QVkPipeline {
    VkPipeline vkPipeline = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
    std::vector<VkDescriptorSet> vkDescriptorSets;
    typedef struct {
        std::string name;
        uint32_t set;
        uint32_t binding;
        VkDescriptorSet vkDescriptorSet;
    } DescriptorSetInfo;
    std::unordered_map<std::string, DescriptorSetInfo> descriptorSetInfos;
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

VkImage dGetVkImage(Texture texture)
{
    return texture->vkImage;
}

VkImageView dGetVkImageView(Texture texture)
{
    return texture->vkImageView;
}

RenderDevice::RenderDevice()
{
    VkResult err;

#ifdef USE_VOLK_LOADER
    if (!volkInitialized) {
        err = volkInitialize();
        assert(!err);
        volkInitialized = true;
    }
#endif /* USE_VOLK_LOADER */

    err = _CreateInstance();
    assert(!err);

#ifdef USE_VOLK_LOADER
    volkLoadInstance(instance);
#endif /* USE_VOLK_LOADER */

    uint32_t version = 0;
    err = vkEnumerateInstanceVersion(&version);

    if (!err) {
        printf("[vulkan] instance version supported: %d.%d.%d\n",
            VK_VERSION_MAJOR(version),
            VK_VERSION_MINOR(version),
            VK_VERSION_PATCH(version));
    } else {
        printf("[vulkan] instance version is <= 1.0");
    }
}

RenderDevice::~RenderDevice()
{
    vkDeviceWaitIdle(device);

    _DestroyPerInitSamplers();

    DestroyFence(submitFence);
    _DestroySyncObjects();
    vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(device, commandPool, VK_NULL_HANDLE);
    // vkDestroySwapchainKHR(device, swapchain, VK_NULL_HANDLE);
    _DestroySwapchain();
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, VK_NULL_HANDLE);
    vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
    vkDestroyInstance(instance, VK_NULL_HANDLE);
}

VkResult RenderDevice::Initialize(VkSurfaceKHR surface)
{
    VkResult err;

    this->surface = surface;

    err = _CreateDevice();
    VK_CHECK_ERROR(err);

    err = _CreateSwapchain(VK_NULL_HANDLE);
    VK_CHECK_ERROR(err);

    err = _CreateCommandPool();
    VK_CHECK_ERROR(err);

    err = _CreateDescriptorPool();
    VK_CHECK_ERROR(err);

    err = _CreateMemoryAllocator();
    VK_CHECK_ERROR(err);

    err = _InitSyncObjects();
    VK_CHECK_ERROR(err);

    err = CreateFence(&submitFence);
    VK_CHECK_ERROR(err);

    err = _PerInitSamplers();
    VK_CHECK_ERROR(err);

    printf("[vulkan] render device for vulkan initialized\n");
    printf("[vulkan]   - VkInstance: %p\n", instance);
    printf("[vulkan]   - VkPhysicalDevice: %p\n", physicalDevice);
    printf("[vulkan]   - VkSurface: %p\n", surface);
    printf("[vulkan]   - VkDevice: %p\n", device);
    printf("[vulkan]   - VkCommandPool: %p\n", commandPool);
    printf("[vulkan]   - VkDescriptorPool: %p\n", descriptorPool);
    printf("[vulkan]   - VmaAllocator: %p\n", allocator);

    return err;
}

VkResult RenderDevice::CreateBuffer(const size_t size, VkBufferUsageFlags usage, Buffer *pBuffer)
{
    VkResult err;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = _GuessMemoryUsage(usage);

    *pBuffer = new QVkBuffer;

    err = vmaCreateBuffer(allocator,
                          &bufferCreateInfo,
                          &allocationCreateInfo,
                          &(*pBuffer)->vkBuffer,
                          &(*pBuffer)->allocation,
                          &(*pBuffer)->allocationInfo);
    VK_CHECK_ERROR(err);

    (*pBuffer)->usage = usage;
    (*pBuffer)->size = size;
    (*pBuffer)->memoryUsage = allocationCreateInfo.usage;

    return err;
}

VkResult RenderDevice::CreateVertexBuffer(size_t size, const void *data, Buffer *pVertexBuffer)
{
    VkResult err;

    VkBufferUsageFlags flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    err = CreateBuffer(size, flags, pVertexBuffer);
    VK_CHECK_ERROR(err);

    WriteBuffer(*pVertexBuffer, size, data);

    return err;
}

VkResult RenderDevice::CreateIndexBuffer(size_t size, const void *data, Buffer *pIndexBuffer)
{
    VkResult err;

    VkBufferUsageFlags flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    err = CreateBuffer(size, flags, pIndexBuffer);
    VK_CHECK_ERROR(err);

    WriteBuffer(*pIndexBuffer, size, data);

    return err;
}

void RenderDevice::DestroyBuffer(Buffer buffer)
{
    vmaDestroyBuffer(allocator, buffer->vkBuffer, buffer->allocation);
    delete buffer;
}

VkResult RenderDevice::CreateTexture(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, Texture *pTexture)
{
    VkResult err;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.extent.depth = 1.0f;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.usage = (usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    err = vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo);
    VK_CHECK_ERROR(err);

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    err = vkCreateImageView(device, &imageViewCreateInfo, VK_NULL_HANDLE, &imageView);
    VK_CHECK_ERROR(err);

    *pTexture = new QVkTexture;

    (*pTexture)->vkImage = image;
    (*pTexture)->vkImageView = imageView;
    (*pTexture)->allocation = allocation;
    (*pTexture)->allocationInfo = allocationInfo;
    (*pTexture)->width = w;
    (*pTexture)->height = h;
    (*pTexture)->format = format;
    (*pTexture)->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return err;
}

void RenderDevice::DestroyTexture(Texture texture2D)
{
    vmaDestroyImage(allocator, texture2D->vkImage, texture2D->allocation);
    vkDestroyImageView(device, texture2D->vkImageView, VK_NULL_HANDLE);
    delete texture2D;
}

VkResult RenderDevice::CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, bool enableAnisotropy, float maxAnisotropy, bool useMipmaps, VkSampler* pSampler)
{
    VkResult err;

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = filter;
    samplerCreateInfo.minFilter = filter;
    samplerCreateInfo.addressModeU = addressMode;
    samplerCreateInfo.addressModeV = addressMode;
    samplerCreateInfo.addressModeW = addressMode;
    samplerCreateInfo.anisotropyEnable = enableAnisotropy ? VK_TRUE : VK_FALSE;
    samplerCreateInfo.maxAnisotropy = maxAnisotropy;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.mipmapMode = useMipmaps ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = useMipmaps ? VK_LOD_CLAMP_NONE : 0.0f;

    err = vkCreateSampler(device, &samplerCreateInfo, VK_NULL_HANDLE, pSampler);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroySampler(VkSampler sampler)
{
    vkDestroySampler(device, sampler, VK_NULL_HANDLE);
}

VkResult RenderDevice::CreatePipeline(const char *shaderName, Pipeline* pPipeline)
{
    VkResult err;

    printf("[vulkan] create graphics pipeline: %s\n", shaderName);

    /* -------------------------------------------------------- */
    /*                      Shader Module                       */
    /* -------------------------------------------------------- */
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

    char vertexShaderPath[PATH_MAX];
    snprintf(vertexShaderPath, sizeof(vertexShaderPath), "%s.%s.spv", shaderName, "vert");
    err = _CreateShaderModule(vertexShaderPath, &vertexShaderModule);
    VK_CHECK_ERROR(err);

    char fragmentShaderPath[PATH_MAX];
    snprintf(fragmentShaderPath, sizeof(fragmentShaderPath), "%s.%s.spv", shaderName, "frag");
    err = _CreateShaderModule(fragmentShaderPath, &fragmentShaderModule);
    VK_CHECK_ERROR(err);

    /* VkPipelineShaderStageCreateInfo */
    VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShaderModule,
            .pName = "main",
        }
    };

    /* -------------------------------------------------------- */
    /*                 Pipeline Layout Create                   */
    /* -------------------------------------------------------- */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkUtils::DescriptorSetLayoutInfo descriptorSetLayoutInfo;
    VkUtils::LoadDescriptorSetLayoutInfo(vertexShaderPath, &descriptorSetLayoutInfo);
    VkUtils::LoadDescriptorSetLayoutInfo(fragmentShaderPath, &descriptorSetLayoutInfo);

    std::vector<VkDescriptorSetLayout> setLayouts(std::size(descriptorSetLayoutInfo.bindingPerSet));
    for (auto& [setIndex, layoutBindings] : descriptorSetLayoutInfo.bindingPerSet) {
        CreateDescriptorSetLayout(std::size(layoutBindings), std::data(layoutBindings), &setLayouts[setIndex]);
    }

    std::vector<VkDescriptorSet> sets(std::size(setLayouts));
    CreateDescriptorSets(std::size(setLayouts), std::data(setLayouts), std::data(sets));

    printf("[vulkan]    build descriptor set info:\n");
    std::unordered_map<std::string, QVkPipeline::DescriptorSetInfo> descriptorSetInfos;
    for (auto& [name, location] : descriptorSetLayoutInfo.nameToBinding) {
        QVkPipeline::DescriptorSetInfo descriptorSetInfo;
        descriptorSetInfo.name = name;
        descriptorSetInfo.set = location.set;
        descriptorSetInfo.binding = location.binding;
        descriptorSetInfo.vkDescriptorSet = sets[location.set];
        descriptorSetInfos[name] = descriptorSetInfo;
        printf("[vulkan]      - layout(set = %u, binding = %u) %p %s\n", location.set, location.binding, sets[location.set], name.c_str());
    }

    pipelineLayoutInfo.setLayoutCount = std::size(setLayouts);
    pipelineLayoutInfo.pSetLayouts = std::data(setLayouts);

    // VkPushConstantRange pushConstantRange = {};
    // pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // pushConstantRange.offset = 0;
    // pushConstantRange.size = sizeof(float) * 16;

    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    // pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    err = vkCreatePipelineLayout(device, &pipelineLayoutInfo, VK_NULL_HANDLE, &pipelineLayout);
    VK_CHECK_ERROR(err);

    /* -------------------------------------------------------- */
    /*                 Pipeline Configuration                   */
    /* -------------------------------------------------------- */
    VkUtils::VertexInputState vertexInputState;
    VkUtils::LoadVertexInputState(vertexShaderPath, &vertexInputState);
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = vertexInputState.createInfo;

    /* VkPipelineInputAssemblyStateCreateInfo */
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    /* VkPipelineViewportStateCreateInfo */
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    /* VkPipelineRasterizationStateCreateInfo */
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;                   // 超出深度范围裁剪而不是 clamp
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;            // 不丢弃几何体
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;            // 填充多边形方式点、线、面
    rasterizationStateCreateInfo.lineWidth = 1.0f;                              // 线宽
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;              // 背面剔除，可改 NONE 或 FRONT
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;           // 前向面定义
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;                    // 不使用深度偏移
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    /* VkPipelineMultisampleStateCreateInfo */
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;                  // 关闭样本着色
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;    // 每像素采样数，1 = 关闭 MSAA
    multisampleStateCreateInfo.minSampleShading = 1.0f;                         // 如果开启 sampleShading，最小采样比例
    multisampleStateCreateInfo.pSampleMask = VK_NULL_HANDLE;                    // 默认全开
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;                // alpha to coverage 禁用
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;                     // alphaToOne 禁用

    /* VkPipelineColorBlendStateCreateInfo */
    VkPipelineColorBlendAttachmentState colorBlendAttachmentStage = {};
    colorBlendAttachmentStage.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentStage.blendEnable = VK_FALSE;                           // 关闭混合

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;                         // 不使用逻辑操作
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;                       // 无效，因为逻辑操作关闭
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentStage;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    /* VkPipelineDynamicStateCreateInfo[] */
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = ARRAY_SIZE(dynamicStates);
    dynamicStateCreateInfo.pDynamicStates = &dynamicStates[0];

    /* dynamic rendering */
    VkFormat formatArr[] = { VK_FORMAT_B8G8R8A8_UNORM };

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = ARRAY_SIZE(formatArr);
    pipelineRenderingInfo.pColorAttachmentFormats = formatArr;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = &pipelineRenderingInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStagesCreateInfo);
    pipelineCreateInfo.pStages = shaderStagesCreateInfo;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pTessellationState = VK_NULL_HANDLE;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = VK_NULL_HANDLE;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &pipeline);
    VK_CHECK_ERROR(err);

    vkDestroyShaderModule(device, vertexShaderModule, VK_NULL_HANDLE);
    vkDestroyShaderModule(device, fragmentShaderModule, VK_NULL_HANDLE);

    Pipeline ret = new QVkPipeline;
    ret->vkPipeline = pipeline;
    ret->vkDescriptorSetLayouts = setLayouts;
    ret->vkDescriptorSets = sets;
    ret->descriptorSetInfos = descriptorSetInfos;
    ret->vkPipelineLayout = pipelineLayout;
    ret->vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    *pPipeline = ret;

    return err;
}

void RenderDevice::DestroyPipeline(Pipeline pipeline)
{
    vkDestroyPipeline(device, pipeline->vkPipeline, VK_NULL_HANDLE);
    DestroyDescriptorSets(std::size(pipeline->vkDescriptorSets), std::data(pipeline->vkDescriptorSets));

    for (auto& setLayout : pipeline->vkDescriptorSetLayouts)
        vkDestroyDescriptorSetLayout(device, setLayout, VK_NULL_HANDLE);

    vkDestroyPipelineLayout(device, pipeline->vkPipelineLayout, VK_NULL_HANDLE);
    delete pipeline;
}

VkResult RenderDevice::CreateCommandBuffer(VkCommandBuffer *pCommandBuffer)
{
    VkResult err;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = commandPool;

    err = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffer);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroyCommandBuffer(VkCommandBuffer commandBuffer)
{
    DestroyCommandBuffers(1, &commandBuffer);
}

void RenderDevice::DestroyCommandBuffers(uint32_t count, const VkCommandBuffer* pCommandBuffers)
{
    vkFreeCommandBuffers(device, commandPool, count, pCommandBuffers);
}

VkResult RenderDevice::CreateFence(VkFence *pFence)
{
    VkResult err;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    err = vkCreateFence(device, &fenceCreateInfo, VK_NULL_HANDLE, pFence);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroyFence(VkFence fence)
{
    vkDestroyFence(device, fence, VK_NULL_HANDLE);
}

VkResult RenderDevice::CreateSemaphore(VkSemaphore *pSemaphore)
{
    VkResult err;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    err = vkCreateSemaphore(device, &semaphoreCreateInfo, VK_NULL_HANDLE, pSemaphore);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroySemaphore(VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);
}

VkResult RenderDevice::CreateDescriptorSetLayout(uint32_t bindingCount, const VkDescriptorSetLayoutBinding *pBindings, VkDescriptorSetLayout* pDescriptorSetLayout)
{
    VkResult err;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
    descriptorSetLayoutCreateInfo.pBindings = pBindings;

    err = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, pDescriptorSetLayout);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, VK_NULL_HANDLE);
}

VkResult RenderDevice::CreateDescriptorSets(uint32_t descriptorSetCount, const VkDescriptorSetLayout* pDescriptorSetLayouts, VkDescriptorSet *pDescriptorSets)
{
    VkResult err;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
    descriptorSetAllocateInfo.pSetLayouts = pDescriptorSetLayouts;

    err = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, pDescriptorSets);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::DestroyDescriptorSets(uint32_t descriptorSetCount, VkDescriptorSet* pDescriptorSets)
{
    vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void RenderDevice::BeginSingleTimeCommandBuffer(VkCommandBuffer *pCommandBuffer)
{
    CreateCommandBuffer(pCommandBuffer);
    BeginCommandBuffer(*pCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void RenderDevice::EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer)
{
    EndCommandBuffer(commandBuffer);
    SubmitQueue(commandBuffer, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    DestroyCommandBuffers(1, &commandBuffer);
}

void RenderDevice::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void RenderDevice::EndCommandBuffer(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);
}

void RenderDevice::CmdMemoryBarrier(VkCommandBuffer commandBuffer, Texture texture, VkImageLayout newLayout)
{
    VkImageLayout oldLayout = texture->layout;

    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        || oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStageMask = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) ?
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstAccessMask = 0;
        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        goto DO_MEMORY_IAMGE_BARRIER_TAG;
    }

    printf("[vulkan] error - unsupported image layout transition!\n");
    return;

DO_MEMORY_IAMGE_BARRIER_TAG:
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture->vkImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    vkCmdPipelineBarrier(commandBuffer,
                         srcStageMask,
                         dstStageMask,
                         0,
                         0, VK_NULL_HANDLE,
                         0, VK_NULL_HANDLE,
                         1, &barrier);

    texture->layout = newLayout;
}

void RenderDevice::CmdBeginRendering(VkCommandBuffer commandBuffer, Texture texture)
{
    VkRenderingAttachmentInfo colorRenderingAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = texture->vkImageView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = { 0.2f, 0.2f, 0.2f, 1.0f }
        }
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = { texture->width, texture->height }
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRenderingAttachment
    };

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void RenderDevice::CmdEndRendering(VkCommandBuffer commandBuffer)
{
    vkCmdEndRendering(commandBuffer);
}

void RenderDevice::CmdBindPipeline(VkCommandBuffer commandBuffer, Pipeline pipeline, uint32_t w, uint32_t h)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

    uint32_t setIndex = 0;
    for (auto& descriptorSet : pipeline->vkDescriptorSets) {
        vkCmdBindDescriptorSets(commandBuffer, pipeline->vkBindPoint, pipeline->vkPipelineLayout,
                                setIndex,
                                1, &descriptorSet,
                                0, VK_NULL_HANDLE);
        setIndex++;
    }

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(w),
        .height = static_cast<float>(h),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = { w, h },
    };

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RenderDevice::CmdBindVertexBuffer(VkCommandBuffer commandBuffer, Buffer buffer, VkDeviceSize offset)
{
    CmdBindVertexBuffers(commandBuffer, 1, &buffer, &offset);
}

void RenderDevice::CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t count, Buffer *pBuffers, VkDeviceSize *pOffsets)
{
    std::vector<VkBuffer> buffers(count);

    for (uint32_t i = 0; i < count; i++)
        buffers[i] = pBuffers[i]->vkBuffer;

    vkCmdBindVertexBuffers(commandBuffer, 0, count, std::data(buffers), pOffsets);
}

void RenderDevice::CmdBindIndexBuffer(VkCommandBuffer commandBuffer, Buffer buffer, uint32_t offset)
{
    vkCmdBindIndexBuffer(commandBuffer, buffer->vkBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void RenderDevice::CmdPushConstants(VkCommandBuffer commandBuffer, Pipeline pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data)
{
    vkCmdPushConstants(commandBuffer, pipeline->vkPipelineLayout, stageFlags, offset, size, data);
}

void RenderDevice::CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount)
{
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void RenderDevice::CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

void RenderDevice::SubmitQueue(VkCommandBuffer commandBuffer, VkFence fence)
{
    SubmitQueue(commandBuffer, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);
}

void RenderDevice::SubmitQueue(VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, uint32_t signalSemaphoreCount, const VkSemaphore* pSignalSemaphores, VkFence fence)
{
    VkResult err;

    if (fence != VK_NULL_HANDLE)
        vkResetFences(device, 1, &fence);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    if (waitSemaphoreCount > 0) {
        VkPipelineStageFlags pipelineStageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = pWaitSemaphores;
        submitInfo.pWaitDstStageMask = pipelineStageFlags;
    }

    if (signalSemaphoreCount > 0) {
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = pSignalSemaphores;
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    err = vkQueueSubmit(queue, 1, &submitInfo, fence);
    assert(!err);
}

void RenderDevice::SubmitAndPresentFrame(VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores)
{
    VkResult err;

    std::vector<VkSemaphore> waitSemaphores = {
        imageAvailableSemaphores[flightIndex]
    };

    for (uint32_t i = 0; i < waitSemaphoreCount; i++)
        waitSemaphores.push_back(pWaitSemaphores[i]);

    SubmitQueue(commandBuffer,
        std::size(waitSemaphores), std::data(waitSemaphores),
        1, &renderFinishedSemaphores[imageIndex],
        inFlightFences[flightIndex]);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };

    err = vkQueuePresentKHR(queue, &presentInfo);
    assert(!err);
}

void RenderDevice::AcquiredNextFrame(VkCommandBuffer* pCommandBuffer, SwapchainImage* pSwapchainImage)
{
    flightIndex = (flightIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    *pCommandBuffer = frameCommandBuffers[flightIndex];

    vkWaitForFences(device, 1, &inFlightFences[flightIndex], VK_TRUE, UINT32_MAX);
    vkResetFences(device, 1, &inFlightFences[flightIndex]);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    VkExtent2D currentExtent2D = capabilities.currentExtent;

    if (currentExtent2D.width != swapchainExtent2D.width || currentExtent2D.height != swapchainExtent2D.height)
        RebuildSwapchain();

    vkAcquireNextImageKHR(device, swapchain, UINT32_MAX, imageAvailableSemaphores[flightIndex], VK_NULL_HANDLE, &imageIndex);
    *pSwapchainImage = listSwapchainImage[imageIndex];
}

void RenderDevice::RebuildSwapchain()
{
    _CreateSwapchain(swapchain);
}

void RenderDevice::ReadBuffer(Buffer buffer, size_t size, void *data)
{
    void* src;
    vmaMapMemory(allocator, buffer->allocation, &src);
    memcpy(data, src, size);
    vmaUnmapMemory(allocator, buffer->allocation);
}

void RenderDevice::WriteBuffer(Buffer buffer, size_t size, const void *data)
{
    if (buffer->memoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {
        Buffer stagingBuffer;
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
        WriteBuffer(stagingBuffer, size, data);
        CopyBuffer(stagingBuffer, 0, buffer, 0, size);
        DestroyBuffer(stagingBuffer);
        return;
    }

    /* buffer->memoryUsage != VMA_MEMORY_USAGE_GPU_ONLY */
    void* dst;
    vmaMapMemory(allocator, buffer->allocation, &dst);
    memcpy(dst, data, size);
    vmaUnmapMemory(allocator, buffer->allocation);
}

void RenderDevice::CopyBuffer(Buffer srcBuffer, uint64_t srcOffset, Buffer dstBuffer, uint64_t dstOffset, uint64_t size)
{
    VkCommandBuffer commandBuffer;
    CreateCommandBuffer(&commandBuffer);
    BeginSingleTimeCommandBuffer(&commandBuffer);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer->vkBuffer, dstBuffer->vkBuffer, 1, &copyRegion);

    EndSingleTimeCommandBuffer(commandBuffer);
}

void RenderDevice::WriteTexture(Texture texture, uint64_t size, void *pixels)
{
    Buffer stagingBuffer;
    CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
    WriteBuffer(stagingBuffer, size, pixels);

    VkCommandBuffer commandBuffer;
    BeginSingleTimeCommandBuffer(&commandBuffer);

    CmdMemoryBarrier(commandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { texture->width, texture->height, 1 }
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer->vkBuffer,
        texture->vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion);

    EndSingleTimeCommandBuffer(commandBuffer);

    DestroyBuffer(stagingBuffer);
}

void RenderDevice::DeviceWaitIdle()
{
    vkDeviceWaitIdle(device);
}

void RenderDevice::QueueWaitIdle()
{
    vkQueueWaitIdle(queue);
}

void RenderDevice::WaitForFences(uint32_t count, const VkFence *pFences)
{
    vkWaitForFences(device, count, pFences, VK_TRUE, UINT64_MAX);
}

VkResult RenderDevice::LoadTextureFromFile(const char *filename, Texture *pTexture)
{
    VkResult err;

    int w, h, channel;
    stbi_uc* pixels = stbi_load(filename, &w, &h, &channel, STBI_rgb_alpha);

    err = CreateTexture(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, pTexture);

    if (err != VK_SUCCESS)
        goto TAG_LOAD_TEXTURE_FROM_FILE;

    WriteTexture(*pTexture, w * h * 4, pixels);

    VkCommandBuffer commandBuffer;
    BeginSingleTimeCommandBuffer(&commandBuffer);
    CmdMemoryBarrier(commandBuffer, *pTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EndSingleTimeCommandBuffer(commandBuffer);

TAG_LOAD_TEXTURE_FROM_FILE:
    stbi_image_free(pixels);
    return err;
}

void RenderDevice::BindUniformBuffer(Pipeline pipeline, const std::string &name, size_t offset, size_t range, Buffer buffer)
{
    if (!pipeline->descriptorSetInfos.contains(name))
        throw std::runtime_error(qk_format("[vulkan] error cannot found descriptor info by name '%s'", name.c_str()));

    const QVkPipeline::DescriptorSetInfo& descriptorSet = pipeline->descriptorSetInfos[name];

    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = buffer->vkBuffer;
    descriptorBufferInfo.offset = offset;
    descriptorBufferInfo.range = range;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet.vkDescriptorSet;
    descriptorWrite.dstBinding = descriptorSet.binding;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
}

void RenderDevice::BindTexture(Pipeline pipeline, const std::string& name, Texture texture, VkSampler sampler)
{
    if (!pipeline->descriptorSetInfos.contains(name))
        throw std::runtime_error(qk_format("[vulkan] error cannot found descriptor info by name '%s'", name.c_str()));

    const QVkPipeline::DescriptorSetInfo& descriptorSet = pipeline->descriptorSetInfos[name];

    VkDescriptorImageInfo descriptorImageInfo = {};
    descriptorImageInfo.imageView = texture->vkImageView;
    descriptorImageInfo.imageLayout = texture->layout;
    descriptorImageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet.vkDescriptorSet;
    descriptorWrite.dstBinding = descriptorSet.binding;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
}

VkImageView RenderDevice::GetVkImageViewHandle(Texture texture) const
{
    return texture->vkImageView;
}

VkResult RenderDevice::_CreateInstance()
{
    VkResult err;

    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Quokka";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Quokka";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    const std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    #if defined(_WIN32)
        "VK_KHR_win32_surface",
    #elif defined(__APPLE__)
        "VK_MVK_macos_surface",
        "VK_EXT_metal_surface",
    #elif defined(__linux__)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    #endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,

#if VK_HEADER_VERSION >= 216
        "VK_KHR_portability_enumeration",
        "VK_KHR_get_physical_device_properties2",
#endif
    };

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if VK_HEADER_VERSION >= 216
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(std::size(layers));
    instanceCreateInfo.ppEnabledLayerNames = std::data(layers);
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
    instanceCreateInfo.ppEnabledExtensionNames = std::data(extensions);

    err = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);
    VK_CHECK_ERROR(err);

    return err;
}

VkResult RenderDevice::_CreateDevice()
{
    VkResult err;

    physicalDevice = VkUtils::PickBestPhysicalDevice(instance);
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    printf("[vulkan] found best physical device: %s\n", physicalDeviceProperties.deviceName);

    queueFamilyIndex = VkUtils::FindQueueFamilyIndex(physicalDevice, surface);
    assert(queueFamilyIndex != UINT32_MAX);

    float priorities = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priorities;

    const std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset"
#endif
    };

    /* dynamic rendering */
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature = {};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &dynamicRenderingFeature;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
    deviceCreateInfo.ppEnabledExtensionNames = std::data(extensions);

    err = vkCreateDevice(physicalDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
    VK_CHECK_ERROR(err);

    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

TAG_DEVICE_Create_END:
    return err;
}

VkResult RenderDevice::_CreateMemoryAllocator()
{
    VkResult err;

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    err = vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    VK_CHECK_ERROR(err);

    return err;
}

VkResult RenderDevice::_CreateSwapchain(VkSwapchainKHR oldSwapchain)
{
    VkResult err;

    if (oldSwapchain != VK_NULL_HANDLE)
        DeviceWaitIdle();

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    VK_CHECK_ERROR(err);

    minImageCount = surfaceCapabilities.minImageCount + 1;
    if (minImageCount > surfaceCapabilities.maxImageCount)
        minImageCount = surfaceCapabilities.maxImageCount;

    swapchainExtent2D = surfaceCapabilities.currentExtent;

    uint32_t formatCount = 0;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, VK_NULL_HANDLE);
    VK_CHECK_ERROR(err);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, std::data(surfaceFormats));
    VK_CHECK_ERROR(err);

    surfaceFormat = VkUtils::ChooseSwapSurfaceFormat(surfaceFormats);

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = minImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent2D;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR tmpSwapchain = VK_NULL_HANDLE;
    err = vkCreateSwapchainKHR(device, &swapchainCreateInfo, VK_NULL_HANDLE, &tmpSwapchain);
    VK_CHECK_ERROR(err);

    if (oldSwapchain != VK_NULL_HANDLE)
        _DestroySwapchain();

    swapchain = tmpSwapchain;

    /* Create swapchain resources */
    err = vkGetSwapchainImagesKHR(device, swapchain, &minImageCount, nullptr);
    VK_CHECK_ERROR(err);

    listSwapchainImage.resize(minImageCount);
    renderFinishedSemaphores.resize(minImageCount);

    std::vector<VkImage> swapchainImages(minImageCount);
    err = vkGetSwapchainImagesKHR(device, swapchain, &minImageCount, std::data(swapchainImages));
    VK_CHECK_ERROR(err);

    for (uint32_t i = 0; i < minImageCount; i++) {
        VkImage swapchainImage = swapchainImages[i];

        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImage;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageView imageView = VK_NULL_HANDLE;
        err = vkCreateImageView(device, &imageViewCreateInfo, VK_NULL_HANDLE, &imageView);
        VK_CHECK_ERROR(err);

        listSwapchainImage[i] = _WrapSwapchainImage(swapchainExtent2D.width, swapchainExtent2D.height, swapchainImage, imageView); // NOLINT

        err = CreateSemaphore(&renderFinishedSemaphores[i]);
        VK_CHECK_ERROR(err);
    }

    return err;
}

VkResult RenderDevice::_CreateCommandPool()
{
    VkResult err;

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                                  | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    err = vkCreateCommandPool(device, &commandPoolCreateInfo, VK_NULL_HANDLE, &commandPool);
    VK_CHECK_ERROR(err);

    return err;
}

VkResult RenderDevice::_CreateDescriptorPool()
{
    VkResult err;

    VkDescriptorPoolSize pool_size[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                256 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          256 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          256 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   256 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   256 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         256 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         256 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 256 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       256 },
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolCreateInfo.maxSets = 1024;
    descriptorPoolCreateInfo.poolSizeCount = ARRAY_SIZE(pool_size);
    descriptorPoolCreateInfo.pPoolSizes = pool_size;

    err = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &descriptorPool);
    VK_CHECK_ERROR(err);

    return err;
}

VkResult RenderDevice::_CreateShaderModule(const char* path, VkShaderModule* pShaderModule)
{
    size_t size;
    VkResult err;

    char *buf = io_read_bytecode(path, &size);

    printf("[vulkan] load shader module %s, code size=%ld\n", path, size);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(buf);

    err = vkCreateShaderModule(device, &shaderModuleCreateInfo, VK_NULL_HANDLE, pShaderModule);
    io_free_buf(buf);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::_DestroySwapchain()
{
    for (uint32_t i = 0; i < minImageCount; i++) {
        vkDestroyImageView(device, listSwapchainImage[i]->vkImageView, VK_NULL_HANDLE);
        delete listSwapchainImage[i];
        DestroySemaphore(renderFinishedSemaphores[i]);
    }

    listSwapchainImage.clear();
    vkDestroySwapchainKHR(device, swapchain, VK_NULL_HANDLE);
}

VkResult RenderDevice::_InitSyncObjects()
{
    VkResult err;

    frameCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        err = CreateCommandBuffer(&frameCommandBuffers[i]);
        VK_CHECK_ERROR(err);

        err = CreateFence(&inFlightFences[i]);
        VK_CHECK_ERROR(err);

        err = CreateSemaphore(&imageAvailableSemaphores[i]);
        VK_CHECK_ERROR(err);
    }

    return err;
}

void RenderDevice::_DestroySyncObjects()
{
    DestroyCommandBuffers(MAX_FRAMES_IN_FLIGHT, std::data(frameCommandBuffers));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        DestroyFence(inFlightFences[i]);
        DestroySemaphore(imageAvailableSemaphores[i]);
    }
}

VkResult RenderDevice::_PerInitSamplers()
{
    VkResult err;

    err = CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, true, 16.0f, true, &linearRepeatSampler);
    VK_CHECK_ERROR(err);

    err = CreateSampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, 1.0f, false, &nearestClampSampler);
    VK_CHECK_ERROR(err);

    return err;
}

void RenderDevice::_DestroyPerInitSamplers()
{
    DestroySampler(linearRepeatSampler);
    DestroySampler(nearestClampSampler);
}

VmaMemoryUsage RenderDevice::_GuessMemoryUsage(VkBufferUsageFlags usage)
{
    if ((usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) && (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT))
        return VMA_MEMORY_USAGE_GPU_ONLY;

    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
        return VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        return VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        return VMA_MEMORY_USAGE_GPU_ONLY;

    // 默认：CPU -> GPU
    return VMA_MEMORY_USAGE_CPU_TO_GPU;
}

SwapchainImage RenderDevice::_WrapSwapchainImage(uint32_t w, uint32_t h, VkImage image, VkImageView imageView)
{
    Texture wrapTexture = new QVkTexture;
    wrapTexture->width = w;
    wrapTexture->height = h;
    wrapTexture->vkImage = image;
    wrapTexture->vkImageView = imageView;

    return wrapTexture;
}