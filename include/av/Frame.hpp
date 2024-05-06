#pragma once

#include <memory>
#include "Util.hpp"

extern "C"
{
#include <libavutil/frame.h>
}

namespace av
{
	// Extension of `std::unique_ptr<AVFrame, ...>`.
	// Eases usage of `av::Scaler` and `av::Resampler`.
	struct Frame : std::unique_ptr<AVFrame, decltype(&frame_free)>
	{
		Frame(AVFrame *const _f)
			: std::unique_ptr<AVFrame, decltype(&frame_free)>(_f, frame_free) {}

		Frame() : Frame(av_frame_alloc())
		{
			if (!get())
				throw std::runtime_error("av_frame_alloc() failed");
		}

		Frame(Frame &&) = delete;
		Frame &operator=(Frame &&) = delete;
	};
}