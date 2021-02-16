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

        m_kinectMesh = new KinectMesh();
        m_aEngine->NewPipeline(m_kinectMesh);

        std::srand(std::time(nullptr));
    }

  protected:
    KinectMesh* m_kinectMesh;
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

    auto x = ((float)std::rand() / (float)RAND_MAX) * 1024;
    auto y = ((float)std::rand() / (float)RAND_MAX) * 1024;
    //auto width = std::floor(((float)std::rand() / (float)RAND_MAX) * 16);
    auto length = ((float)std::rand() / (float)RAND_MAX) * 16;
    auto angle = ((float)std::rand() / (float)RAND_MAX) * 360.f;

    for (int i = 0; i < (int)std::ceil(length); i++)
    {
        auto xPixel = x + (std::sin(angle) * i);
        auto yPixel = y + (std::cos(angle) * i);
        auto pixel = xPixel + (yPixel * 1024);
        std::cout << i << ": { " << xPixel << ", " << yPixel << " }: " << pixel << std::endl;
    }

    for (size_t i = 0; i < kDepthSize; i++)
    {
        // TODO: Generate some noise
    }

    for (size_t i = 0; i < kColorSize; i++)
    {

    }
}

TEST_F(EdgeTest, populateBuffers)
{
    //uint16_t* depthBuffer = (uint16_t*)malloc(kDepthSize);
    //glm::vec<4, uint8_t>* colorBuffer = (glm::vec<4, uint8_t>*)malloc(kColorSize);

    bool bufferCbHasRun = false;

    auto bufferCb = [&bufferCbHasRun](uint16_t *pipelineDepthBuf, glm::vec<4, uint8_t>* pipelineColorBuf) -> bool {
        GenerateEdge(pipelineDepthBuf, pipelineColorBuf);

        bufferCbHasRun = true;

        //memcpy(pipelineDepthBuf, depthBuffer, kDepthSize);
        //memcpy(pipelineColorBuf, colorBuffer, kColorSize);
        return true;
    };

    m_kinectMesh->SetupBufferCallback(bufferCb);

    m_aEngine->StartRender();

    ASSERT_EQ(m_kinectMesh->IsValid(), true);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::seconds(3);

    while (!bufferCbHasRun && timeout > std::chrono::high_resolution_clock::now())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ASSERT_EQ(bufferCbHasRun, true);
}