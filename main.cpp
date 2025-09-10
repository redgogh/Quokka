#include "main.h"

#include "engine/render/screen_render_pass.h"

int main()
{
#ifdef WIN32
    char _cwd[PATH_MAX];
    system("chcp 65001");
    getcwd(_cwd, sizeof(_cwd));
    _chdir("../shaders");
    system("spvc.bat");
    _chdir(_cwd);
#endif

#ifdef __APPLE__
    char _cwd[PATH_MAX];
    getcwd(_cwd, sizeof(_cwd));
    chdir("../shaders");
    system("./spvc");
    chdir(_cwd);
#endif

    setbuf(stdout, NULL);

    const std::unique_ptr<Window> window = std::make_unique<Window>("Quokka", 1450, 850);
    const std::unique_ptr<Dispatcher> dispatcher = std::make_unique<Dispatcher>(window.get());
    const std::unique_ptr<RenderDevice> device = std::make_unique<RenderDevice>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = window->CreateWindowSurface(device->GetInstance(), VK_NULL_HANDLE, &surface);
    assert(!err);
    device->Initialize(surface);

    const std::unique_ptr<ScreenRenderPass> screenRenderPass = std::make_unique<ScreenRenderPass>(device.get());

    std::unique_ptr<GameEditor> editor = std::make_unique<GameEditor>(device.get(), window.get());

    Pipeline pipeline;
    device->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    device->CreateVertexBuffer(sizeof(vertices), vertices, &vertexBuffer);

    Buffer indexBuffer;
    device->CreateIndexBuffer(sizeof(indices), indices, &indexBuffer);

    Buffer uniformBuffer;
    device->CreateBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uniformBuffer);
    device->BindUniformBuffer(pipeline, "camera", 0, sizeof(glm::mat4), uniformBuffer);

    Camera camera(0.0f, 0.0f, 3.0f);

    // 离屏渲染
    VkCommandBuffer v2CommandBuffer;
    device->CreateCommandBuffer(&v2CommandBuffer);

    Texture v2Texture;
    ImVec2 watchWSize(32, 32);
    ImVec2 watchVWSize(32, 32);
    device->CreateTexture(static_cast<uint32_t>(watchWSize.x), static_cast<uint32_t>(watchWSize.y), VK_FORMAT_B8G8R8A8_UNORM,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);

    VkFence drawFence;
    device->CreateFence(&drawFence);

    Texture quokkaLogo;
    device->LoadTextureFromFile("../misc/quokka_1.png", &quokkaLogo);
    device->BindTexture(pipeline, "tex", quokkaLogo, device->GetLinearRepeatSampler());

    ImTextureID textureId = editor->CreateTextureId(v2Texture);

    uint32_t fps = 0;
    uint32_t frameCount = 0;
    double lastFrameTime = 0;

    editor->RegisterViewport("视图", [&]() {
        // 只有当焦点在 viewport 窗口上才触发 Move 操作
        if (ImGui::IsWindowFocused()) {
            if (dispatcher->IsKeyHeld(GLFW_KEY_W))
                camera.Move(1, 0, 0);
            if (dispatcher->IsKeyHeld(GLFW_KEY_S))
                camera.Move(-1, 0, 0);
            if (dispatcher->IsKeyHeld(GLFW_KEY_A))
                camera.Move(0, -1, 0);
            if (dispatcher->IsKeyHeld(GLFW_KEY_D))
                camera.Move(0, 1, 0);
        }

        ImVec2 currentVWSize = ImGui::GetContentRegionAvail();
        if (watchVWSize.x != currentVWSize.x || watchVWSize.y != currentVWSize.y)
            watchVWSize = currentVWSize;
        ImGui::Image(textureId, currentVWSize);

        // fps
        GameEditor::DrawFPS(fps);
    });

    editor->RegisterWindow("检视器", [&camera]() {
        glm::vec3& position = camera.GetPosition();
        QkImGuiDragFloat3("位置", glm::value_ptr(position), 0.01f);
    });

    while (!window->ShouldClose()) {
        dispatcher->PollEvents();

        /* 计算 MVP 矩阵 */
        camera.Update();
        glm::mat4 PC_MVP = camera.GetProjectionMatrix() * camera.GetViewMatrix() * glm::mat4(1.0f);

        if (watchWSize.x != watchVWSize.x || watchWSize.y != watchVWSize.y) {
            watchWSize = watchVWSize;
            camera.SetAspectRatio(watchWSize.x / watchWSize.y);
            device->DeviceWaitIdle();
            editor->DestroyTextureId(textureId);
            device->DestroyTexture(v2Texture);
            device->CreateTexture(watchWSize.x, watchWSize.y, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);
            textureId = editor->CreateTextureId(v2Texture);
        }

        // update uniform buffer
        device->WriteBuffer(uniformBuffer, sizeof(glm::mat4), glm::value_ptr(PC_MVP));

        device->BeginCommandBuffer(v2CommandBuffer);
        device->CmdMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        device->CmdBeginRendering(v2CommandBuffer, v2Texture);

        device->CmdBindPipeline(v2CommandBuffer, pipeline, watchWSize.x, watchWSize.y);
        device->CmdBindVertexBuffer(v2CommandBuffer, vertexBuffer, 0);
        device->CmdBindIndexBuffer(v2CommandBuffer, indexBuffer, 0);
        // device->CmdDraw(v2CommandBuffer, ARRAY_SIZE(vertices));
        device->CmdDrawIndexed(v2CommandBuffer, 6);

        device->CmdEndRendering(v2CommandBuffer);
        device->CmdMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        device->EndCommandBuffer(v2CommandBuffer);
        device->SubmitQueue(v2CommandBuffer, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, drawFence);
        device->WaitForFences(1, &drawFence);

        screenRenderPass->Draw([&](VkCommandBuffer commandBuffer, uint32_t w, uint32_t h) {
            editor->BeginNewFrame(commandBuffer);
            GameEditor::ShowDemoWindow();
            editor->EndFrame(commandBuffer);
        });

        screenRenderPass->Execute();

        /* fps 计算 */
        frameCount++;

        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - lastFrameTime;
        if (elapsedTime > 1) {
            fps = frameCount;
            frameCount = 0;
            lastFrameTime = currentTime;
        }
    }

    device->DeviceWaitIdle();

    device->DestroyTexture(quokkaLogo);
    device->DestroyFence(drawFence);
    device->DestroyTexture(v2Texture);
    device->DestroyPipeline(pipeline);
    device->DestroyBuffer(uniformBuffer);
    device->DestroyBuffer(vertexBuffer);
    device->DestroyBuffer(indexBuffer);

    return 0;
}
