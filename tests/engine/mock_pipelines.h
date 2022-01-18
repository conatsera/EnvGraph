#ifndef ENVGRAPH_TEST_ENGINE_MOCK_PIPELINES_H
#define ENVGRAPH_TEST_ENGINE_MOCK_PIPELINES_H

#include <pipelines.h>

using namespace EnvGraph;

class MockPipeline : public Pipelines::Base
{
  public:
    void Setup(Pipelines::SetupInfo<kGpuApiSetting> setupInfo) final
    {
        m_setupComplete = true;
    }
    void Cleanup(Pipelines::SetupInfo<kGpuApiSetting>) final
    {
    }

    const Pipelines::QueueRequirements_t GetQueueRequirements() const
    {
        return {1, 0};
    }

    void EnableResources()
    {
        m_resourcesEnabled = true;
    }

    virtual void Render(Pipelines::RenderInfo<kGpuApiSetting> renderInfo) final
    {
    }

    bool HasRun() const
    {
        return true;
    }

  private:

    bool m_resourcesEnabled = false;
};

class MockPipelineTwoStage : public Pipelines::Base
{
  public:
    void Setup(Pipelines::SetupInfo<kGpuApiSetting> setupInfo) final
    {
        m_setupComplete = true;
    }
    void Cleanup(Pipelines::SetupInfo<kGpuApiSetting>) final
    {
    }

    const Pipelines::QueueRequirements_t GetQueueRequirements() const
    {
        return { 1, 0 };
    }

    void EnableResources()
    {
        m_resourcesEnabled = true;
    }

    virtual void Render(Pipelines::RenderInfo<kGpuApiSetting> renderInfo) final
    {
    }

    bool HasRun() const
    {
        return true;
    }

  private:

    bool m_resourcesEnabled = false;
};

#endif