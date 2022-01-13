#ifndef ENVGRAPH_TESTS_DEPTH_UT_BASE_H
#define ENVGRAPH_TESTS_DEPTH_UT_BASE_H

#include "../base.h"

#include "graphics/depth_mesh.h"
#include "graphics/pipelines/holographic.h"

using EnvGraph::Graphics::DepthMesh;
using EnvGraph::Graphics::HoloPipeline;

class EdgeTest : public EnvGraph::TestBase
{
  protected:
    void SetUp() override
    {
        EnvGraph::TestBase::SetUp();

        m_depthMesh = new DepthMesh();
        m_holoPipeline = new HoloPipeline();
        m_engine->NewPipeline(m_holoPipeline);

        std::srand((unsigned int)std::time(nullptr));
    }

  protected:
    DepthMesh *m_depthMesh;
    HoloPipeline *m_holoPipeline;
};

#endif