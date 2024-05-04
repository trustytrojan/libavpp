#pragma once

#include "Error.hpp"
#include "Frame.hpp"

extern "C"
{
#include <libswscale/swscale.h>
}

namespace av
{
	class Scaler
	{
		SwsContext *_ctx = nullptr;

	public:
		Scaler(
			int src_width,
			int src_height,
			AVPixelFormat src_format,
			int dst_width,
			int dst_height,
			AVPixelFormat dst_format,
			int flags = 0,
			SwsFilter *src_filter = NULL,
			SwsFilter *dst_filter = NULL,
			const double *param = NULL)
			: _ctx(sws_getContext(
				  src_width, src_height, src_format,
				  dst_width, dst_height, dst_format,
				  flags, src_filter, dst_filter, param))
		{
			if (!_ctx)
				throw std::runtime_error("sws_getContext() failed");
		}

		~Scaler()
		{
			sws_freeContext(_ctx);
		}

		void scale_frame(AVFrame *const dst, const AVFrame *const src)
		{
			if (const auto rc = sws_scale_frame(_ctx, dst, src); rc < 0)
				throw Error("sws_scale_frame", rc);
		}
	};
}
