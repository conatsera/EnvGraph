#ifndef ENVGRAPH_UI_VIEW_H
#define ENVGRAPH_UI_VIEW_H

#include <array>
#include <string>
#include <thread>

#ifdef _WIN32
#include <Windows.h>

#include <atlbase.h>
#include <atlwin.h>

#include <InteractionContext.h>
#else

#endif

#include "common.h"
#include "events.h"

#include "ui/input.h"

namespace EnvGraph
{
namespace UI
{

struct ViewInputDescriptor
{
};

enum ViewType
{
    LOCAL,
    REMOTE_LINUX,
    REMOTE_WINDOWS,
};

struct ViewMsg : Message
{
    ViewMsg() : Message(Events::RENDER, 0)
    {
    }
    ViewMsg(RenderEventBits type) : Message(Events::RENDER, type)
    {
    }
    ViewMsg(Extent newExtent) : Message(Events::RENDER, RenderEventBits::RESIZE), m_newExtent(newExtent)
    {
    }
    ViewMsg(bool hidden) : Message(Events::RENDER, RenderEventBits::VISIBILITY), m_hidden(hidden)
    {
    }
    const bool m_hidden = false;
    const Extent m_newExtent{0, 0};
    /*constexpr ViewMsg &operator=(ViewMsg &rhs) noexcept {
        m_hidden = rhs.m_hidden;
        for (size_t i = 0; i < rhs.m_newExtent.length(); i++)
        {
            m_newExtent[i] = rhs.m_newExtent[i];
        }
    }*/
};


#ifdef WIN32
class View : 
             public Publisher<ViewMsg, RenderEventBits, 128>,
             public CWindowImpl<View, CWindow, CWinTraits<WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_SIZEBOX>>
#else
class View : public Publisher<ViewMsg, RenderEventBits>
#endif
{
  public:
    View(std::shared_ptr<InputManager> inputManager);
    ~View();

    std::string GetTitle() const
    {
        return m_title;
    }
    void SetTitle(std::string title)
    {
        m_title.assign(title);
    }

    Extent GetExtent() const
    {
        int w = 0, h = 0;
        // if (m_window != nullptr)
        // SDL_GetWindowSize(m_window, &w, &h);
        // TODO: Get window size
        return {w, h};
    }
    void ResizeWindow(Extent newExtent)
    {
        ResizeWindow(newExtent.x, newExtent.y);
    }
    void ResizeWindow(uint32_t width, uint32_t height);

    void Setup();

    bool IsInitialized() const
    {
#ifdef WIN32
        return ::IsWindow(m_hWnd);
#endif
    }

    bool HasQuit() const
    {
        return m_quit;
    }

    bool IsVisible() const;

#ifdef WIN32
    HWND GetWindow() const
    {
        return m_hWnd;
    };
#else
    xcb_window_t GetWindow() const;
#endif

  private:
    std::shared_ptr<InputManager> m_inputManager;

    bool m_quit = false;
    bool m_visible;
    std::string m_title{""};
    float m_dpi = 96.0F;

  private:
    void Pause();
    void Debug();

#ifdef WIN32
  public:
    BEGIN_MSG_MAP(c)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
    MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
    MESSAGE_HANDLER(WM_USER, OnOcclusion)
    MESSAGE_HANDLER(WM_POINTERDOWN, OnPointerDown)
    MESSAGE_HANDLER(WM_POINTERUP, OnPointerUp)
    MESSAGE_HANDLER(WM_POINTERUPDATE, OnPointerUpdate)
    MESSAGE_HANDLER(WM_POINTERWHEEL, OnPointerWheel)
    MESSAGE_HANDLER(WM_POINTERHWHEEL, OnPointerHWheel)
    MESSAGE_HANDLER(WM_POINTERCAPTURECHANGED, OnPointerCaptureChange)
    MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
    MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
    MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChange)
    MESSAGE_HANDLER(WM_TIMER, OnTimerUpdate)
    END_MSG_MAP()

    DECLARE_WND_CLASS_EX(nullptr, 0, -1);

    HRESULT Initialize(_In_ Extent bounds, _In_ std::wstring title);
    HRESULT Run();

  private:
    static void CALLBACK InteractionCallback(_In_opt_ void *clientData,
                                             _In_reads_(1) const INTERACTION_CONTEXT_OUTPUT *output);

    void PointerHandling(UINT pointerId);

    LRESULT OnCreate(_In_ UINT, _In_ WPARAM, _In_ LPARAM lParam, _Out_ BOOL &bHandled);
    LRESULT OnDestroy(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnWindowPosChanged(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnDisplayChange(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnGetMinMaxInfo(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnActivate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnOcclusion(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnPointerDown(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerUp(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerUpdate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerWheel(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerHWheel(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerCaptureChange(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnEnterSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnExitSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnDpiChange(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnTimerUpdate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    // Store DPI information for use by this class.
    static RECT CalcWindowRectNewDpi(_In_ RECT oldRect, _In_ float oldDpi, _In_ float newDPI);
    float GetDpiForWindow();

    DWORD m_occlusion = DWORD(0.0);
    HINTERACTIONCONTEXT m_interactionContext = NULL;
    UINT_PTR m_timerId;
    UINT m_frameId;
#endif

};

} // namespace UI
} // namespace EnvGraph

#endif