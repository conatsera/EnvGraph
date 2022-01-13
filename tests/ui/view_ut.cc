#include <gtest/gtest.h>

#include "ui/view_controller.h"

#include "../base.h"
#include "engine.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class ViewTest : public EnvGraph::TestBase
{
  protected:
    void SetUp() override
    {
        EnvGraph::TestBase::SetUp();

        m_view = std::make_shared<UI::View>();

        m_view->SetTitle("Test");

        m_viewController->NewView(EnvGraph::UI::ViewType::LOCAL, m_view, kDefaultWidth, kDefaultHeight);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    };
};

TEST_F(ViewTest, createAndDestroy)
{
    m_view->Run();
    //ASSERT_STREQ(SDL_GetError(), "");
}

TEST_F(ViewTest, viewEvents)
{
    int resizeEventTriggered = 0;
    int shownEventTriggered = 0;
    int hiddenEventTriggered = 0;

    auto resizeSub =
        m_view->CreateNewSub(RenderEventBits::RESIZE, [&resizeEventTriggered](EnvGraph::UI::ViewMsg e) {
            resizeEventTriggered++;

            ASSERT_EQ(e.m_eventType, EnvGraph::Events::RENDER);
            ASSERT_EQ(e.m_eventSubType, RenderEventBits::RESIZE);

            if (resizeEventTriggered == 1)
            {
                ASSERT_EQ(e.m_newExtent.x, 1920);
                ASSERT_EQ(e.m_newExtent.y, 1080);
            }
            ASSERT_NE(e.m_newExtent.x, 0);
            ASSERT_NE(e.m_newExtent.y, 0);
            std::cout << "X: " << e.m_newExtent.x << ", Y: " << e.m_newExtent.y << std::endl;
        });

    auto visSub =
        m_view->CreateNewSub(RenderEventBits::VISIBILITY,
                            [&shownEventTriggered, &hiddenEventTriggered](EnvGraph::UI::ViewMsg e) {
                                ASSERT_EQ(e.m_eventType, EnvGraph::Events::RENDER);
                                if (e.m_hidden)
                                    shownEventTriggered++;
                                else
                                    hiddenEventTriggered++;
                                std::cout << "Hidden: " << e.m_hidden << std::endl;
                            });

    // m_view.Minimize();
    m_view->ResizeWindow(1920, 1080);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(resizeEventTriggered, 1);
    
    m_view->Run();

    ASSERT_GT(resizeEventTriggered, 1);
}

TEST_F(ViewTest, attach)
{
    ASSERT_EQ(true, false);
}