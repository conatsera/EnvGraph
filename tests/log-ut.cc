import log;

#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

namespace {
TEST(LogTest, InitConsoleLog) {
	auto logger = EnvGraph::Logger("", "");

	if (logger.Init(true))
		logger.Log("test");
	else
		FAIL();
}

TEST(LogTest, InitFileLog) {
	auto logger = EnvGraph::Logger("", "");
	auto current_path = std::filesystem::current_path().append("logs");

	EXPECT_EQ(logger.Init(false, current_path), true);
}

TEST(LogTest, TestFileLog) {
	std::filesystem::path logFilePath;
	{
		auto logger = EnvGraph::Logger("", "");
		auto current_path = std::filesystem::current_path().append("logs");

		EXPECT_EQ(logger.Init(false, current_path), true);

		logger.Log("test file log");
		logFilePath = logger.GetLogFilePath();
	}

	std::string logFileContents;
	std::getline(std::ifstream(logFilePath), logFileContents);
	
	EXPECT_EQ(true, logFileContents.ends_with("test file log"));
}
}