#pragma once

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "Error.hpp"

namespace av
{

/**
 * Non-owning wrapper of `AVFilterContext` with convenience methods.
 */
class FilterContext
{
	AVFilterContext *const ctx = nullptr;

public:
	FilterContext(AVFilterContext *const ctx)
		: ctx(ctx)
	{
	}

	AVFilterContext *get() { return ctx; }
	AVFilterContext *operator->() { return ctx; }

	void send_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersrc_add_frame(ctx, frame); rc < 0)
			throw Error("av_buffersrc_add_frame", rc);
	}

	void receive_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersink_get_frame(ctx, frame); rc < 0)
			throw Error("av_buffersink_get_frame", rc);
	}
};

} // namespace av
