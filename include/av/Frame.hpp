#pragma once

#include <memory>
#include "Util.hpp"
#include "Error.hpp"

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

		void get_buffer()
		{
			if (const auto rc = av_frame_get_buffer(get(), 0); rc < 0)
				throw Error("av_frame_get_buffer", rc);
		}
	};
}