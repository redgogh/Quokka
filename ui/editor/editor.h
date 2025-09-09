#ifndef EDITOR_H_
#define EDITOR_H_

#include "driver/render_device.h"
#include "platform/glfw3/window.h"
#include <qk_imgui/qk_imgui.h>

//std
#include <string>
#include <functional>

struct EditorWindow {
    std::string name;
    std::function<void()> drawFunc;
    bool visible = true;
};

class GameEditor
{
public:
    GameEditor(RenderDevice* pDevice, Window* window);
   ~GameEditor();

    void BeginNewFrame(VkCommandBuffer commandBuffer);
    void EndFrame(VkCommandBuffer commandBuffer);

    ImTextureID CreateTextureId(Texture texture);
    void DestroyTextureId(ImTextureID textureId);

    void RegisterWindow(const std::string& name, std::function<void()> drawFunc);
    EditorWindow& GetWindow(const std::string& name) { return windows[name]; }
    void RegisterViewport(const std::string& name, std::function<void()> drawFunc);
    EditorWindow& GetViewport(const std::string& name) { return viewports[name]; }

public:
    static void ShowDemoWindow();
    static void DrawFPS(uint32_t fps);

private:
    RenderDevice* device = VK_NULL_HANDLE;
    Window* hwnd = VK_NULL_HANDLE;
    std::unordered_map<std::string, EditorWindow> viewports;
    std::unordered_map<std::string, EditorWindow> windows;

};

#endif /* EDITOR_H_ */
