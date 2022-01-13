#include "ui/view_controller.h"

namespace EnvGraph
{
namespace UI
{

ViewController::ViewController()
{
}

ViewController::~ViewController()
{
}

void ViewController::NewView(ViewType type, std::shared_ptr<View> &view, int width, int height)
{
    if (type == ViewType::LOCAL)
    {
        view->Setup();
    }
}

int ViewController::OpenView(View &view)
{
    return 0;
}

int ViewController::CloseView(View &view)
{
    view.~View();
    return 0;
}

} // namespace UI
} // namespace EnvGraph
