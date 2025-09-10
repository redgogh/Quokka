#include "screen_render_pass.h"

ScreenRenderPass::ScreenRenderPass(RenderDevice *device)
    : RenderPass(device)
{
    DO_NOTHING();
}

ScreenRenderPass::~ScreenRenderPass()
{
    DO_NOTHING();
}

void ScreenRenderPass::Execute()
{
    device->AcquiredNextFrame(&commandBuffer, &swapchainImage);

    device->BeginCommandBuffer(commandBuffer);
    device->CmdMemoryBarrier(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    device->CmdBeginRendering(commandBuffer, swapchainImage);

    uint32_t w, h;
    dGetTextureSize(swapchainImage, &w, &h);

    while (!drawCallbacks.empty()) {
        DrawCallback drawFunc = drawCallbacks.front();
        drawFunc(commandBuffer, w, h);
        drawCallbacks.pop();
    }

    device->CmdEndRendering(commandBuffer);
    device->CmdMemoryBarrier(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    device->EndCommandBuffer(commandBuffer);

    device->SubmitAndPresentFrame(commandBuffer, 0, VK_NULL_HANDLE);
}
