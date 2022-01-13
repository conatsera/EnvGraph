#ifndef ENVGRAPH_SUBSYSTEMS_H
#define ENVGRAPH_SUBSYSTEMS_H

#include <memory>

#include <common.h>

namespace EnvGraph
{

class Engine;


template<GpuApiSetting api_setting>
class GraphicsSubsystem
{
  public:
    ~GraphicsSubsystem() = default;

    void Init(Engine *engine);
    void DeInit();

    void Start();
    void Stop();

    void EnableRenderPass();
    void DisableRenderPass();
    void EnableComputePass();
    void DisableComputePass();
};

}

#endif