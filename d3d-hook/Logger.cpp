﻿// PHZ
// 2018-5-15

#if defined(WIN32) || defined(_WIN32) 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "Logger.h"
#include <stdarg.h>
#include <iostream>
#include "Timestamp.h"

const char* Priority_To_String[] =
{
    "DEBUG",
    "CONFIG",
    "INFO",
    "WARNING",
    "ERROR"
};

Logger::Logger() 
{
    
}

Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

Logger::~Logger()
{

}

void Logger::init(char *pathname)
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (pathname != nullptr)
	{
		_ofs.open(pathname);
		if (_ofs.fail())
		{
			std::cerr << "Failed to open logfile." << std::endl;
		}
	}
}

void Logger::exit()
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (_ofs.is_open())
	{
		_ofs.close();
	}
}

void Logger::log(Priority priority, const char* __file, const char* __func, int __line, const char *fmt, ...)
{	
	std::unique_lock<std::mutex> lock(_mutex);

    char buf[2048] = {0};
    sprintf(buf, "[%s][%s:%s:%d] ", Priority_To_String[priority],  __file, __func, __line);
    va_list args;
    va_start(args, fmt);
    vsprintf(buf + strlen(buf), fmt, args);
    va_end(args);

	write(std::string(buf));
}

void Logger::log2(Priority priority, const char *fmt, ...)
{
	std::unique_lock<std::mutex> lock(_mutex);

	char buf[4096] = { 0 };
	sprintf(buf, "[%s] ", Priority_To_String[priority]);  
	va_list args;
	va_start(args, fmt);
	vsprintf(buf + strlen(buf), fmt, args);
	va_end(args);

	write(std::string(buf));
}

void Logger::write(std::string info)
{
	if (_ofs.is_open())
	{
		_ofs << "[" << Timestamp::localtime() << "]"
			<< info << std::endl;
	}
   
    std::cout << "[" << Timestamp::localtime() << "]"  
			<< info << std::endl;
}
