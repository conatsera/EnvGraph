/* Copyright (c) 2021 Austin Conatser
 * Log system tests
 */
#include <gtest/gtest.h>

#include <filesystem>

#include "log.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace EnvGraph;

class LogTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto logDir = std::filesystem::current_path();
        logDir+="/logs";
        m_logPath = logDir;
        InitLog(true, m_logPath);
        m_logPath+="/engine.log";

        m_logReadStream = std::ifstream(m_logPath);
    }

    void TearDown() override
    {
        DeInitLog();

        m_logReadStream.close();
    }

  protected:
    std::filesystem::path m_logPath;
    std::ifstream m_logReadStream;

};

TEST_F(LogTest, basic)
{
    Log("TEST");
}

TEST_F(LogTest, integers)
{
    int i1 = 1;
    int iMax = INT_MAX;
    int iNegMax = INT_MIN;
    uint32_t ui32 = 128;
    uint32_t ui32Max = UINT32_MAX;
    int64_t i64 = 1;
    int64_t i64_2 = (int64_t)INT32_MAX*2;
    int hex1 = 0xF;
    int hex2 = 0xF0;
    int hex3 = 0xF00F;
    int hex4 = 0x00F;
    int hex5 = 0xABC;

    constexpr const char* logFormatString1 = "%i %u %i %04.4u %u";
    constexpr const char* logFormatString2 = "%li %li";
    constexpr const char* logFormatString3 = "%#04x %i %#03.2x %#04x %#08x %#X";

    char* expectedLogString1 = (char*)malloc(sizeof(char)*100);
    char* expectedLogString2 = (char*)malloc(sizeof(char)*100);
    char* expectedLogString3 = (char*)malloc(sizeof(char)*100);

    Log(logFormatString1, i1, iMax, iNegMax, ui32, ui32Max);
    snprintf(expectedLogString1, 100, logFormatString1, i1, iMax, iNegMax, ui32, ui32Max);

    Log(logFormatString2, i64, i64_2);
    snprintf(expectedLogString2, 100, logFormatString2, i64, i64_2);

    Log(logFormatString3, hex1, hex1, hex2, hex3, hex4, hex5);
    snprintf(expectedLogString3, 100, logFormatString3, hex1, hex1, hex2, hex3, hex4, hex5);

    DeInitLog();
    
    std::string line[3];
    
    std::getline(m_logReadStream, line[0]);
    std::getline(m_logReadStream, line[1]);
    std::getline(m_logReadStream, line[2]);

//  TODO: Confirm how snprintf behaves on linux
    ASSERT_STREQ(line[0].substr(26).c_str(), expectedLogString1);
#ifdef WIN32
    ASSERT_STREQ(line[1].substr(26).c_str(), "1 4294967294");
    ASSERT_STREQ(line[2].substr(26).c_str(), "0x000f 15 0x0f0 0xf00f 0x0000000f 0XABC");
#else
    ASSERT_STREQ(line[1].substr(26).c_str(), expectedLogString2);
    ASSERT_STREQ(line[2].substr(26).c_str(), expectedLogString3);
#endif
    
/*
    Log("%s - %s ", "test", "test2");
    Log("%s - %s a", "test3", "test4");
    Log("test5 - test6");
    Log("%x test", Extent{256, 512});
    Log("%f %.12f %f", 2.123456789012345689, 2.123456789012345689, 2.123456789012345689);
    float f = 4294967281.f;
    double d = 4294967281.f;
    double n = std::nan("dddd");
    double infi = INFINITY;
    Log("%a %A %f %d", f, d, n, infi);*/
}

TEST_F(LogTest, floating_point)
{
    float f = 0.1245678901234567890123456789f;
    double d = 0.1245678901234567890123456789;

    constexpr const char* logFormatString1 = "%f %2f %2.2f %016.8f %8.16f";
    constexpr const char* logFormatString2 = "%f %2f %2.2f %016.8f %8.16f";

    char* expectedLogString1 = (char*)malloc(sizeof(char)*100);
    char* expectedLogString2 = (char*)malloc(sizeof(char)*100);

    Log(logFormatString1, f, f, f, f, f);
    snprintf(expectedLogString1, 100, logFormatString1, f, f, f, f, f);
    
    Log(logFormatString2, d, d, d, d, d);
    snprintf(expectedLogString2, 100, logFormatString2, d, d, d, d, d);

    DeInitLog();
    
    std::string line[2];
    
    std::getline(m_logReadStream, line[0]);
    std::getline(m_logReadStream, line[1]);

    ASSERT_STREQ(line[0].substr(26).c_str(), expectedLogString1);
    ASSERT_STREQ(line[1].substr(26).c_str(), expectedLogString2);
}