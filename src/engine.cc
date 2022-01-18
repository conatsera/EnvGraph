#include "engine.h"

#include <assert.h>

namespace EnvGraph
{

Engine::Engine(std::string appName) : m_appName(appName)
{
}

Engine::~Engine()
{
    m_resizeSub->End();
}

void Engine::ConfigureView(std::shared_ptr<UI::View> view)
{
    if (view)
    {
        m_view = view;
        m_resizeSub = m_view->CreateNewSub(
            RenderEventBits::RESIZE, [this](UI::ViewMsg e) { SetRenderResolution(e.m_newExtent, e.m_resizeEnd); });
    }
    else
    {
        std::cerr << "Invalid view configured" << std::endl;
    }
}

bool Engine::Init()
{
    assert(m_view);
#if _WIN32
    const bool isInitialized = (m_view->IsInitialized());
#elif __linux__
    const bool isInitialized = (m_view->GetNativeXWindow()) && (m_view->GetNativeXDisplay());
#endif
    if (isInitialized)
    {
        m_graphicsSubsystem.Init(this);
        return true;
    }
    else
        return false;
}

void Engine::StartRender()
{
    m_graphicsSubsystem.Start();
}

void Engine::NewPipeline(Pipelines::Base *newPipeline)
{
    m_graphicsSubsystem.NewPipeline(newPipeline);
    m_pipelines.push_back(newPipeline);
}

void Engine::StopRender()
{
    m_graphicsSubsystem.Stop();
}

} // namespace EnvGraph