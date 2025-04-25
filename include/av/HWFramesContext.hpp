#pragma once

extern "C"
{
#include <libavutil/hwcontext.h>
}

#include "Error.hpp"
#include "Frame.hpp"
#include "HWDeviceContext.hpp"

namespace av
{

class HWFramesContext
{
	AVBufferRef *_ctx{};

public:
	HWFramesContext(
		AVBufferRef *const devctx,
		const AVPixelFormat format,
		const AVPixelFormat sw_format,
		const int width,
		const int height,
		const int initial_pool_size = 20)
	{
		if (!(_ctx = av_hwframe_ctx_alloc(devctx)))
			throw Error("av_hwframe_ctx_alloc", AVERROR(ENOMEM));

		const auto hwfctx = reinterpret_cast<AVHWFramesContext *>(_ctx->data);
		hwfctx->format = format;
		hwfctx->sw_format = sw_format;
		hwfctx->width = width;
		hwfctx->height = height;
		hwfctx->initial_pool_size = initial_pool_size;

		if (const auto rc = av_hwframe_ctx_init(_ctx); rc < 0)
			throw Error("av_hwframe_ctx_init", rc);
	}

	HWFramesContext(
		const HWDeviceContext &devctx,
		const AVPixelFormat format,
		const AVPixelFormat sw_format,
		const int width,
		const int height,
		const int initial_pool_size = 20)
		: HWFramesContext{
			  devctx.get_ref(),
			  format,
			  sw_format,
			  width,
			  height,
			  initial_pool_size}
	{
	}

	~HWFramesContext() { av_buffer_unref(&_ctx); }

	AVBufferRef *get_ref() const { return _ctx; }

	/**
	 * Similar to `Frame::get_buffer`, gets a hardware-accelerated (aka on the
	 * GPU) buffer on `frame`.
	 * @param frame The frame to allocate a hardware buffer for
	 * @throws `av::Error` if `av_hwframe_get_buffer` fails
	 */
	void get_buffer(const Frame &frame, const int flags = 0)
	{
		if (const auto rc = av_hwframe_get_buffer(_ctx, frame.get(), flags);
			rc < 0)
			throw Error("av_hwframe_get_buffer", rc);
		if (!frame->hw_frames_ctx)
			throw Error("av_hwframe_get_buffer", AVERROR(ENOMEM));
	}

	/**
	 *
	 * @param dst the destination frame. dst is not touched on failure.
	 * @param src the source frame.
	 * @param flags currently unused, should be set to zero
	 * @throws `av::Error` if `av_hwframe_transfer_data` fails
	 */
	static void
	transfer_data(const Frame &dst, const Frame &src, const int flags = 0)
	{
		if (const auto rc =
				av_hwframe_transfer_data(dst.get(), src.get(), flags);
			rc < 0)
			throw Error("av_hwframe_transfer_data", rc);
	}
};

} // namespace av
