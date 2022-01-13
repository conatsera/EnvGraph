/* Copyright (c) 2021 Austin Conatser
 * Tests filtering of events messages
 */
#include <gtest/gtest.h>

#include "engine.h"

#include "../base.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class EngineTest : public TestBase
{
  protected:
    std::shared_ptr<UI::View> m_engineTestView;
    std::unique_ptr<Engine> m_engineTestEngine;
};

TEST_F(EngineTest, init)
{
    ASSERT_EQ(m_engine->Init(), true);
}

TEST_F(EngineTest, start)
{
    ASSERT_EQ(m_engine->Init(), true);
    m_engine->StartRender();
}

TEST_F(EngineTest, invalidView)
{
    SetupEngine(m_engineTestEngine, m_engineTestView);
    ASSERT_EQ(m_engineTestEngine->Init(), false);
}