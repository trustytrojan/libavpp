#pragma once

#include "Error.hpp"

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}

namespace av
{

/**
 * Non-owning wrapper over an `AVFrame *`.
 * Eases usage of `av::Scaler`, `av::Resampler`, and `av_image_*` functions.
 */
class Frame
{
protected:
	AVFrame *_f{};

public:
	Frame(AVFrame *const f)
		: _f{f}
	{
	}

	// Provide access to the underlying pointer
	AVFrame *get() const { return _f; }
	AVFrame *operator->() const { return _f; }
	operator AVFrame *() const { return _f; }

	void get_buffer(int align = 0)
	{
		if (const auto rc = av_frame_get_buffer(_f, align); rc < 0)
			throw Error("av_frame_get_buffer", rc);
	}

	int image_get_buffer_size(int align = 1) const
	{
		const auto size = av_image_get_buffer_size(
			(AVPixelFormat)_f->format, _f->width, _f->height, align);
		if (size < 0)
			throw Error("av_image_get_buffer_size", size);
		return size;
	}

	void
	image_copy_to_buffer(uint8_t *dst, const int dst_size, int align = 1) const
	{
		if (const auto rc = av_image_copy_to_buffer(
				dst,
				dst_size,
				_f->data,
				_f->linesize,
				(AVPixelFormat)_f->format,
				_f->width,
				_f->height,
				align);
			rc < 0)
		{
			throw Error("av_image_copy_to_buffer", rc);
		}
	}

	void unref() { av_frame_unref(_f); }
};

/**
 * Extension of Frame that owns its underlying AVFrame pointer.
 * Handles allocation and deallocation of the AVFrame.
 */
class OwnedFrame : public Frame
{
public:
	OwnedFrame()
		: Frame{av_frame_alloc()}
	{
		if (!_f)
			throw Error("av_frame_alloc", AVERROR(ENOMEM));
	}

	~OwnedFrame() { av_frame_free(&_f); }
};

} // namespace av
