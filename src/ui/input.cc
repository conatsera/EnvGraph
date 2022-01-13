#include <ui/input.h>

namespace EnvGraph
{
namespace UI
{

InputManager::InputManager() {}
InputManager::~InputManager() {}

void InputManager::EnumerateDevices()
{
    std::array<InputDevice_t, 32> devs = {(InputDevice_t)42, (InputDevice_t)11, (InputDevice_t)30, (InputDevice_t)2};
    StatusPub::NewEvent(StatusMessage(4, devs));
}

void InputManager::Listen(InputDevice_t device)
{
}

void InputManager::EndListen(InputDevice_t device)
{
}

} // namespace UI
} // namespace EnvGraph