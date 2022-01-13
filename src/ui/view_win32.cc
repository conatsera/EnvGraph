#include "ui/view.h"

#include <ShellScalingApi.h>

#define ASSERT(expression) _ASSERTE(expression)

namespace EnvGraph
{
namespace UI
{

constexpr const UINT_PTR kGestureEngineTimerID = 10;

View::View(std::shared_ptr<UI::InputManager> inputManager) : m_inputManager(inputManager)
{
    m_occlusion = DWORD(0.0);
}

View::~View()
{
    m_hWnd = nullptr;
}

void CALLBACK View::InteractionCallback(_In_opt_ void *clientData,
                                         _In_reads_(1) const INTERACTION_CONTEXT_OUTPUT *output)
{
    std::cout << output->inputType << " : ( " << output->x << ", " << output->y << " )" << std::endl;
}

HRESULT View::Initialize(_In_ Extent extent, _In_ std::wstring title)
{
    RECT bounds;

    bounds.left = 10;
    bounds.top = 10;
    bounds.right = extent.x;
    bounds.bottom = extent.y;

    // Create main application window.
    m_hWnd = __super::Create(nullptr, bounds, title.c_str());
    if (!m_hWnd)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Initialize member variables with default values.
    m_visible = TRUE;
    m_occlusion = DWORD(0.0);

    // Enable mouse to act as pointing device for this application.
    if (!EnableMouseInPointer(TRUE))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT res = S_OK;
    res = CreateInteractionContext(&m_interactionContext);
    if (res != S_OK)
    {
        std::cout << "CreateInteractionContext failed: " << GetLastError() << std::endl;
    }
    else
    {
        res = RegisterOutputCallbackInteractionContext(m_interactionContext, InteractionCallback, this);
        if (res != S_OK)
        {
            std::cout << "RegisterOutputCallbackInteractionContext failed: " << GetLastError() << std::endl;
        }
        res = SetPropertyInteractionContext(m_interactionContext, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE);
        if (res != S_OK)
        {
            std::cout << "SetPropertyInteractionContext failed: " << GetLastError() << std::endl;
        }
        INTERACTION_CONTEXT_CONFIGURATION cfg[] = INTERACTION_CONTEXT_CONFIGURATION_DEFAULT;
        res = SetInteractionConfigurationInteractionContext(m_interactionContext, sizeof(cfg) / sizeof(cfg[0]), cfg);
        if (res != S_OK)
        {
            std::cout << "SetInteractionConfigurationInteractionContext failed: " << GetLastError() << std::endl;
        }
        AddPointerInteractionContext(m_interactionContext, 0);
    }

    return S_OK;
}

HRESULT View::Run()
{
    HRESULT hr = S_OK;

    MSG message = {};
    do
    {
        if (m_visible)
        {
            // hr = Render();
        }
        else
        {
            WaitMessage();
        }

        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);

            DispatchMessage(&message);
        }
    } while (message.message != WM_QUIT);

    return hr;
}

LRESULT View::OnCreate(_In_ UINT, _In_ WPARAM, _In_ LPARAM lParam, _Out_ BOOL &bHandled)
{
    auto cs = reinterpret_cast<CREATESTRUCT *>(lParam);

    auto monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    UINT dpix;
    UINT dpiy;
    if (FAILED(GetDpiForMonitor(monitor, MONITOR_DPI_TYPE::MDT_EFFECTIVE_DPI, &dpix, &dpiy)))
    {
        dpix = 96;
        dpiy = 96;
    }
    auto windowDpi = static_cast<float>(dpix);
    // TODO:

    cs->style |= (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    cs->dwExStyle |= (WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);

    SetWindowLong(GWL_STYLE, cs->style);
    SetWindowLong(GWL_EXSTYLE, cs->dwExStyle);
    ASSERT(SetWindowPos(nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER));

    bHandled = TRUE;
    return 0;
}

//
// Destroying this window will also quit the application.
//
LRESULT
View::OnDestroy(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    m_quit = true;

    PostQuitMessage(0);

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnWindowPosChanged(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    RECT clientRect;
    auto windowPos = reinterpret_cast<WINDOWPOS *>(lparam);
    GetClientRect(&clientRect);
    if (!(windowPos->flags & SWP_NOSIZE))
    {
        // DeviceResources::Size size;
        auto width = static_cast<uint32_t>(clientRect.right - clientRect.left) / (m_dpi / 96.0F);
        auto height = static_cast<uint32_t>(clientRect.bottom - clientRect.top) / (m_dpi / 96.0F);
        // m_deviceResources->SetLogicalSize(size);
        // Render();
        // TODO: Send resize info
        NewEvent(ViewMsg(Extent{width, height}));
    }

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnDisplayChange(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    RECT current;
    ZeroMemory(&current, sizeof(current));
    GetWindowRect(&current);

    //float oldDpix, oldDpiy, newDpi;
    //m_deviceResources->GetD2DDeviceContext()->GetDpi(&oldDpix, &oldDpiy);
    auto newDpi = GetDpiForWindow();

    if (m_dpi != newDpi)
    {
        auto newRect = CalcWindowRectNewDpi(current, m_dpi, newDpi);
        m_dpi = newDpi;
        SetWindowPos(0, &newRect, 0);
    }
    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnGetMinMaxInfo(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    auto minMaxInfo = reinterpret_cast<MINMAXINFO *>(lparam);

    minMaxInfo->ptMinTrackSize.y = 200;

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnActivate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    m_visible = !HIWORD(wparam);

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnOcclusion(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    ASSERT(m_occlusion);

    /*if (S_OK == m_deviceResources->GetSwapChain()->Present(0, DXGI_PRESENT_TEST))
    {
        m_deviceResources->GetDxgiFactory()->UnregisterOcclusionStatus(m_occlusion);
        m_occlusion = 0;
        m_visible = true;
    }*/

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnPointerDown(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localx = static_cast<float>(pt.x); // / (m_deviceResources->GetDpi() / 96.0F);
    auto localy = static_cast<float>(pt.y); // / (m_deviceResources->GetDpi() / 96.0F);

    PointerHandling(GET_POINTERID_WPARAM(wparam));

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnPointerUpdate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localx = static_cast<float>(pt.x); // / (m_deviceResources->GetDpi() / 96.0F);
    auto localy = static_cast<float>(pt.y); // / (m_deviceResources->GetDpi() / 96.0F);

    PointerHandling(GET_POINTERID_WPARAM(wparam));

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnPointerUp(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localX = static_cast<float>(pt.x); // / (m_deviceResources->GetDpi() / 96.0F);
    auto localY = static_cast<float>(pt.y); // / (m_deviceResources->GetDpi() / 96.0F);

    PointerHandling(GET_POINTERID_WPARAM(wparam));

    bHandled = TRUE;
    return 0;
}

LRESULT View::OnPointerWheel(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    PointerHandling(GET_POINTERID_WPARAM(wparam));

    bHandled = TRUE;
    return 0;
}

LRESULT View::OnPointerHWheel(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    PointerHandling(GET_POINTERID_WPARAM(wparam));

    bHandled = TRUE;
    return 0;
}

LRESULT View::OnPointerCaptureChange(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    StopInteractionContext(m_interactionContext);
    bHandled = TRUE;
    return 0;
}

void View::PointerHandling(UINT pointerId)
{
    // Get frame id from current message.
    POINTER_INFO pointerInfo = {};
    if (GetPointerInfo(pointerId, &pointerInfo))
    {
        if (pointerInfo.frameId != m_frameId)
        {
            // This is the new frame to process.
            m_frameId = pointerInfo.frameId;

            // Find out pointer count and frame history length.
            UINT entriesCount = 0;
            UINT pointerCount = 0;
            if (GetPointerFrameInfoHistory(pointerId, &entriesCount, &pointerCount, NULL))
            {
                // Allocate space for pointer frame history.
                POINTER_INFO *pointerInfoFrameHistory = NULL;
                try
                {
                    pointerInfoFrameHistory = new POINTER_INFO[entriesCount * pointerCount];
                }
                catch (...)
                {
                    pointerInfoFrameHistory = NULL;
                }

                if (pointerInfoFrameHistory != NULL)
                {
                    // Retrieve pointer frame history.
                    if (GetPointerFrameInfoHistory(pointerId, &entriesCount, &pointerCount, pointerInfoFrameHistory))
                    {
                        // As multiple frames may have occurred, we need to process them.
                        ProcessPointerFramesInteractionContext(m_interactionContext, entriesCount, pointerCount,
                                                               pointerInfoFrameHistory);
                    }

                    delete[] pointerInfoFrameHistory;
                    pointerInfoFrameHistory = NULL;
                }
            }
        }
    }
}

LRESULT View::OnTimerUpdate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    if ((wparam == kGestureEngineTimerID) && (m_timerId != 0))
    {
        ProcessInertiaInteractionContext(m_interactionContext);
    }
    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnEnterSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    // Call handler implemented by derived class for WindowPosChanging message.
    // OnEnterSizeMove();

    bHandled = TRUE;
    return 0;
}

LRESULT
View::OnExitSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled)
{
    // Call handler implemented by derived class for WindowPosChanging message.
    // OnExitSizeMove();

    bHandled = TRUE;
    return 0;
}

void View::ResizeWindow(uint32_t width, uint32_t height)
{
    RECT clientRect, windowRect;
    
    GetClientRect(&clientRect);
    GetWindowRect(&windowRect);

    auto x = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    auto y = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

    windowRect.right = windowRect.left + width + x;
    windowRect.bottom = windowRect.top + height + y;

    MoveWindow(&windowRect);
}

LRESULT
View::OnDpiChange(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled)
{
    auto lprcNewScale = reinterpret_cast<LPRECT>(lparam);

    float changedDpi = static_cast<float>(LOWORD(wparam));
    m_dpi = changedDpi;
    SetWindowPos(0, lprcNewScale, 0);

    bHandled = TRUE;
    return 0;
}

RECT View::CalcWindowRectNewDpi(_In_ RECT oldRect, _In_ float oldDpi, _In_ float newDpi)
{
    float oldWidth = static_cast<float>(oldRect.right - oldRect.left);
    float oldHeight = static_cast<float>(oldRect.bottom - oldRect.top);

    int newWidth = static_cast<int>(oldWidth * newDpi / oldDpi);
    int newHeight = static_cast<int>(oldHeight * newDpi / oldDpi);

    RECT newRect = {oldRect.left, oldRect.top, newWidth, oldRect.top + newHeight};
    return newRect;
}

float View::GetDpiForWindow()
{
    auto monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

    UINT dpix;
    UINT dpiy;
    if (FAILED(GetDpiForMonitor(monitor, MONITOR_DPI_TYPE::MDT_EFFECTIVE_DPI, &dpix, &dpiy)))
    {
        dpix = 96;
        dpiy = 96;
    }
    return static_cast<float>(dpix);
}

} // namespace UI
} // namespace EnvGraph