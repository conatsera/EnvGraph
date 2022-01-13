#ifndef ENVGRAPH_TEST_BASE_H
#define ENVGRAPH_TEST_BASE_H

#include <thread>

#include <gtest/gtest.h>
#ifdef _WIN32
#include <Windows.h>
#include <windowsx.h>
#else
#include <xcb/xcb.h>
#endif

#include "engine.h"
#include "ui/view_controller.h"
#include "ui/input.h"

namespace EnvGraph
{
#ifndef TEST_DEFAULT_WIDTH
#define TEST_DEFAULT_WIDTH 512
#endif
#ifndef TEST_DEFAULT_HEIGHT
#define TEST_DEFAULT_HEIGHT 512
#endif

constexpr const uint32_t kDefaultWidth = TEST_DEFAULT_WIDTH;
constexpr const uint32_t kDefaultHeight = TEST_DEFAULT_HEIGHT;

class TestBase : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        SetupViewController();
        SetupView(m_view);
        SetupEngine(m_engine, m_view);

        m_engineReady = true;
    }

    void TearDown() override
    {
        if (!m_view->HasQuit())
            m_view->Run();
        m_engineReady = false;
        m_engine.reset();
        m_engine.release();
        m_view.reset();
    }

    void SetupInputManager()
    {
        m_inputManager = std::make_shared<UI::InputManager>();
    }

    void SetupViewController()
    {
        m_viewController = std::make_unique<UI::ViewController>();
    }

    void SetupView(std::shared_ptr<UI::View> &view)
    {
        view = std::make_shared<UI::View>(m_inputManager);

        view->SetTitle("Test");

        m_viewController->NewView(EnvGraph::UI::ViewType::LOCAL, view, kDefaultWidth, kDefaultHeight);
    }

    void SetupEngine(std::unique_ptr<Engine> &engine, std::shared_ptr<UI::View> &view)
    {
        engine = std::make_unique<Engine>("TestApp");

        engine->ConfigureView(view);

        engine->Init();
    }

  protected:
    bool m_engineReady = false;
    std::unique_ptr<Engine> m_engine;

    std::shared_ptr<UI::InputManager> m_inputManager;
    std::unique_ptr<UI::ViewController> m_viewController;

    std::shared_ptr<UI::View> m_view;
};

} // namespace EnvGraph

#endif