#ifndef ENVGRAPH_ENGINE_H
#define ENVGRAPH_ENGINE_H

#include <array>
#include <mutex>
#include <memory>
#include <filesystem>

#include "events.h"

#include "ui/view.h"
#include "ui/input.h"
#include "pipelines.h"

#ifdef _WIN32
#include <Windows.h>
#else

#endif

#include "vk/subsystem.h"
#ifdef DX_SUBSYSTEM_ENABLED

#endif

namespace EnvGraph
{

enum EngineEventBits : EventTypeBase
{

};

struct EngineEvent : Message
{
    EngineEvent() : Message(Events::INVALID, 0)
    {
    }
};

enum class EngineMode
{
    VULKAN,
    DIRECTX
};

class Engine : public Publisher<EngineEvent, EngineEventBits, 64>
{
  public:
    Engine(std::string appName);
    ~Engine();

    bool Init();

    void UpdateWindowExtents(uint32_t width, uint32_t height, bool end)
    {

    }
    
    void ConfigureLog(bool enabledStdOut, std::filesystem::path logDirPath);
    void ConfigureView(std::shared_ptr<UI::View> view);

    void StartRender();
    void PauseRender();
    void StopRender();

    void NewPipeline(Pipelines::Base* newPipeline);

    std::string GetAppName() const
    {
        return m_appName;
    }

    UI::View* GetView() const
    {
        return m_view.get();
    }

    Extent GetRenderResolution() const
    {
        return m_renderResolution;
    }
    bool SetRenderResolution(Extent newRenderResolution, bool end)
    {
        if (m_graphicsSubsystem.CheckRenderResolutionLimits(newRenderResolution))
        {
            m_renderResolution = newRenderResolution;
            if (end)
                m_graphicsSubsystem.UpdateRenderResolution();
            return true;
        }
        else
            return false;
    }

  private:
    const std::string m_appName;

    std::mutex m_rendererActive;

    Extent m_renderResolution{0, 0};
    Subscriber<std::function<void(UI::ViewMsg)>, RenderEventBits> *m_resizeSub;

    GraphicsSubsystem<kGpuApiSetting> m_graphicsSubsystem;

    std::shared_ptr<UI::View> m_view;

    std::vector<Pipelines::Base*> m_pipelines;
};

} // namespace EnvGraph

#endif