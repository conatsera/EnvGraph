// Tests an edge filter using the kinect mesh shader pipeline

#include <cstdlib>

#include <gtest/gtest.h>

#include <glm/gtx/quaternion.hpp>

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

        std::srand((unsigned int)std::time(nullptr));
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
//constexpr const size_t kColorSize = 4096 * 3072;

static void GenerateEdge(uint16_t *depthBuffer, glm::vec<4, uint8_t> *colorBuffer)
{
    auto x = ((double)std::rand() / (double)RAND_MAX) * 1024;
    auto y = ((double)std::rand() / (double)RAND_MAX) * 1024;
    auto z = ((double)std::rand() / (double)RAND_MAX) * UINT16_MAX;
    auto width = std::floor(((double)std::rand() / (double)RAND_MAX) * 16);
    auto length = ((double)std::rand() / (double)RAND_MAX) * 128;


    ASSERT_LT(x, UINT16_MAX);
    ASSERT_GT(x, 0);
    ASSERT_LT(y, UINT16_MAX);
    ASSERT_GT(y, 0);
    ASSERT_LT(z, UINT16_MAX);
    ASSERT_GT(z, 0);

    ASSERT_LT(width, 16);
    ASSERT_GT(width, 0);
    ASSERT_LT(length, 128);
    ASSERT_GT(length, 0);
    
    glm::highp_quat edgeQuat = glm::identity<glm::highp_quat>();

    for (int i = 0; i < (int)std::ceil(length); i++)
    {
        auto edgeAngles = glm::eulerAngles(edgeQuat);
        auto xPixel = (int)std::floor(x + (std::sin(edgeAngles.x) * i));
        auto yPixel = (int)std::floor(y + (std::cos(edgeAngles.y) * i));
        auto zNext = (uint16_t)std::floor(z + (std::tan(edgeAngles.z) * i));
        auto pixel = xPixel + (yPixel * 1024);
        //std::cout << i << ": { " << xPixel << ", " << yPixel << " }, pixel:" << pixel << ", depth:" << zNext << std::endl;
        ASSERT_LT(pixel, kDepthSize);
        assert(zNext < UINT16_MAX && zNext > 0);
        depthBuffer[pixel] = 0xFFFF;
    }
}

template<uint32_t n>
static void GenerateEdges(uint16_t* depthBuffer, glm::vec<4, uint8_t> *colorBuffer) {
    for (uint32_t i = 0; i < n; i++) {
        GenerateEdge(depthBuffer, colorBuffer);
    }
}

enum EdgeResolution {
    STANDARD = 256,
    HIGH = 1024,
};

TEST_F(EdgeTest, populateBuffers)
{
    //uint16_t* depthBuffer = (uint16_t*)malloc(kDepthSize);
    //glm::vec<4, uint8_t>* colorBuffer = (glm::vec<4, uint8_t>*)malloc(kColorSize);

    bool bufferCbHasRun = false;

    auto bufferCb = [&bufferCbHasRun](uint16_t *pipelineDepthBuf, glm::vec<4, uint8_t>* pipelineColorBuf) -> bool {
        GenerateEdges<STANDARD>(pipelineDepthBuf, pipelineColorBuf);

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

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ASSERT_EQ(bufferCbHasRun, true);
}

TEST_F(EdgeTest, drawCube) {
    
}