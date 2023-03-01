import engine;

#include "gtest/gtest.h"
#include <filesystem>

namespace {
	TEST(EngineTest, InitEngine) {
		auto engine = EnvGraph::Engine::MainEngine();

		EXPECT_EQ(engine.Init(), true);
	}
}