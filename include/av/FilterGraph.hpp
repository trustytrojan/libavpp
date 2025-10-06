#pragma once

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "Error.hpp"
#include "FilterContext.hpp"

namespace av
{

/**
 * Owns an `AVFilterGraph` and provides convenience methods.
 */
class FilterGraph
{
private:
	AVFilterGraph *_fg = nullptr;

public:
	FilterGraph()
	{
		if (!(_fg = avfilter_graph_alloc()))
			throw Error("avfilter_graph_alloc", AVERROR(ENOMEM));
	}

	~FilterGraph() { avfilter_graph_free(&_fg); }

	AVFilterGraph *get() { return _fg; }
	AVFilterGraph *operator->() { return _fg; }

	FilterContext add_filter(
		const AVFilter *const filter,
		const char *const name,
		const char *const args = NULL)
	{
		AVFilterContext *filter_ctx;
		if (const int rc = avfilter_graph_create_filter(
				&filter_ctx, filter, name, args, NULL, _fg);
			rc < 0)
			throw Error("avfilter_graph_create_filter", rc);
		return filter_ctx;
	}

	void parse(const char *const filters)
	{
		AVFilterInOut *inputs, *outputs;
		if (const int rc =
				avfilter_graph_parse2(_fg, filters, &inputs, &outputs);
			rc < 0)
			throw Error("avfilter_graph_parse2", rc);
		avfilter_inout_free(&inputs);
		avfilter_inout_free(&outputs);
	}

	void configure()
	{
		if (const int rc = avfilter_graph_config(_fg, NULL); rc < 0)
			throw Error("avfilter_graph_config", rc);
	}
};

} // namespace av
