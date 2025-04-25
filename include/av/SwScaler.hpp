#pragma once

#include "Error.hpp"
#include <cstdint>

extern "C"
{
#include <libswscale/swscale.h>
}

namespace av
{

class SwScaler
{
public:
	struct SrcDstArgs
	{
		uint32_t width, height;
		AVPixelFormat format;
		SwsFilter *filter = nullptr;
	};

private:
	SwsContext *const _ctx;

public:
	SwScaler(
		const SrcDstArgs &src,
		const SrcDstArgs &dst,
		int flags = 0,
		const double *param = NULL)
		: _ctx(sws_getContext(
			  static_cast<int>(src.width),
			  static_cast<int>(src.height),
			  src.format,
			  static_cast<int>(dst.width),
			  static_cast<int>(dst.height),
			  dst.format,
			  flags,
			  src.filter,
			  dst.filter,
			  param))
	{
		if (!_ctx)
			throw Error("sws_getContext", AVERROR(ENOMEM));
	}

	~SwScaler() { sws_freeContext(_ctx); }

	void scale_frame(AVFrame *const dst, const AVFrame *const src)
	{
		if (const auto rc = sws_scale_frame(_ctx, dst, src); rc < 0)
			throw Error("sws_scale_frame", rc);
	}
};

} // namespace av
