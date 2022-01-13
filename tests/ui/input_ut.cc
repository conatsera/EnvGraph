#include <gtest/gtest.h>

#include "../base.h"

#include <ui/input.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class InputTests : public EnvGraph::TestBase
{
    virtual void SetUp() override
    {
        EnvGraph::TestBase::SetUp();
        m_inputManager.NewStatusSub([this](UI::StatusMessage statusMessage) {

        });
    }

protected:
    UI::InputManager m_inputManager;
};

TEST_F(InputTests, enumerate)
{
    m_inputManager.NewStatusSub([](UI::StatusMessage e) { std::cout << e.deviceCount << std::endl; });
    m_inputManager.EnumerateDevices();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
TEST_F(InputTests, listenCharacters)
{

}

TEST_F(InputTests, listenMouse)
{
}

TEST_F(InputTests, listenWMRMC)
{
    glm::vec3 pos{glm::zero<glm::vec3>()};
    glm::quat rot{glm::identity<glm::quat>()};

    // TODO: Listen for WMR controllers

    ASSERT_TRUE(glm::all(glm::notEqual(pos, glm::zero<glm::vec3>())));
    ASSERT_TRUE(glm::all(glm::notEqual(rot, glm::identity<glm::quat>())));
}