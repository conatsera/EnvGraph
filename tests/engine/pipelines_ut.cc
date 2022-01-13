/* Copyright (c) 2021 Austin Conatser
 *
 */
#include <gtest/gtest.h>

#include "pipelines.h"

#include "../base.h"
#include "mock_pipelines.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class PipelinesTest : public TestBase
{
  public:
    PipelinesTest()
    {
        m_testPipeline = new MockPipeline();
        m_testPipeline2 = new MockPipelineTwoStage();
    }
  protected:

    void CreatePipelines()
    {        
        m_engine->NewPipeline(m_testPipeline);
        ASSERT_EQ(m_testPipeline->IsSetup(), true);

        m_engine->NewPipeline(m_testPipeline2);
        ASSERT_EQ(m_testPipeline2->IsSetup(), true);
    }

  protected:
    MockPipeline *m_testPipeline;
    MockPipelineTwoStage *m_testPipeline2;
};

TEST_F(PipelinesTest, create)
{
    MockPipeline *testPipeline = new MockPipeline();
    m_engine->NewPipeline(testPipeline);
    ASSERT_EQ(testPipeline->IsSetup(), true);

    MockPipelineTwoStage *testPipeline2 = new MockPipelineTwoStage();
    m_engine->NewPipeline(testPipeline2);
    ASSERT_EQ(testPipeline2->IsSetup(), true);
}

TEST_F(PipelinesTest, createResources)
{
    m_testPipeline->EnableResources();
    m_testPipeline2->EnableResources();
}

TEST_F(PipelinesTest, run)
{
    CreatePipelines();

    m_engine->StartRender();

    ASSERT_EQ(m_testPipeline->HasRun(), true);
    ASSERT_EQ(m_testPipeline2->HasRun(), true);

    m_engine->StopRender();
}