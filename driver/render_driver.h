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

typedef struct QVkTexture2D *Texture2D;
typedef struct QVkBuffer *Buffer;
typedef struct QVkPipeline *Pipeline;

VkImageView dGetVkImageView(Texture2D texture);

class RenderDriver
{
public:
    RenderDriver();
   ~RenderDriver();

    VkResult Initialize(VkSurfaceKHR surface);

    VkResult CreateBuffer(size_t size, VkBufferUsageFlags usage, Buffer *pBuffer);
    void DestroyBuffer(Buffer buffer);
    VkResult CreateTexture2D(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, Texture2D *pTexture2D);
    void DestroyTexture2D(Texture2D Texture2D);
    VkResult CreateSampler(VkSampler* pSampler);
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

    // Vulkan commands
    void BeginSingleTimeCommandBuffer(VkCommandBuffer* pCommandBuffer);
    void EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer);
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags = 0);
    void EndCommandBuffer(VkCommandBuffer commandBuffer);
    void CmdTextureMemoryBarrier(VkCommandBuffer commandBuffer, Texture2D texture, VkImageLayout newLayout);
    void CmdBeginRendering(VkCommandBuffer commandBuffer, Texture2D texture);
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

    // Open APIs
    void AcquiredNextFrame(VkCommandBuffer* pCommandBuffer, Texture2D* pTexture);
    void RebuildSwapchain();
    void ReadBuffer(Buffer buffer, size_t size, void* data);
    void WriteBuffer(Buffer buffer, size_t size, void* data);
    void CopyBuffer(Buffer srcBuffer, uint64_t srcOffset, Buffer dstBuffer, uint64_t dstOffset, uint64_t size);
    void WriteTexture2D(Texture2D texture, uint64_t size, void* pixels);
    void DeviceWaitIdle();
    void QueueWaitIdle();
    void WaitForFences(uint32_t count, const VkFence* pFences);
    VkResult LoadTextureFromFile(const char* filename, Texture2D* pTexture);

    // pipeline bind
    void BindUniformBuffer(Pipeline pipeline, const std::string &name, size_t offset, size_t range, Buffer buffer);
    void BindTexture(Pipeline pipeline, const std::string& name, Texture2D texture, VkSampler sampler);

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
    VkImageView GetVkImageViewHandle(Texture2D texture) const;

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

    static VmaMemoryUsage _GuessMemoryUsage(VkBufferUsageFlags usage);
    static Texture2D _WrapTexture2D(uint32_t w, uint32_t h, VkImage image, VkImageView imageView);

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
    std::vector<Texture2D> swapchainTextures;
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
};

#endif /* RENDER_DRIVER_H_ */
