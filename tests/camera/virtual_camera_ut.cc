#include <gtest/gtest.h>

#include <memory>

#include "virtual_camera.h"

class VirtualCameraFixture {


private:
    std::unique_ptr<VirtualCamera> camera_;


}

TEST_F(VirtualCameraFixture, "")