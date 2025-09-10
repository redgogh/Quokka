#pragma once

#include "render_pass.h"

class ScreenRenderPass : public RenderPass {
public:
    ScreenRenderPass(RenderDevice* device);
   ~ScreenRenderPass();

    void Execute() override final;

private:
    VkCommandBuffer commandBuffer;
    SwapchainImage swapchainImage;
};