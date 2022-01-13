#include <gtest/gtest.h>

#include "ui/view_controller.h"

#include "../base.h"
#include "../view_base.h"

#include "engine.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using EnvGraph::View;
using EnvGraph::ViewController;

TEST_F(ViewTest, create)
{
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