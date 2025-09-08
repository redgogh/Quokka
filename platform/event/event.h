#ifndef EVENT_H_
#define EVENT_H_

enum EventType {
    EVENT_TYPE_KEY_DOWN,
    EVENT_TYPE_KEY_UP,
    EVENT_TYPE_KEY_DOUBLE_CLICK,
    EVENT_TYPE_MOUSE_MOVE,
    EVENT_TYPE_MOUSE_BUTTON_DOWN,
    EVENT_TYPE_MOUSE_BUTTON_UP,
    EVENT_TYPE_MOUSE_DOUBLE_CLICK,
    EVENT_TYPE_SCROLL,
};

struct Event {
    EventType type;
    int key, mods;
    double x, y;
};

#endif /* EVENT_H_ */
