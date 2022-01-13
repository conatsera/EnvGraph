// Copyright 2021 Austin Conatser
// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef ENVGRAPH_EVENTS_H
#define ENVGRAPH_EVENTS_H

#include <concepts>
#include <functional>
#include <type_traits>

#include <vector>
#include <cstring>
#include <cstdint>

#include <semaphore>
#include <thread>
#include <mutex>

#include <iostream>

namespace EnvGraph
{

typedef uint32_t EventTypeBase;

enum class Events : EventTypeBase
{
    INVALID = 0,
    STATUS,
    RENDER,
    INPUT,
    AUDIO,
    OBJECT,
    RESERVED = 255
};

enum RenderEventBits : EventTypeBase
{
    MOVEMENT = 1,
    LIGHTS = 1 << 1,
    VISIBILITY = 1 << 2,
    RESIZE = 1 << 3,
    FRAME_PERF = 1 << 4,
    RENDER_DEBUG = 1u << 31
};

typedef struct EventFilterBase
{
    Events type;
    uint32_t subtype;
} EventFilterBase_t;

template <typename T>
struct is_valid_event_subtype : std::bool_constant<std::is_same_v<std::underlying_type_t<T>, uint32_t>>
{
};

template <typename T> concept SubType = is_valid_event_subtype<T>::value;


struct Message
{
    Message(Events eventType, EventTypeBase eventSubType) : m_eventType(eventType), m_eventSubType(eventSubType){};

    const Events m_eventType;
    const EventTypeBase m_eventSubType;
};

template <typename T> constexpr bool is_message = std::is_base_of_v<Message, T>;

template <typename T, uint32_t count> concept IsValid = is_message<T> &&count <= 32;

struct EventBase
{
    const Events m_type;
    EventBase(Events type) : m_type(type)
    {
    }
};

template <typename T, uint32_t count> requires IsValid<T, count> struct Event : EventBase
{
    const bool IsMultipart = count > 1;
    Event(Events type, T messageContent) : EventBase(type), m_eventMessages(std::array<T, count>{messageContent})
    {
    }
    Event(Events type, std::array<T, count> messageContent) : EventBase(type), m_eventMessages(messageContent)
    {
    }
    std::array<T, count> m_eventMessages;
};

struct SubscriberBase
{
    bool ready = false;
    void *callback = nullptr;
};

template <typename CallbackFunc, SubType EventTypes> struct Subscriber
{
    bool ready = false;
    EventTypeBase filter;
    CallbackFunc func;
    Subscriber(EventTypeBase filter, CallbackFunc func) : filter(filter), func(func)
    {
        ready = true;
    }
    void End()
    {
        ready = false;
    }
};

template<typename BaseEvent, SubType EventTypes, unsigned int buffer_length> class Publisher
{
  public:
    using SubCallbackFunc = std::function<void(BaseEvent)>;
    Publisher()
    {
        m_processingThread = std::thread([&] {
            while (m_publisherActive)
            {
                std::scoped_lock<std::mutex> l(m_eventMutex);
                if (m_queueEnd != m_queueStart)
                {
                    if (++m_queueStart == buffer_length)
                        m_queueStart = 0;
                    auto message = m_storedMessages[m_queueStart];
                    bool cleanupSubs = false;
                    for (const auto &sub : m_subs)
                    {
                        if (sub.ready)
                        {
                            // no filter = all messages
                            if ((sub.filter == 0) || (message.m_eventSubType & sub.filter))
                                sub.func(message);
                        }
                        else
                            cleanupSubs = true;
                    }
                    if (cleanupSubs)
                        m_subs.erase(
                            std::remove_if(m_subs.begin(), m_subs.end(), [](const auto &s) { return !s.ready; }),
                            m_subs.end());
                }
            }
        });
    }
    ~Publisher()
    {
        m_publisherActive = false;
        m_processingThread.join();
    }

    Subscriber<SubCallbackFunc, EventTypes>* CreateNewSub(EventTypeBase filter, SubCallbackFunc func)
    {
        std::scoped_lock<std::mutex> l(m_eventMutex);
        return &(m_subs.emplace_back(Subscriber<SubCallbackFunc, EventTypes>(filter, func)));
    }
    Subscriber<SubCallbackFunc, EventTypes>* CreateNewSub(SubCallbackFunc func)
    {
        std::scoped_lock<std::mutex> l(m_eventMutex);
        return &(m_subs.emplace_back(Subscriber<SubCallbackFunc, EventTypes>(0, func)));
    }

  protected:
    std::vector<Subscriber<SubCallbackFunc, EventTypes>> m_subs;

    void NewEvent(BaseEvent message, bool preempt = false)
    {
        std::scoped_lock<std::mutex> l(m_eventMutex);

        if (++m_queueEnd == buffer_length)
            m_queueEnd = 0;
        if (m_queueEnd == m_queueStart)
            if (++m_queueStart == buffer_length)
                m_queueStart = 0;

        constexpr const size_t kMessageLen = sizeof(BaseEvent);
        std::memcpy(&m_storedMessages[m_queueEnd], &message, kMessageLen);
    }

  private:
    bool m_publisherActive = true;
    std::thread m_processingThread;
    std::mutex m_eventMutex;
    int m_queueStart = 0;
    int m_queueEnd = 0;

    std::array<BaseEvent, buffer_length> m_storedMessages;
};

} // namespace EnvGraph

#endif