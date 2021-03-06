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

namespace EnvGraph
{

class TestBase : public ::testing::Test {
protected:
    void SetUp() override {
        SetupWindow();
        SetupEngine();

        m_engineReady = true;

        SetupInputThread();
    }

    void TearDown() override {
        m_engineReady = false;
        m_aEngine.reset();

#ifdef _WIN32
        CloseWindow(m_window);
#else
        xcb_destroy_window_checked(m_xConnection, m_xWindow);
#endif

        m_shutdownSignal = true;
        m_inputThread.join();
#ifdef _WIN32

#else
        xcb_disconnect(m_xConnection);
#endif
    }

    void SetupWindow() {
        
#ifdef _WIN32
        WNDCLASS wc = {0};

        wc.lpfnWndProc   = View::WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = L"Test Window Class";

        RegisterClass(&wc);

        m_window = CreateWindowEx(WS_EX_APPWINDOW, L"Test Window Class", L"Test Window",
                                WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                kDefaultWidth, kDefaultHeight, NULL, NULL, GetModuleHandle(NULL), this);
        std::this_thread::sleep_for(std::chrono::seconds(1));
#else
        m_xConnection = xcb_connect(NULL, NULL);

        const xcb_setup_t *xSetup = xcb_get_setup(m_xConnection);
        xcb_screen_iterator_t xScreenIter = xcb_setup_roots_iterator(xSetup);
        xcb_screen_t *mainScreen = xScreenIter.data;

        auto xcbMask = XCB_CW_EVENT_MASK;
        xcb_event_mask_t xcbValMask[] = {
        (xcb_event_mask_t)(XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS)};

        m_xWindow = xcb_generate_id(m_xConnection);
        xcb_create_window(m_xConnection, XCB_COPY_FROM_PARENT, m_xWindow, mainScreen->root, 0, 0, kDefaultWidth, kDefaultHeight,
                      10, XCB_WINDOW_CLASS_INPUT_OUTPUT, mainScreen->root_visual, xcbMask, xcbValMask);

        xcb_map_window(m_xConnection, m_xWindow);

        auto flushRet = xcb_flush(m_xConnection);
        if (flushRet != 0)
        {
            //std::cerr << "XCB Flush failed: " << flushRet << std::endl;
        }
#endif
    }

    void SetupInputThread() {
        
#ifndef _WIN32
        m_inputThread = std::thread([this] {
            vk::Extent2D currentWindowExtents{kDefaultWidth, kDefaultHeight};

            while (!m_shutdownSignal)
            {

                auto newEvent = xcb_wait_for_event(m_xConnection);
                if (newEvent != nullptr)
                {
                    switch (newEvent->response_type)
                    {
                    case XCB_EXPOSE: {
                        xcb_expose_event_t *exposeEvent = (xcb_expose_event_t *)newEvent;
                        currentWindowExtents.setWidth(exposeEvent->width + exposeEvent->x);
                        currentWindowExtents.setHeight(exposeEvent->height + exposeEvent->y);
                        m_aEngine->UpdateWindowExtents(exposeEvent->width + exposeEvent->x,
                                                        exposeEvent->height + exposeEvent->y);
                        break;
                    }
                    default:
                        break;
                    }
                } else {
                    m_shutdownSignal = true;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(250));
            }
        });
#endif
    }

    void SetupEngine() {
#ifdef VK_USE_PLATFORM_XCB_KHR
        m_aEngine = std::make_unique<Engine::Engine>(m_xConnection, m_xWindow);
#else
        m_aEngine = std::make_unique<Engine::Engine>(m_window);
#endif
    }

protected:
    bool m_engineReady = false;
    std::unique_ptr<Engine::Engine> m_aEngine;

#ifdef _WIN32
    HWND m_window = NULL;
#else
    xcb_connection_t* m_xConnection;
    xcb_window_t m_xWindow;
#endif

    bool m_shutdownSignal = false;
    std::thread m_inputThread;
};



} // namespace EnvGraph


#endif