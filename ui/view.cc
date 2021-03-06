#include "view_controller.h"

namespace EnvGraph
{

View::View() {

}

View::~View() {

}

#ifdef WIN32
bool View::SetWin32(HWND window)
{
    if (m_window == NULL)
    {
        m_window = window;
        return true;
    }
    else
        return false;
}
#else
bool View::SetXCB(xcb_connection_t* xConnection) {
    if (m_window == xcb_window_enum_t::XCB_WINDOW_NONE) {
        m_xConnection = xConnection;
        return true;
    } else
        return false;
}
#endif

void View::Pause() {

}

void View::Debug() {

}

void View::StartInput(Engine::Engine* engine) {
    if (m_inputThreadActive)
    {
        m_inputThreadActive = false;
        m_inputThread.join();
    }
    m_inputThreadActive = true;
    m_inputThread = std::thread([this] {
            while (m_inputThreadActive)
            {
#ifdef WIN32

#else
                auto newEvent = xcb_wait_for_event(m_xConnection);
                if (newEvent != nullptr)
                {
                    switch (newEvent->response_type)
                    {
                    case XCB_EXPOSE: {
                        xcb_expose_event_t *exposeEvent = (xcb_expose_event_t *)newEvent;
                        m_extents[0] = exposeEvent->width + exposeEvent->x;
                        m_extents[1] = exposeEvent->height + exposeEvent->y;
                        engine->UpdateWindowExtents(exposeEvent->width + exposeEvent->x,
                                                        exposeEvent->height + exposeEvent->y);
                        break;
                    }
                    default:
                        break;
                    }
                } else {
                    m_inputThreadActive = false;
                    break;
                }
#endif
                std::this_thread::sleep_for(std::chrono::microseconds(250));
            }
    });
}

#ifdef _WIN32
LRESULT CALLBACK View::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    View* context = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        context = (View*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)context);
    }
    else
    {
        context = (View*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (uMsg)
    {
    case WM_SIZE:
    {
        if (context->m_engineReady)
            context->m_aEngine->UpdateWindowExtents(LOWORD(lParam), HIWORD(lParam));
        break;
    }
    case WM_MOUSEWHEEL:
    {
        //auto xPos = GET_X_LPARAM(lParam);
       // auto zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        break;
    }
    default:
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }
    return 0;
}

#endif

} // namespace EnvGraph
