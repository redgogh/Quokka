#ifndef RENDER_DRIVER_H_
#define RENDER_DRIVER_H_

#define USE_VOLK_LOADER

#ifdef USE_VOLK_LOADER
#include <volk/volk.h>
#endif /* USE_VOLK_LOADER */

#include <vma/vk_mem_alloc.h>
#include <quokka/typedefs.h>

// std
#include <assert.h>
#include <vector>

typedef struct QVkTexture *Texture;
typedef struct QVkBuffer *Buffer;
typedef struct QVkPipeline *Pipeline;

/* 定义 SwapchainImage 对象，因为 Texture 不太适合描述交换链图像 */
typedef Texture SwapchainImage;

/* dGetVkImage 函数用于在不依赖 RenderDriver 的情况下获取 Vulkan 原生 VkImage 对象 */
VkImage dGetVkImage(Texture texture);

/* dGetVkImageView 函数用于在不依赖 RenderDriver 的情况下获取 Vulkan 原生 VkImageView 对象 */
VkImageView dGetVkImageView(Texture texture);

class RenderDriver
{
public:
    RenderDriver();
   ~RenderDriver();

    VkResult Initialize(VkSurfaceKHR surface);

    /* 与 Vulkan 资源创建和释放相关的函数  */
    VkResult CreateBuffer(size_t size, VkBufferUsageFlags usage, Buffer *pBuffer);
    VkResult CreateVertexBuffer(size_t size, const void* data, Buffer* pVertexBuffer);
    VkResult CreateIndexBuffer(size_t size, const void* data, Buffer* pIndexBuffer);
    void DestroyBuffer(Buffer buffer);
    VkResult CreateTexture(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, Texture *pTexture);
    void DestroyTexture(Texture Texture);
    VkResult CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, bool enableAnisotropy, float maxAnisotropy, bool useMipmaps, VkSampler* pSampler);
    void DestroySampler(VkSampler sampler);
    VkResult CreatePipeline(const char *shaderName, Pipeline* pPipeline);
    void DestroyPipeline(Pipeline pipeline);
    VkResult CreateCommandBuffer(VkCommandBuffer* pCommandBuffer);
    void DestroyCommandBuffer(VkCommandBuffer commandBuffer);
    void DestroyCommandBuffers(uint32_t count, const VkCommandBuffer* pCommandBuffers);
    VkResult CreateFence(VkFence* pFence);
    void DestroyFence(VkFence fence);
    VkResult CreateSemaphore(VkSemaphore* pSemaphore);
    void DestroySemaphore(VkSemaphore semaphore);
    VkResult CreateDescriptorSetLayout(uint32_t bindingCount, const VkDescriptorSetLayoutBinding *pBindings, VkDescriptorSetLayout* pDescriptorSetLayout);
    void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
    VkResult CreateDescriptorSets(uint32_t descriptorSetCount, const VkDescriptorSetLayout* pDescriptorSetLayouts, VkDescriptorSet *pDescriptorSets);
    void DestroyDescriptorSets(uint32_t descriptorSetCount, VkDescriptorSet* pDescriptorSets);

    /* Vulkan 命令相关函数 */
    void BeginSingleTimeCommandBuffer(VkCommandBuffer* pCommandBuffer);
    void EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer);
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags = 0);
    void EndCommandBuffer(VkCommandBuffer commandBuffer);
    void CmdMemoryBarrier(VkCommandBuffer commandBuffer, Texture texture, VkImageLayout newLayout);
    void CmdBeginRendering(VkCommandBuffer commandBuffer, Texture texture);
    void CmdEndRendering(VkCommandBuffer commandBuffer);
    void CmdBindPipeline(VkCommandBuffer commandBuffer, Pipeline pipeline, uint32_t w, uint32_t h);
    void CmdBindVertexBuffer(VkCommandBuffer commandBuffer, Buffer buffer, VkDeviceSize offset);
    void CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t count, Buffer *pBuffers, VkDeviceSize *pOffsets);
    void CmdBindIndexBuffer(VkCommandBuffer commandBuffer, Buffer buffer, uint32_t offset);
    void CmdPushConstants(VkCommandBuffer commandBuffer, Pipeline pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data);
    void CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount);
    void CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount);
    void SubmitQueue(VkCommandBuffer commandBuffer, VkFence fence);
    void SubmitQueue(VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, uint32_t signalSemaphoreCount, const VkSemaphore* pSignalSemaphores, VkFence fence);
    void SubmitAndPresentFrame(VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores);

    /* 自定义封装开放 API */
    void AcquiredNextFrame(VkCommandBuffer* pCommandBuffer, SwapchainImage* pSwapchainImage);
    void RebuildSwapchain();
    void ReadBuffer(Buffer buffer, size_t size, void* data);
    void WriteBuffer(Buffer buffer, size_t size, const void* data);
    void CopyBuffer(Buffer srcBuffer, uint64_t srcOffset, Buffer dstBuffer, uint64_t dstOffset, uint64_t size);
    void WriteTexture(Texture texture, uint64_t size, void* pixels);
    void DeviceWaitIdle();
    void QueueWaitIdle();
    void WaitForFences(uint32_t count, const VkFence* pFences);
    VkResult LoadTextureFromFile(const char* filename, Texture* pTexture);

    /* 渲染管线绑定函数，与 Vulkan 中的描述符绑定相关 */
    void BindUniformBuffer(Pipeline pipeline, const std::string &name, size_t offset, size_t range, Buffer buffer);
    void BindTexture(Pipeline pipeline, const std::string& name, Texture texture, VkSampler sampler);

    /* Vulkan 对象资源句斌获取函数 */
    VkInstance GetInstance() const { return instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    uint32_t GetQueueFamilyIndex() const { return queueFamilyIndex; }
    VkQueue GetGraphicsQueue() const { return queue; }
    VkQueue GetPresentQueue() const { return queue; }
    VkDevice GetDevice() const { return device; }
    VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
    uint32_t GetMinImageCount() const { return minImageCount; }
    VkExtent2D GetSwapchainExtent2D() const { return swapchainExtent2D; }
    float GetSwapchainAspectRatio() const { return swapchainExtent2D.width / swapchainExtent2D.height; }
    VkFormat GetSwapchainFormat() const { return surfaceFormat.format; }
    VkImageView GetVkImageViewHandle(Texture texture) const;
    VkSampler GetLinearRepeatSampler() { return linearRepeatSampler; }
    VkSampler GetNearestClampSampler() { return nearestClampSampler; }

private:
    VkResult _CreateInstance();
    VkResult _CreateDevice();
    VkResult _CreateMemoryAllocator();
    VkResult _CreateSwapchain(VkSwapchainKHR oldSwapchain);
    VkResult _CreateCommandPool();
    VkResult _CreateDescriptorPool();
    VkResult _CreateShaderModule(const char* shaderName, VkShaderModule* pShaderModule);

    void _DestroySwapchain();

    VkResult _InitSyncObjects();
    void _DestroySyncObjects();

    VkResult _PerInitSamplers();
    void _DestroyPerInitSamplers();

    static VmaMemoryUsage _GuessMemoryUsage(VkBufferUsageFlags usage);
    static SwapchainImage _WrapSwapchainImage(uint32_t w, uint32_t h, VkImage image, VkImageView imageView);

    // Vulkan handles
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkFence submitFence = VK_NULL_HANDLE;

    // Vulkan swapchain resources
    uint32_t minImageCount = 0;
    std::vector<SwapchainImage> listSwapchainImage;
    VkExtent2D swapchainExtent2D = {};
    uint32_t imageIndex = 0;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    // Sync objects
    uint32_t flightIndex = 0;
    uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkCommandBuffer> frameCommandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t queueFamilyIndex = UINT32_MAX;
    VkSurfaceFormatKHR surfaceFormat = {};
    VkPhysicalDeviceProperties physicalDeviceProperties = {};

    /* sampler */
    VkSampler linearRepeatSampler = VK_NULL_HANDLE;
    VkSampler nearestClampSampler = VK_NULL_HANDLE;
};

#endif /* RENDER_DRIVER_H_ */
