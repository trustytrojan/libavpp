#pragma once

#include <stdexcept>

extern "C"
{
#include <libavutil/error.h>
}

namespace av
{

struct Error : std::runtime_error
{
	const char *const func;
	const int errnum;

	Error(const char *const func, const int errnum)
		: std::runtime_error{std::string{func} + ": " + av_err2str(errnum)},
		  func{func},
		  errnum{errnum}
	{
	}
};

} // namespace av
