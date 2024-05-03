#pragma once

#include <memory>

extern "C"
{
#include <libavutil/frame.h>
}

namespace av
{
	void frame_free(AVFrame *_f)
	{
		av_frame_free(&_f);
	}

	struct Frame : std::unique_ptr<AVFrame, decltype(&frame_free)>
	{
		Frame(AVFrame *const _f) : std::unique_ptr<AVFrame, decltype(&frame_free)>(_f, frame_free) {}

		Frame() : Frame(av_frame_alloc())
		{
			if (!get())
				throw std::runtime_error("av_frame_alloc() failed");
		}
	};
}