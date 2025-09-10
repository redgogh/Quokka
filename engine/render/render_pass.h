#pragma once

#include "driver/render_device.h"
#include "engine/camera/camera.h"

//std
#include <queue>
#include <functional>

struct RenderTarget {
    Texture texture;
};

class RenderPass {
public:
    using DrawCallback = std::function<void(VkCommandBuffer, uint32_t, uint32_t)>;

    RenderPass(RenderDevice* v_device)
        : device(v_device) { DO_NOTHING(); }

    void Draw(const DrawCallback &drawFunc)
      {
        drawCallbacks.push(drawFunc);
      }

    virtual void Execute() = 0;

protected:
    RenderDevice* device;
    std::queue<DrawCallback> drawCallbacks;
};