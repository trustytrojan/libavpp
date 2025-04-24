#pragma once

extern "C"
{
#include <libavutil/hwcontext.h>
}

#include "Error.hpp"

namespace av
{

/**
 * Holds a reference to an AVHWDeviceContext.
 */
class HWDeviceContext
{
	AVBufferRef *_ctx{};

public:
	HWDeviceContext(
		const AVHWDeviceType type,
		const char *const device = nullptr,
		AVDictionary *const opts = nullptr,
		const int flags = 0)
	{
		if (const auto rc =
				av_hwdevice_ctx_create(&_ctx, type, device, opts, flags);
			rc < 0)
			throw Error("av_hwdevice_ctx_create", rc);
	}

	~HWDeviceContext() { av_buffer_unref(&_ctx); }

	AVBufferRef *get_ref() const { return _ctx; }
};

} // namespace av
