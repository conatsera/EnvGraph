#ifndef ENVGRAPH_TEST_BASE_H
#define ENVGRAPH_TEST_BASE_H

#include <thread>

#include <gtest/gtest.h>
#include <xcb/xcb.h>

#include "engine.h"

namespace EnvGraph
{

class TestBase : public ::testing::Test {
protected:
    void SetUp() override {
        SetupXWindow();
        SetupEngine();
        SetupXInputThread();
    }

    void TearDown() override {
        m_aEngine.reset();

        xcb_destroy_window_checked(m_xConnection, m_xWindow);

        m_shutdownSignal = true;
        m_xInputThread.join();

        xcb_disconnect(m_xConnection);
    }

    void SetupXWindow() {
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
    }

    void SetupXInputThread() {
        m_xInputThread = std::thread([this] {
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
    }

    void SetupEngine() {
#ifdef VK_USE_PLATFORM_XCB_KHR
        m_aEngine = std::make_unique<Engine::Engine>(m_xConnection, m_xWindow);
#elif

#endif
    }

protected:
    std::unique_ptr<Engine::Engine> m_aEngine;
    xcb_connection_t* m_xConnection;
    xcb_window_t m_xWindow;

    bool m_shutdownSignal = false;
    std::thread m_xInputThread;
};



} // namespace EnvGraph


#endif