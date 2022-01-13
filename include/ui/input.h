#ifndef ENVGRAPH_INPUT_H
#define ENVGRAPH_INPUT_H

#include <array>
#include <cassert>

#include <events.h>

namespace EnvGraph
{

namespace UI
{

enum InputDevice_t : EventTypeBase
{
    INVALID_DEVICE = 0,
    CHAR,
    POINTER,
    HAND_POSE
};

enum InputEvent_t : EventTypeBase
{
    INVALID_STATUS = 0,
    DEV_ENUM
};

struct StatusMessage : Message
{
    StatusMessage() : Message(Events::STATUS, 0)
    {}
    StatusMessage(InputEvent_t type) : Message(Events::STATUS, type)
    {}
    StatusMessage(uint32_t deviceCount, std::array<InputDevice_t, 32> devices)
        : Message(Events::STATUS, InputEvent_t::DEV_ENUM), deviceCount(deviceCount), devices(devices)
    {
        assert(deviceCount < 33 && deviceCount != 0);
    }

    uint32_t deviceCount = 0;
    std::array<InputDevice_t, 32> devices;
};

struct InputMessage : Message
{
    InputMessage() : Message(Events::INPUT, 0) {}

};

using StatusPub = Publisher<StatusMessage, InputEvent_t, 32>;
using InputPub = Publisher<InputMessage, InputDevice_t, 64>;

class InputManager :
    public StatusPub, public InputPub
{
  public:
    void NewStatusSub(StatusPub::SubCallbackFunc func)
    {
        StatusPub::CreateNewSub(func);
    }
    void NewStatusSub(InputEvent_t type, StatusPub::SubCallbackFunc func)
    {
        StatusPub::CreateNewSub(type, func);
    }

    void NewInputSub(InputPub::SubCallbackFunc func)
    {
        InputPub::CreateNewSub(func);
    }
    void NewInputSub(InputDevice_t type, InputPub::SubCallbackFunc func)
    {
        InputPub::CreateNewSub(type, func);
    }

    void NewStatusEvent(StatusMessage message)
    {
        StatusPub::NewEvent(message);
    }
    void NewInputEvent(InputMessage message)
    {
        InputPub::NewEvent(message);
    }

  public:
    InputManager();
    ~InputManager();

    void EnumerateDevices();

    void Listen(InputDevice_t dev);
    void EndListen(InputDevice_t dev);
};
} // namespace UI
} // namespace EnvGraph

#endif