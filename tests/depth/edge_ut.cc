// Tests an edge filter using the kinect mesh shader pipeline

#include <gtest/gtest.h>

#include "engine.h"
#include "pipelines/kinect_mesh.h"

#include "../base.h"

using Engine::Pipelines::KinectMesh;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EdgeTest : public EnvGraph::TestBase
{
  protected:
    void SetUp() override
    {
        EnvGraph::TestBase::SetUp();

        m_kinectMesh = std::make_shared<KinectMesh>();
        m_aEngine->NewPipeline(m_kinectMesh);
    }

    void TearDown() override
    {
        // Shutdown the Mesh generator prior to engine shutdown
        m_kinectMesh.reset();

        EnvGraph::TestBase::TearDown();
    }

  protected:
    std::shared_ptr<KinectMesh> m_kinectMesh;
};

TEST_F(EdgeTest, createPipeline)
{
    m_aEngine->StartRender();

    // A mesh generator without buffer callbacks is invalid
    ASSERT_EQ(m_kinectMesh->IsValid(), false);
}

TEST_F(EdgeTest, createBufferCallbacks)
{

    auto bufferCb = [](uint16_t *pipelineDepthBuf, glm::vec<4, uint8_t>* pipelineColorBuf) -> bool { return true; };

    m_kinectMesh->SetupBufferCallback(bufferCb);

    m_aEngine->StartRender();

    ASSERT_EQ(m_kinectMesh->IsValid(), true);
}

constexpr const size_t kDepthSize = 1024 * 1024;
constexpr const size_t kColorSize = 4096 * 3072;

static void GenerateEdge(uint16_t *depthBuffer, glm::vec<4, uint8_t> *colorBuffer)
{

   /* auto x = (float)std::rand() / (float)RAND_MAX;
    auto y = (float)std::rand() / (float)RAND_MAX;
    auto width = std::floor(((float)std::rand() / (float)RAND_MAX) * 16);
    auto length = (float)std::rand() / (float)RAND_MAX;
    auto angle = ((float)std::rand() / (float)RAND_MAX) * 360.f;

*/

    for (size_t i = 0; i < kDepthSize; i++)
    {

    }

    for (size_t i = 0; i < kColorSize; i++)
    {

    }
}

TEST_F(EdgeTest, populateBuffers)
{
    //uint16_t* depthBuffer = (uint16_t*)malloc(kDepthSize);
    //glm::vec<4, uint8_t>* colorBuffer = (glm::vec<4, uint8_t>*)malloc(kColorSize);

    auto bufferCb = [](uint16_t *pipelineDepthBuf, glm::vec<4, uint8_t>* pipelineColorBuf) -> bool {
        GenerateEdge(pipelineDepthBuf, pipelineColorBuf);

        //memcpy(pipelineDepthBuf, depthBuffer, kDepthSize);
        //memcpy(pipelineColorBuf, colorBuffer, kColorSize);
        return true;
    };

    m_kinectMesh->SetupBufferCallback(bufferCb);

    m_aEngine->StartRender();

    ASSERT_EQ(m_kinectMesh->IsValid(), true);
}