#pragma once

extern "C"
{
#include <libavfilter/buffersink.h>
}

#include <av/FilterContext.hpp>

namespace av
{

struct BufferSink : FilterContext
{
	BufferSink()
		: FilterContext{}
	{
	}

	BufferSink(AVFilterContext *c)
		: FilterContext{c}
	{
		validate_is_buffersink(c);
	}

	void get_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersink_get_frame(ctx, frame); rc < 0)
			throw Error("av_buffersink_get_frame", rc);
	}

	void operator>>(AVFrame *const frame) { get_frame(frame); }

private:
	static void validate_is_buffersink(const AVFilterContext *const c)
	{
		static const auto buffer = avfilter_get_by_name("buffersink"),
						  abuffer = avfilter_get_by_name("abuffersink");
		if (c->filter != buffer && c->filter != abuffer)
			throw std::invalid_argument{"this is not a buffersrc!"};
	}
};

} // namespace av
