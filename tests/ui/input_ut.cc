#include <gtest/gtest.h>

#include "../base.h"

#include <ui/input.h>

#ifdef WIN32
#include <ShellScalingApi.h>
#endif

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class InputTests : public EnvGraph::TestBase
{
  public:
    void SetUp() override
    {
        EnvGraph::TestBase::SetUp();
    }
  protected:
    bool SimulateClick(Location2D loc)
    {
        bool success = true;
#ifdef WIN32
        auto monitor = MonitorFromWindow(m_view->m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo;
        ZeroMemory(&monitorInfo, sizeof(MONITORINFO));
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoW(monitor, &monitorInfo);

        INPUT mouseTest[3];
        ZeroMemory(&mouseTest, sizeof(INPUT) * 3);

        POINT pt{static_cast<long>(loc.x), static_cast<long>(loc.y)};
        m_view->ClientToScreen(&pt);

        auto screenX = static_cast<long double>(pt.x);
        auto screenY = static_cast<long double>(pt.y);

        auto screenW = static_cast<long double>(monitorInfo.rcMonitor.right);
        auto screenH = static_cast<long double>(monitorInfo.rcMonitor.bottom);

        auto screenXNorm = screenX / screenW;
        auto screenYNorm = screenY / screenH;

        auto screenXVirt = screenXNorm * 65535.0;
        auto screenYVirt = screenYNorm * 65535.0;

        auto x = static_cast<long>(std::round(screenXVirt));
        auto y = static_cast<long>(std::round(screenYVirt));

        mouseTest[0].type = INPUT_MOUSE;
        mouseTest[0].mi.dx = x;
        mouseTest[0].mi.dy = y;
        mouseTest[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

        mouseTest[1].type = INPUT_MOUSE;
        mouseTest[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        mouseTest[2].type = INPUT_MOUSE;
        mouseTest[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        if (1 != SendInput(1, &mouseTest[0], sizeof(INPUT)))
        {
            success = false;
            std::cout << "SendInput error: " << GetLastError() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(125));

        if (1 != SendInput(1, &mouseTest[1], sizeof(INPUT)))
        {
            success = false;
            std::cout << "SendInput error: " << GetLastError() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(125));

        if (1 != SendInput(1, &mouseTest[2], sizeof(INPUT)))
        {
            success = false;
            std::cout << "SendInput error: " << GetLastError() << std::endl;
        }
#endif
        return success;
    }
};

TEST_F(InputTests, enumerate)
{
    auto sub = m_inputManager->NewStatusSub([](UI::StatusMessage e) { std::cout << e.deviceCount << std::endl; });
    m_inputManager->EnumerateDevices();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sub->End();
}
TEST_F(InputTests, listenCharacters)
{

}

TEST_F(InputTests, listenMouse)
{
    auto clickCount = 0;
    auto sub = m_inputManager->NewInputSub([&clickCount](UI::InputMessage e) {
        std::cout << " ( " << e.screenPointer.x << ", " << e.screenPointer.y << " ), ( " << e.windowPointer.x << ", "
                    << e.windowPointer.y << " )" << std::endl;
        switch (clickCount)
        {
        case 0:
            ASSERT_EQ(e.windowPointer.x, 128);
            ASSERT_EQ(e.windowPointer.y, 128);
            break;
        case 1:
            ASSERT_EQ(e.windowPointer.x, 128);
            ASSERT_EQ(e.windowPointer.y, 256);
            break;
        case 2:
            ASSERT_EQ(e.windowPointer.x, 128);
            ASSERT_EQ(e.windowPointer.y, 512);
            break;
        case 3:
            ASSERT_EQ(e.windowPointer.x, 128);
            ASSERT_EQ(e.windowPointer.y, 768);
            break;
        case 4:
            ASSERT_EQ(e.windowPointer.x, 256);
            ASSERT_EQ(e.windowPointer.y, 768);
            break;
        case 5:
            ASSERT_EQ(e.windowPointer.x, 512);
            ASSERT_EQ(e.windowPointer.y, 768);
            break;
        case 6:
            ASSERT_EQ(e.windowPointer.x, 768);
            ASSERT_EQ(e.windowPointer.y, 768);
            break;
        }
        clickCount++;
    });
    auto mouseThread = std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        SimulateClick({128, 128});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({128, 256});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({128, 512});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({128, 768});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({256, 768});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({512, 768});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        SimulateClick({768, 768});
    });

    m_view->Run();

    mouseThread.join();

    ASSERT_EQ(clickCount, 7);

    sub->End();
}

TEST_F(InputTests, listenWMRMC)
{
    glm::vec3 pos{glm::zero<glm::vec3>()};
    glm::quat rot{glm::identity<glm::quat>()};

    // TODO: Listen for WMR controllers

    ASSERT_TRUE(glm::all(glm::notEqual(pos, glm::zero<glm::vec3>())));
    ASSERT_TRUE(glm::all(glm::notEqual(rot, glm::identity<glm::quat>())));
}