#include "Logger.h"

//On MinGW32 spdlog will fail to compile due to lacking the c++ thread model, so don't use it on that compiler and stub the function below
#if !defined(NETX_MINGW32)
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include "../common/misc.h"

#include <iostream>
#include <memory>
#include <string>

namespace NXE
{
namespace Utils
{
namespace Logger
{
	#if defined(NETX_MINGW32)
		void init(std::string filename) {

		}
	#else
		static const char *LOG_PATTERN = "%^[%H:%M:%S.%e] [%l] [%@ %!]: %v%$";
		std::vector<spdlog::sink_ptr> sinks;

  		void init(std::string filename) {
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(widen(filename), true));

			spdlog::set_default_logger(std::make_shared<spdlog::logger>("nxe logger", begin(sinks), end(sinks)));

			spdlog::set_pattern(LOG_PATTERN);

			spdlog::set_error_handler([](const std::string &msg) {
			std::cerr << "spdlog error: " << msg << std::endl;
			});

			spdlog::flush_on(spdlog::level::debug);
			#if defined(DEBUG)
				spdlog::set_level(spdlog::level::trace);
			#else
				spdlog::set_level(spdlog::level::info);
			#endif
		}
	#endif


} // namespace Logger
} // namespace Utils
} // namespace NXE
