#ifndef EDITOR_H_
#define EDITOR_H_

#include "driver/render_driver.h"
#include "platform/glfw3/window.h"
#include <qk_imgui/qk_imgui.h>

class GameEditor
{
public:
    static void Initialize(RenderDriver* driver, Window* window);
    static void Terminate();

    static void BeginNewFrame(VkCommandBuffer commandBuffer);
    static void EndNewFrame(VkCommandBuffer commandBuffer);

    static void CreateImTextureID(Texture texture, ImTextureID* pImTextureId);
    static void DestroyImTextureID(ImTextureID textureId);
    static void DrawImage(ImTextureID textureId, const ImVec2& size);

    static void ShowDemoWindow();
};

#endif /* EDITOR_H_ */
