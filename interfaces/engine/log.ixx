module;
/*
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <filesystem>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
*/
export module log;

import std.compat;

namespace EnvGraph {

typedef struct _Log_t {
	std::chrono::time_point<std::chrono::system_clock> logTime;
	std::function<std::string()> logEntryFn;
} Log_t;

export class Logger
{
  public:
    Logger() {};
	~Logger() {
		m_logThread.request_stop();
		m_logThread.join();
		if (m_logFileStream)
		{
			m_logFileStream.flush();
			m_logFileStream.close();
		}
	};

	bool Init(bool stdOutEnabled = false, std::filesystem::path logPath = "") {
		m_stdOutEnabled = stdOutEnabled;
        if (!logPath.empty())
		{
			if (!std::filesystem::exists(logPath.parent_path()))
				std::filesystem::create_directory(logPath.parent_path());
			if (!std::filesystem::exists(logPath))
				std::filesystem::create_directory(logPath);

			std::stringstream logFileName("");
			const auto now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            const auto now_tm = *std::localtime(&now_time);

			char timeString[std::size("yyyy-mm-dd_hh-mm-ss")];
            std::strftime(timeString, std::size(timeString), "%F_%H-%M-%S", &now_tm);

			logFileName << timeString << ".log";

			const auto logFilePath = logPath.append(logFileName.str());
			m_logFilePath = logFilePath;

			m_logFileStream.open(logFilePath);

		}
		m_logQueue = std::queue<Log_t>();
		m_logThread = std::jthread([&](std::stop_token logStop) {
			while (!logStop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
            std::stringstream logSS("");
			int sleepExp = 0;
			while (!logStop.stop_requested() || !m_logQueue.empty()) {
				if (!m_logQueue.empty()) {
					sleepExp = 0;
					Log_t log;
					{
						std::unique_lock l(m_logQueueMutex);
						log = m_logQueue.front();
						m_logQueue.pop();
					}

					const auto milliseconds = log.logTime.time_since_epoch().count() % 1000;
					const auto time = std::chrono::system_clock::to_time_t(log.logTime);
					const auto _tm = *std::localtime(&time);

					char timeString[std::size("yyyy-mm-dd hh:mm:ss")];
                    std::strftime(timeString, std::size(timeString), "%F %T", &_tm);

					logSS.str("");
                    logSS << timeString << "." << milliseconds << " : " << log.logEntryFn() << std::endl;
					if (m_stdOutEnabled)
						std::cout << logSS.str();
					if (m_logFileStream.is_open())
						m_logFileStream << logSS.str();
				}
				else {
					// Simple exponential backoff brings log polling rate to approx 1000Hz minimum
					if (sleepExp != 5)
						sleepExp++;
					std::this_thread::sleep_for(std::chrono::microseconds(32 << sleepExp));
				}
			}
			});
		return true;
	}

	bool AddFile(std::filesystem::path log_file_path);
	bool AddSocket();

	void Log(const char* entry) {
		auto logTime = std::chrono::system_clock::now();
		m_logQueue.push(Log_t{ logTime,[entry]() -> std::string {return LogInt(entry); } });
	}

	template<typename T, typename ... Args>
	void Log(const char* format, T value, Args... args) {
		auto logTime = std::chrono::system_clock::now();
		m_logQueue.push(Log_t{ logTime,[format, value, args...]() -> std::string {return LogInt(format, value, args...); } });
	}

	const std::filesystem::path GetLogFilePath() const {
		return m_logFilePath;
	}

private:
	std::string WriteHeader() const;

	static std::string LogInt(const char* format, std::stringstream& logEntry) {
		while (format && *format)
		{
			// There are no args left, so if it's not an escaped % there's a problem
			if (*format == '%' && *++format != '%')
				throw std::runtime_error("Not enough args provided to Log function");
			else
				logEntry << *format++;
		}

		return logEntry.str();
	}

	static std::string LogInt(const char* entry) {
		std::stringstream s("");
		return LogInt(entry, s);
	}

	template<typename T, typename ... Args>
	static std::string LogInt(const char* format, std::stringstream& logEntry, T value, Args... args) {
		while (format && *format)
		{
			// If it's a format string, but not an escaped %, call Log again
			if (*format == '%' && *++format != '%') {
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
				switch (*format)
				{
				case 'X':
					logEntry << std::uppercase;
				case 'x':
					logEntry.width(logEntry.width() + 2);
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

	template<typename T, typename ... Args>
	static std::string LogInt(const char* format, T value, Args... args) {
		std::stringstream logEntry("");
		return LogInt(format, logEntry, value, args...);
	}

private:
	std::filesystem::path m_logFilePath{""};

	bool              m_stdOutEnabled = false;
	std::ofstream     m_logFileStream = std::ofstream();
	std::queue<Log_t> m_logQueue;

	std::mutex  m_logQueueMutex;
	std::jthread m_logThread;
};

} // namespace EnvGraph