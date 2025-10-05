#pragma once

#include "Error.hpp"

extern "C"
{
#include <libavutil/frame.h>
}

namespace av
{

/**
 * Wrapper over an `AVFrame *`.
 * Eases usage of `av::Scaler` and `av::Resampler`.
 */
class Frame
{
	AVFrame *_frame{};

public:
	Frame()
	{
		if (!(_frame = av_frame_alloc()))
			throw Error("av_frame_alloc", AVERROR(ENOMEM));
	}

	~Frame() { av_frame_free(&_frame); }

	Frame(const Frame &) = delete;
	Frame &operator=(const Frame &) = delete;

	Frame(Frame &&other)
	{
		_frame = other._frame;
		other._frame = {};
	}

	Frame &operator=(Frame &&other)
	{
		if (this != &other)
		{
			av_frame_free(&_frame);
			_frame = other._frame;
			other._frame = {};
		}
		return *this;
	}

	// Provide access to the underlying pointer
	AVFrame *get() const { return _frame; }
	AVFrame *operator->() const { return _frame; }

	void get_buffer(int align = 0)
	{
		if (const auto rc = av_frame_get_buffer(get(), align); rc < 0)
			throw Error("av_frame_get_buffer", rc);
	}
};

} // namespace av
