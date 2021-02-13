// Tests an edge filter using the kinect mesh shader pipeline

#include <gtest/gtest.h>

#include "engine.h"
#include "pipelines/kinect_mesh.h"

#include "../base.h"

using Engine::Pipelines::KinectMesh;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EdgeTest : public EnvGraph::TestBase {
protected:
    void SetUp() override {
        EnvGraph::TestBase::SetUp();

        m_kinectMesh = std::make_shared<KinectMesh>();
        m_aEngine->NewPipeline(m_kinectMesh);
    }

    void TearDown() override {
        m_kinectMesh.reset();

        EnvGraph::TestBase::TearDown();
    }
protected:
    std::shared_ptr<KinectMesh> m_kinectMesh;
};

TEST_F(EdgeTest, createPipeline) {
    m_aEngine->StartRender();

    ASSERT_EQ(m_kinectMesh->IsValid(), true);
}

