#pragma once

#include "driver/render_device.h"
#include "engine/camera/camera.h"

//std
#include <queue>
#include <functional>
#include <string>

enum AttachmentUsage {
    Color,
    Depth
};

struct AttachmentDesc {
    Texture texture;
};

class RenderPass {
public:
    using DrawCallback = std::function<void(VkCommandBuffer)>;

    RenderPass(const std::string& name);

    void SetAttachment(const AttachmentDesc& desc);
    void SetDrawCallback(DrawCallback callback);

private:
    std::string name;
    AttachmentDesc desc;
    DrawCallback drawFunc;
};