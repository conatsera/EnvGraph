/* Copyright (c) 2021 Austin Conatser
 * Tests filtering of events messages
 */
#include <gtest/gtest.h>

#include "events.h"

#include "../base.h"
#include "mock_object.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class EventTest : public ::testing::Test
{
  protected:
    TestObject testObject;
    TestObjectLarge testObjectLarge;
};

TEST_F(EventTest, testTemplateConstraints)
{
    static_assert(is_valid_event_subtype<TestObjectEventBits>::value == true);
    static_assert(is_valid_event_subtype<TestObjectEventsBad>::value == false);

    std::array<TestObjectMsg, 4> messages = {TestObjectMsg{TestObjectEventBits::TEST1, 1, 1.f},
                                             {TestObjectEventBits::TEST2, 2, 2.f},
                                             {TestObjectEventBits::TEST3, 3, 3.f},
                                             {TestObjectEventBits::TEST4, 4, 4.f}};

    auto testMsg = Event<TestObjectMsg, 1>(Events::OBJECT, {TestObjectEventBits::TEST1, 1, 1.f});
    auto testMsg2 = Event<TestObjectMsg, 4>(Events::OBJECT, messages);
    // auto testBadMsg = Event<TestBadMsg, 33>(Events::OBJECT, {1, 1.f});
    // auto testMsg = Event<TestBadMsg2, 1>(Events::OBJECT, {1, 1.f});

    ASSERT_EQ(testMsg.IsMultipart, false);
    ASSERT_EQ(testMsg2.IsMultipart, true);
    // ASSERT_EQ(testBadMsg.IsMultipart, false);
}

TEST_F(EventTest, testFilter)
{
    int subAllCalled = 0, subCalled = 0, sub2Called = 0, sub3Called = 0, sub4Called = 0, sub5Called = 0,
        sub1And2Called = 0;

    auto subAll = testObject.CreateNewSub([&subAllCalled](TestObjectMsg e) { subAllCalled++; });
    auto sub = testObject.CreateNewSub(TestObjectEventBits::TEST1,
                                       [&subCalled](TestObjectMsg e) { subCalled++; });
    auto sub2 = testObject.CreateNewSub(TestObjectEventBits::TEST2,
                                        [&sub2Called](TestObjectMsg e) { sub2Called++; });
    auto sub3 = testObject.CreateNewSub(TestObjectEventBits::TEST3,
                                        [&sub3Called](TestObjectMsg e) { sub3Called++; });
    auto sub4 = testObject.CreateNewSub(TestObjectEventBits::TEST4,
                                        [&sub4Called](TestObjectMsg e) { sub4Called++; });
    auto sub5 = testObject.CreateNewSub(TestObjectEventBits::TEST5,
                                        [&sub5Called](TestObjectMsg e) { sub5Called++; });
    auto sub1And2 = testObject.CreateNewSub(TestObjectEventBits::TEST1 | TestObjectEventBits::TEST2,
                                            [&sub1And2Called](TestObjectMsg e) { sub1And2Called++; });

    testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_EQ(subAllCalled, 1);
    ASSERT_EQ(subCalled, 1);
    ASSERT_EQ(sub2Called, 0);
    ASSERT_EQ(sub3Called, 0);
    ASSERT_EQ(sub4Called, 0);
    ASSERT_EQ(sub5Called, 0);
    ASSERT_EQ(sub1And2Called, 1);

    testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST2, 20, 20.f));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_EQ(subAllCalled, 2);
    ASSERT_EQ(subCalled, 1);
    ASSERT_EQ(sub2Called, 1);
    ASSERT_EQ(sub3Called, 0);
    ASSERT_EQ(sub4Called, 0);
    ASSERT_EQ(sub5Called, 0);
    ASSERT_EQ(sub1And2Called, 2);

    testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST3, 20, 20.f));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_EQ(subAllCalled, 3);
    ASSERT_EQ(subCalled, 1);
    ASSERT_EQ(sub2Called, 1);
    ASSERT_EQ(sub3Called, 1);
    ASSERT_EQ(sub4Called, 0);
    ASSERT_EQ(sub5Called, 0);
    ASSERT_EQ(sub1And2Called, 2);
    
    subAll->End();
    sub->End();
    sub2->End();
    sub3->End();
    sub4->End();
    sub5->End();
    sub1And2->End();
}

TEST_F(EventTest, overflow)
{
    int subCalled = 0;
    auto sub = testObject.CreateNewSub([&subCalled](TestObjectMsg e) { subCalled++; });

    for (int i = 0; i < 128; i++)
        testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_GT(subCalled, 0);
    EXPECT_NE(subCalled, 128);

    // Let the buffer empty before rechecking
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto subPrevCalled = subCalled;
    testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    ASSERT_EQ(subCalled, subPrevCalled + 1);

    sub->End();
}

TEST_F(EventTest, overflow2)
{
    int subCalled = 0;
    auto sub = testObjectLarge.CreateNewSub([&subCalled](TestObjectMsg e) { subCalled++; });

    for (int i = 0; i < 128; i++)
    {
        testObjectLarge.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_GT(subCalled, 0);
    EXPECT_NE(subCalled, 128);

    // Let the buffer empty before rechecking
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto subPrevCalled = subCalled;
    testObjectLarge.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_EQ(subCalled, subPrevCalled + 1);

    sub->End();
}

TEST_F(EventTest, abandonedSubs)
{
    int subCalled = 0, sub2Called = 0;

    // Make sure the publisher has events to attempt sending to the rapidly destructed subscriber
    for (int i = 0; i < 128; i++)
    {
        testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

    auto sub = testObject.CreateNewSub([&subCalled](TestObjectMsg e) { subCalled++; });
    sub->End();

    auto sub2 = testObject.CreateNewSub([&sub2Called](TestObjectMsg e) { sub2Called++; });

    // Continue sending events to ensure the dead sub is handled correctly
    for (int i = 0; i < 128; i++)
    {
        testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Although it's extraordinarily unlikely for a message to be sent before the sub ends, best to play it safe and just EXPECT 0
    EXPECT_EQ(subCalled, 0);
    ASSERT_NE(sub2Called, 0);

    auto prevSubCalled = subCalled;
    auto prevSub2Called = sub2Called;

    testObject.SendEvent(TestObjectMsg(TestObjectEventBits::TEST1, 20, 20.f));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_EQ(subCalled, prevSubCalled);
    ASSERT_NE(sub2Called, prevSub2Called);
}