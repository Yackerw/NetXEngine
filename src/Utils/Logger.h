#ifndef _LOGGER_H

//On MinGW32 spdlog will fail to compile due to lacking the c++ thread model, so don't use it on that compiler and remove the functions using empty defines below
#if !defined(NETX_MINGW32)
	#include <spdlog/spdlog.h>
#endif

#include <string>

namespace NXE
{
namespace Utils
{

namespace Logger
{

  void init(std::string filename);

#if defined(NETX_MINGW32)
	#define LOG_TRACE 
	#define LOG_DEBUG 
	#define LOG_INFO 
	#define LOG_WARN 
	#define LOG_ERROR 
	#define LOG_CRITICAL 
#else
	#define LOG_TRACE SPDLOG_TRACE
	#define LOG_DEBUG SPDLOG_DEBUG
	#define LOG_INFO SPDLOG_INFO
	#define LOG_WARN SPDLOG_WARN
	#define LOG_ERROR SPDLOG_ERROR
	#define LOG_CRITICAL SPDLOG_CRITICAL
#endif

} // namespace Logger
} // namespace Utils
} // namespace NXE

#endif //_LOGGER_H