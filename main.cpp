#include "main.h"

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
    const std::unique_ptr<RenderDriver> driver = std::make_unique<RenderDriver>();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = window->CreateWindowSurface(driver->GetInstance(), VK_NULL_HANDLE, &surface);
    assert(!err);
    driver->Initialize(surface);

    std::unique_ptr<GameEditor> editor = std::make_unique<GameEditor>(driver.get(), window.get());

    Pipeline pipeline;
    driver->CreatePipeline("qk_simple_shader", &pipeline);

    Buffer vertexBuffer;
    driver->CreateVertexBuffer(sizeof(vertices), vertices, &vertexBuffer);

    Buffer indexBuffer;
    driver->CreateIndexBuffer(sizeof(indices), indices, &indexBuffer);

    Buffer uniformBuffer;
    driver->CreateBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uniformBuffer);
    driver->BindUniformBuffer(pipeline, "camera", 0, sizeof(glm::mat4), uniformBuffer);

    Camera camera(0.0f, 0.0f, 3.0f);

    // 离屏渲染
    VkCommandBuffer v2CommandBuffer;
    driver->CreateCommandBuffer(&v2CommandBuffer);

    Texture v2Texture;
    ImVec2 watchWSize(32, 32);
    ImVec2 watchVWSize(32, 32);
    driver->CreateTexture(static_cast<uint32_t>(watchWSize.x), static_cast<uint32_t>(watchWSize.y), VK_FORMAT_B8G8R8A8_UNORM,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);

    VkFence drawFence;
    driver->CreateFence(&drawFence);

    Texture quokkaLogo;
    driver->LoadTextureFromFile("../misc/quokka_1.png", &quokkaLogo);
    driver->BindTexture(pipeline, "tex", quokkaLogo, driver->GetLinearRepeatSampler());

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
            driver->DeviceWaitIdle();
            editor->DestroyTextureId(textureId);
            driver->DestroyTexture(v2Texture);
            driver->CreateTexture(watchWSize.x, watchWSize.y, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v2Texture);
            textureId = editor->CreateTextureId(v2Texture);
        }

        // update uniform buffer
        driver->WriteBuffer(uniformBuffer, sizeof(glm::mat4), glm::value_ptr(PC_MVP));

        driver->BeginCommandBuffer(v2CommandBuffer);
        driver->CmdMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        driver->CmdBeginRendering(v2CommandBuffer, v2Texture);

        driver->CmdBindPipeline(v2CommandBuffer, pipeline, watchWSize.x, watchWSize.y);
        driver->CmdBindVertexBuffer(v2CommandBuffer, vertexBuffer, 0);
        driver->CmdBindIndexBuffer(v2CommandBuffer, indexBuffer, 0);
        // driver->CmdDraw(v2CommandBuffer, ARRAY_SIZE(vertices));
        driver->CmdDrawIndexed(v2CommandBuffer, 6);

        driver->CmdEndRendering(v2CommandBuffer);
        driver->CmdMemoryBarrier(v2CommandBuffer, v2Texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        driver->EndCommandBuffer(v2CommandBuffer);
        driver->SubmitQueue(v2CommandBuffer, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, drawFence);
        driver->WaitForFences(1, &drawFence);

        VkCommandBuffer cmd;
        SwapchainImage swapchainImage;
        driver->AcquiredNextFrame(&cmd, &swapchainImage);
        driver->BeginCommandBuffer(cmd);
        driver->CmdMemoryBarrier(cmd, swapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        driver->CmdBeginRendering(cmd, swapchainImage);

        editor->BeginNewFrame(cmd);
        GameEditor::ShowDemoWindow();
        editor->EndFrame(cmd);

        driver->CmdEndRendering(cmd);
        driver->CmdMemoryBarrier(cmd, swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        driver->EndCommandBuffer(cmd);
        driver->SubmitAndPresentFrame(cmd, 0, VK_NULL_HANDLE);

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

    driver->DeviceWaitIdle();

    driver->DestroyTexture(quokkaLogo);
    driver->DestroyFence(drawFence);
    driver->DestroyTexture(v2Texture);
    driver->DestroyPipeline(pipeline);
    driver->DestroyBuffer(uniformBuffer);
    driver->DestroyBuffer(vertexBuffer);
    driver->DestroyBuffer(indexBuffer);

    return 0;
}