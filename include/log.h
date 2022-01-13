#ifndef ENVGRAPH_LOG_H
#define ENVGRAPH_LOG_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <list>

#include "common.h"

namespace EnvGraph
{
// Stringify functions are provided by operator<< defined by the type

typedef struct _Log_t {
    std::chrono::time_point<std::chrono::system_clock> logTime;
    std::string logEntry;
} Log_t;

static std::ofstream s_logFileStream;
static bool s_stdoutEnabled = false;
static std::list<Log_t> s_logStore;
static bool s_logEnabled = false;
static std::mutex s_logMutex;
static std::thread s_logThread;

static void LogLoop()
{
    std::stringstream logStringStream("");
    while (s_logEnabled || !s_logStore.empty())
    {
         if (!s_logStore.empty())
         {
             Log_t log;
             {
                 std::unique_lock l(s_logMutex);
                 log = s_logStore.back();
                 s_logStore.pop_back();
             }
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(log.logTime.time_since_epoch()).count() % 1000;
            auto time = std::chrono::system_clock::to_time_t(log.logTime);
            auto timestamp = std::localtime(&time);
            logStringStream.str("");
            logStringStream << std::put_time(timestamp, "%F %T") << "." << std::setw(3) << std::setfill('0') << milliseconds << " : " << log.logEntry << std::endl;
            if (s_stdoutEnabled)
                std::cout << logStringStream.str();
            if (s_logFileStream)
                s_logFileStream << logStringStream.str();
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

static void InitLog(bool stdoutEnabled, std::filesystem::path logDirPath = "")
{
    if (!s_logEnabled)
    {
        s_stdoutEnabled = stdoutEnabled;
        if (!logDirPath.empty())
        {
            if (!std::filesystem::exists(logDirPath))
                std::filesystem::create_directory(logDirPath);
            else if(!std::filesystem::is_directory(logDirPath))
                throw std::runtime_error("Log dir exists and is not a directory!");

            std::filesystem::path logFilePath(logDirPath);
            logFilePath+="/engine.log";
            if (std::filesystem::exists(logFilePath))
            {
                auto logFileSuffix = std::chrono::file_clock::now().time_since_epoch().count();
                auto oldLogFilePath(logFilePath);
                oldLogFilePath.concat(".bak" + std::to_string(logFileSuffix));
                std::filesystem::copy_file(logFilePath, oldLogFilePath);
                std::filesystem::resize_file(logFilePath, 0);
            }
            s_logFileStream = std::ofstream(logFilePath);
        }
        s_logStore = std::list<Log_t>();
        s_logEnabled = true;
        s_logThread = std::thread(&LogLoop);
    }
}

static void DeInitLog()
{
    if (s_logEnabled)
    {
        s_logEnabled = false;
        s_logThread.join();
        s_logFileStream.flush();
        s_logFileStream.close();
    }
}

static void Log(const char* format, std::stringstream& logEntry)
{
    while (format && *format)
    {
        // There are no args left, so if it's not an escaped % there's a problem
        if (*format == '%' && *++format!='%')
            throw std::runtime_error("Not enough args provided to Log function");
        else
            logEntry << *format++;
    }
    auto logTime = std::chrono::system_clock::now();
    {
        std::unique_lock l(s_logMutex);
        s_logStore.push_front(Log_t{logTime, logEntry.str()});
    }
}

template<typename T, typename... Args>
static void Log(const char* format, std::stringstream& logEntry, T value, Args... args)
{
    while (format && *format)
    {
        // If it's a format string, but not an escaped %, call Log again
        if (*format == '%' && *++format!='%') {
            std::string numSpecs("");
            // Process special format specifications
            while (*format == '#' || (*format >= '0' && *format <= '9'))
                numSpecs += (*format++);
            std::streamsize lastWidth = 0;
            std::streamsize lastPrec = 0;
            if (numSpecs[0] == '#')
            {
                logEntry << std::showbase;
                numSpecs = numSpecs.substr(1);
            }
            if (numSpecs.compare("") != 0)
            {
                if (numSpecs[0] == '0')
                {
                    logEntry.fill('0');
                    numSpecs = numSpecs.substr(1);
                }
                auto numWidth = std::stoi(numSpecs);
                lastWidth = logEntry.width(numWidth);
                logEntry << std::internal;
            }
            if (*format == '.')
            {
                numSpecs = "";
                format++;
                while (*format > '0' && *format < '9')
                    numSpecs += (*format++);
                if (numSpecs.compare("") != 0)
                {
                    auto numPrec = std::stoi(numSpecs);
                    lastPrec = logEntry.precision(numPrec);
                }
            }
            switch  (*format)
            {
                case 'X':
                    logEntry << std::uppercase;
                case 'x':
                    logEntry.width(logEntry.width()+2);
                    logEntry << std::hex << value << std::dec << std::nouppercase;
                    break;
                case 'E':
                    logEntry << std::uppercase;
                case 'e':
                    logEntry << std::scientific << value << std::defaultfloat << std::nouppercase;
                    break;
                case 'A':
                    logEntry << std::uppercase;
                case 'a':
                    logEntry << std::hexfloat << value << std::defaultfloat << std::nouppercase;
                    break;
                default:
                    logEntry << value;
            }
            if (lastWidth)
                logEntry.width(lastWidth);
            if (lastPrec)
                logEntry.precision(lastPrec);
            // Cleanup excess chars in a format specifier. Unlike printf, type specification is ignored and deduced by template to the operator<<
            while (format[0] != '\0' && format[1] != ' ' && format[1] != '\0')
                format++;
            return Log(++format, logEntry, args...);
        }
        else
            logEntry << *format++;
    }
    throw std::runtime_error("Too many args provided to Log");
}

static void Log(const char* format)
{
    std::stringstream logEntry("");
    Log(format, logEntry);
}

template<typename T, typename... Args>
static void Log(const char* format, T value, Args... args)
{
    std::stringstream logEntry("");
    Log(format, logEntry, value, args...);
}

} // namespace EnvGraph

#endif