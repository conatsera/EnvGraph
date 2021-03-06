#ifndef ENVGRAPH_UI_VIEW_CONTROLLER_H
#define ENVGRAPH_UI_VIEW_CONTROLLER_H

#include <array>
#include <cstdint>
#include <cstdlib>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <xcb/xcb.h>
#endif

#include "engine.h"

namespace EnvGraph
{

struct ViewEvent
{
};

struct ViewInputDescriptor
{
};

class View
{
  public:
    View();
    ~View();

#ifdef WIN32
    bool SetWin32(HWND window);
#else
    bool SetXCB(xcb_connection_t *xConnection);
#endif

    ViewEvent AttachTo(ViewInputDescriptor inputDesc);

  private:
  bool m_engineReady = false;
  Engine::Engine* m_aEngine = nullptr;
#ifdef WIN32
    HWND m_window{NULL};
#else
    xcb_connection_t *m_xConnection = nullptr;
    xcb_window_t m_window = xcb_window_enum_t::XCB_WINDOW_NONE;
#endif
  private:
    bool m_inputThreadActive = false;
    std::thread m_inputThread;

    std::array<uint32_t, 2> m_extents;
    std::array<uint32_t, 2> m_pointerLocation;

    void Pause();
    void Debug();

    // TODO: Add an EngineInput interface
    void StartInput(Engine::Engine *engine);
  public:
#ifdef _WIN32
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
};

enum ViewType
{
    LOCAL_LINUX,
    LOCAL_WINDOWS,
    REMOTE_LINUX,
    REMOTE_WINDOWS,
};

class ViewController
{
  public:
    ViewController();
    ~ViewController();

    void NewView(ViewType type, View &view);

    int OpenView(View &view);
    int CloseView(View &view);

  private:
    void Cleanup();
    void TerminateConnection();

    void EnumerateLocalInstances();
    void EnumerateRemoteInstances();

  private:
#ifdef WIN32
#else
    xcb_connection_t *m_xConnection;
    xcb_window_t m_xWindow;
#endif
};

} // namespace EnvGraph

#endif