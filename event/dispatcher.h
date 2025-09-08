#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "platform/glfw3/window.h"
#include "event.h"

//std
#include <queue>

class Dispatcher {
public:
    Dispatcher(Window* pWindow);
   ~Dispatcher();

    double GetMouseX() { return mouseX; }
    double GetMouseY() { return mouseY; }
    double GetScrollX() { return scrollX; }
    double GetScrollY() { return scrollY; }

    bool IsKeyDown(int key) { return keyDown[key]; }
    bool IsKeyUp(int key) { return keyUp[key]; }
    bool IsKeyHeld(int key) { return keyHeld[key]; }

    bool IsMouseButtonDown(int button) { return mouseButtonDown[button]; }
    bool IsMouseButtonUp(int button) { return mouseButtonUp[button]; }
    bool IsMouseButtonHeld(int button) { return mouseButtonHeld[button]; }

   /* 每帧都拉取最新事件列表 */
    void PollEvents();

private:
    void _ResetStateForFrame();

    Window* window;

    int keyDown[512] = {0};
    int keyUp[512] = {0};
    int keyHeld[512] = {0};

    int mouseButtonDown[32] = {0};
    int mouseButtonUp[32] = {0};
    int mouseButtonHeld[32] = {0};

    double mouseX = 0.0f, mouseY = 0.0f;
    double scrollX = 0.0f, scrollY = 0.0f;

    std::queue<Event> queue;
};

#endif /* DISPATCHER_H_ */
