/* Copyright (c) 2021 Austin Conatser
 * A mock object that emits events on-demand and
 * periodically for testing. This includes "Bad" examples
 * that will not compile due to template constraints.
 * I have not yet found a good reason for them to exist, since
 * they can't be unit tested through gtest.
 */

#include <thread>

#include "events.h"

using namespace EnvGraph;

// It is not a hard requirement for an event publisher to have subtypes but
// filtering is directly dependent on them.
enum TestObjectEventBits : EventTypeBase
{
    TEST1 = 1,
    TEST2 = 1 << 1,
    TEST3 = 1 << 2,
    TEST4 = 1 << 3,
    TEST5 = 1 << 4,
    TEST_PERIODIC = 1 << 5,
    TEST_DEMAND = 1 << 6,
    TEST_RESERVED = 255
};

// Event subtype must be a uint32_t
enum class TestObjectEventsBad : uint64_t
{
    TEST1 = 1,
    TEST2 = 1 << 1,
    TEST3 = 1 << 2,
    TEST4 = 1 << 3,
    TEST5 = 1 << 4,
    TEST_PERIODIC = 1 << 5,
    TEST_DEMAND = 1 << 6,
    TEST_RESERVED = 255
};

struct TestObjectMsg : Message
{
    TestObjectMsg() : Message(Events::OBJECT, 0)
    {
    }
    TestObjectMsg(TestObjectEventBits type, int a, float b) : Message(Events::OBJECT, type), i(a), f(b)
    {
        if (type == TestObjectEventBits::TEST_PERIODIC)
            periodic = true;
    }
    int i;
    float f;
    bool periodic;
};

// This message sets the type to INVALID. This is also equivalent to leaving the type unset or 0.
struct TestObjectBadMsg : Message
{
    TestObjectBadMsg(TestObjectEventBits type, int a, float b) : Message(Events::INVALID, type), a(a), b(b)
    {
    }
    int a;
    float b;
};

// This message does not sub-struct the Message struct, a requirement of functioning messages
struct TestObjectBadMsg2
{
    TestObjectBadMsg2(int a, float b) : a(a), b(b)
    {
    }
    int a;
    float b;
};

class TestObject : public Publisher<TestObjectMsg, TestObjectEventBits, 64>
{
  public:
    TestObject()
    {
    }
    ~TestObject()
    {
    }

    void EnablePeriodicEvents()
    {
        periodicEventsEnabled = true;
        periodicEventThread = std::thread([&] {
            while (periodicEventsEnabled)
            {
                NewEvent(TestObjectMsg(TestObjectEventBits::TEST_PERIODIC, 1, 1.f));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    void DisablePeriodicEvents()
    {
        periodicEventsEnabled = false;
        periodicEventThread.join();
    }

    void SendEvent(TestObjectMsg msg)
    {
        NewEvent(msg);
    }

  private:
    std::thread periodicEventThread;
    bool periodicEventsEnabled = false;
};

class TestObjectLarge : public Publisher<TestObjectMsg, TestObjectEventBits, 256>
{
  public:
    TestObjectLarge()
    {
    }
    ~TestObjectLarge()
    {
    }

    void EnablePeriodicEvents()
    {
        periodicEventsEnabled = true;
        periodicEventThread = std::thread([&] {
            while (periodicEventsEnabled)
            {
                NewEvent(TestObjectMsg(TestObjectEventBits::TEST_PERIODIC, 1, 1.f));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    void DisablePeriodicEvents()
    {
        periodicEventsEnabled = false;
        periodicEventThread.join();
    }

    void SendEvent(TestObjectMsg msg)
    {
        NewEvent(msg);
    }

  private:
    std::thread periodicEventThread;
    bool periodicEventsEnabled = false;
};