#pragma once

#include <cstdint>

namespace iotk
{

enum eKey
{
    kUnknown = 0u,
    kSpecialBegin = 1u,
    kTab = kSpecialBegin,
    kLeft,
    kRight,
    kUp,
    kDown,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,
    kInsert,
    kDelete,
    kBackspace,
    kEnter,
    kEscape,
    kSpecialEnd,

    kASCIIBegin = 0x20,
    kASCIIEnd = 0x7f,
};

enum fKeyMod
{
    kCtrl = 1u << 0,
    kShift = 1u << 1,
    kAlt = 1u << 2
};

enum fMouseButton
{
    kLeftBtn = 1u << 0,
    kMiddleBtn = 1u << 1,
    kRightBtn = 1u << 2
};

struct input_t
{
    float time_delta;

    bool key_down[256];
    uint32_t mod_down;
    int32_t cursor[2];
    int32_t wheel_delta;
    uint32_t button_down;
};

}
