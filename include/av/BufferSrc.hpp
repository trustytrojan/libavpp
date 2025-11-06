#pragma once

extern "C"
{
#include <libavfilter/buffersrc.h>
}

#include <av/FilterContext.hpp>

namespace av
{

struct BufferSrc : FilterContext
{
	class Parameters
	{
		AVBufferSrcParameters *const p;

	public:
		Parameters()
			: p{av_buffersrc_parameters_alloc()}
		{
			if (!p)
				throw Error("av_buffersrc_parameters_alloc", AVERROR(ENOMEM));
		}

		~Parameters() { av_free(p); }
		AVBufferSrcParameters *operator->() { return p; }
		operator AVBufferSrcParameters *() { return p; }
	};

	BufferSrc()
		: FilterContext{}
	{
	}

	BufferSrc(AVFilterContext *c)
		: FilterContext{c}
	{
		validate_is_buffersrc(c);
	}

	BufferSrc &operator=(AVFilterContext *c)
	{
		validate_is_buffersrc(c);
		ctx = c;
		return *this;
	}

	void add_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersrc_add_frame(ctx, frame); rc < 0)
			throw Error("av_buffersrc_add_frame", rc);
	}

	void parameters_set(AVBufferSrcParameters *const p)
	{
		if (const int rc = av_buffersrc_parameters_set(ctx, p); rc < 0)
			throw av::Error("av_buffersrc_parameters_set", rc);
	}

	void operator<<(AVFrame *const frame) { add_frame(frame); }

private:
	static void validate_is_buffersrc(AVFilterContext *c)
	{
		static const auto buffer = avfilter_get_by_name("buffer"),
						  abuffer = avfilter_get_by_name("abuffer");
		if (c->filter != buffer && c->filter != abuffer)
			throw std::invalid_argument{"this is not a buffersrc!"};
	}
};

} // namespace av
