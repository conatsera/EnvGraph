#ifndef ENVGRAPH_UI_VIEW_CONTROLLER_H
#define ENVGRAPH_UI_VIEW_CONTROLLER_H

#include <array>
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "events.h"
#include "ui/view.h"

namespace EnvGraph
{
namespace UI
{

class ViewController : public Publisher<ViewMsg, RenderEventBits, 64>
{
  public:
    ViewController();
    ~ViewController();

    void NewView(ViewType type, std::shared_ptr<View> &view, int width, int height);

    int OpenView(View &view);
    int CloseView(View &view);

  private:
    void Cleanup();
    void TerminateConnection();

    void EnumerateLocalInstances();
    void EnumerateRemoteInstances();

};

} // namespace UI
} // namespace EnvGraph

#endif