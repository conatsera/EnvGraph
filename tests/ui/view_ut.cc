#include <gtest/gtest.h>

#include "ui/view_controller.h"

#include "engine.h"
#include "../base.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using EnvGraph::View;
using EnvGraph::ViewController;

class ViewTest : public EnvGraph::TestBase {
protected:
    void SetUp() override {
        EnvGraph::TestBase::SetUp();

        vc.NewView(EnvGraph::ViewType::LOCAL_LINUX, view);

#ifdef _WIN32
        view.SetWin32(m_window);
#else
        view.SetXCB(m_xConnection);
#endif

        m_aEngine->StartRender();
    };

    void TearDown() override {
        ASSERT_EQ(vc.CloseView(view), 0);
        EnvGraph::TestBase::TearDown();

    };

private:
    ViewController vc;
    View view;
};

TEST_F(ViewTest, create) {
    ASSERT_EQ(true, false);
}

/*
TEST_F(ViewTest, destroy) {
    ASSERT_EQ(true, false);
}

TEST_F(ViewTest, input) {
    ASSERT_EQ(true, false);
}

TEST_F(ViewTest, attach) {
    ASSERT_EQ(true, false);
}*/