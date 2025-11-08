#pragma once

extern "C"
{
#include <libavfilter/avfilter.h>
}

#include "Error.hpp"
#include "FilterContext.hpp"
#include "Util.hpp"

namespace av
{

/**
 * Owns an `AVFilterGraph` and provides convenience methods.
 */
class FilterGraph
{
private:
	AVFilterGraph *_fg{};

public:
	FilterGraph()
	{
		if (!(_fg = avfilter_graph_alloc()))
			throw Error("avfilter_graph_alloc", AVERROR(ENOMEM));
	}

	FilterGraph(const char *const filters)
		: FilterGraph{}
	{
		(void)parse(filters);
		configure();
	}

	~FilterGraph() { avfilter_graph_free(&_fg); }

	AVFilterGraph *operator->() { return _fg; }
	operator AVFilterGraph *() { return _fg; }

	FilterContext create_filter(
		const AVFilter *const filter,
		const char *const name,
		const char *const args = {})
	{
		AVFilterContext *filter_ctx;
		if (const int rc = avfilter_graph_create_filter(
				&filter_ctx, filter, name, args, {}, _fg);
			rc < 0)
			throw Error("avfilter_graph_create_filter", rc);
		return filter_ctx;
	}

	FilterContext create_filter(
		const char *const filter,
		const char *const name,
		const char *const args = {})
	{
		return create_filter(av::get_filter_by_name(filter), name, args);
	}

	FilterContext
	alloc_filter(const AVFilter *const filter, const char *const name)
	{
		if (const auto filter_ctx =
				avfilter_graph_alloc_filter(_fg, filter, name))
			return filter_ctx;
		throw Error("avfilter_graph_alloc_filter", AVERROR(ENOMEM));
	}

	FilterContext alloc_filter(const char *const filter, const char *const name)
	{
		return alloc_filter(av::get_filter_by_name(filter), name);
	}

	struct ParseReturn
	{
		AVFilterInOut *inputs, *outputs;
		~ParseReturn()
		{
			avfilter_inout_free(&inputs);
			avfilter_inout_free(&outputs);
		}
	};

	[[nodiscard]] ParseReturn parse(const char *const filters)
	{
		AVFilterInOut *inputs, *outputs;
		if (const int rc =
				avfilter_graph_parse2(_fg, filters, &inputs, &outputs);
			rc < 0)
			throw Error("avfilter_graph_parse2", rc);
		return {inputs, outputs};
	}

	void configure()
	{
		if (const int rc = avfilter_graph_config(_fg, NULL); rc < 0)
			throw Error("avfilter_graph_config", rc);
	}
};

} // namespace av
