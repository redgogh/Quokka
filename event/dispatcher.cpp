#include "dispatcher.h"

#define UC_KEY "dispatcher"

//std
#include <cstring>

Dispatcher::Dispatcher(Window *pWindow) : window(pWindow)
{
    window->SetUserContextData(UC_KEY, this);

    // 注册键盘事件
    window->RegisterKeyCallback([](Window* uc_window, int key, int scancode, int action, int mods) {
        Dispatcher* dispatcher = static_cast<Dispatcher*>(uc_window->GetUserContextData(UC_KEY));
        EventType TP = action == GLFW_PRESS ? EVENT_TYPE_KEY_DOWN : EVENT_TYPE_KEY_UP;

        if (action == GLFW_PRESS && !dispatcher->keyHeld[key]) {
            dispatcher->keyDown[key] = true;
            dispatcher->keyHeld[key] = true;
            dispatcher->queue.push({ TP, key, mods, 0, 0 });
        }

        if (action == GLFW_RELEASE) {
            dispatcher->keyUp[key] = true;
            dispatcher->keyHeld[key] = false;
        }
    });

    // 鼠标按钮事件
    window->RegisterMouseButtonCallback([](Window* uc_window, int button, int action, int mods) {
        Dispatcher* dispatcher = static_cast<Dispatcher*>(uc_window->GetUserContextData(UC_KEY));

        EventType TP = action == GLFW_PRESS ? EVENT_TYPE_MOUSE_BUTTON_DOWN : EVENT_TYPE_MOUSE_BUTTON_UP;

        if (action == GLFW_PRESS && !dispatcher->mouseButtonHeld[button]) {
            dispatcher->mouseButtonDown[button] = true;
            dispatcher->mouseButtonHeld[button] = true;
            dispatcher->queue.push({ TP, button, mods, 0, 0 });
        }

        if (action == GLFW_RELEASE) {
            dispatcher->mouseButtonUp[button] = true;
            dispatcher->mouseButtonHeld[button] = false;
        }
    });

    // 鼠标滚轮事件
    window->RegisterScrollCallback([](Window* uc_window, double x, double y) {
        Dispatcher* dispatcher = static_cast<Dispatcher*>(uc_window->GetUserContextData(UC_KEY));
        dispatcher->queue.push({ EVENT_TYPE_SCROLL, 0, 0, x, y });
        dispatcher->scrollX = x;
        dispatcher->scrollY = y;
        printf("y = %.2f\n", dispatcher->scrollY);
    });

    // 鼠标移动事件
    window->RegisterCursorPosCallback([](Window* uc_window, double x, double y) {
        Dispatcher* dispatcher = static_cast<Dispatcher*>(uc_window->GetUserContextData(UC_KEY));
        dispatcher->queue.push({ EVENT_TYPE_MOUSE_MOVE, 0, 0, x, y });
        dispatcher->mouseX = x;
        dispatcher->mouseY = y;
    });
}

Dispatcher::~Dispatcher()
{
    /* do nothing... */
}

void Dispatcher::PollEvents()
{
    _ResetStateForFrame();
    glfwPollEvents();
}

void Dispatcher::_ResetStateForFrame()
{
    memset(keyDown, 0, sizeof(keyDown));
    memset(keyUp, 0, sizeof(keyUp));

    scrollX = 0;
    scrollY = 0;

    /* 清理事件队列 */
    std::queue<Event> empty;
    std::swap(queue, empty);
}