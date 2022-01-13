#include "ui/view.h"

namespace EnvGraph
{
namespace UI
{

void View::Setup()
{
#ifdef WIN32
    Initialize(Extent{1024, 1024}, L"EnvGraph");
#endif
}

void View::Pause()
{
}

void View::Debug()
{
}

} // namespace UI
} // namespace EnvGraph
