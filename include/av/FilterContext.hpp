#pragma once

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

#include "Error.hpp"

namespace av
{

/**
 * Non-owning wrapper of `AVFilterContext` with convenience methods.
 */
class FilterContext
{
	AVFilterContext *const ctx;

public:
	FilterContext(AVFilterContext *const ctx)
		: ctx{ctx}
	{
	}

	AVFilterContext *operator->() { return ctx; }
	operator AVFilterContext *() { return ctx; }

	void add_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersrc_add_frame(ctx, frame); rc < 0)
			throw Error("av_buffersrc_add_frame", rc);
	}

	void get_frame(AVFrame *const frame)
	{
		if (const int rc = av_buffersink_get_frame(ctx, frame); rc < 0)
			throw Error("av_buffersink_get_frame", rc);
	}

private:
	template <typename T>
	static constexpr auto get_init_func()
	{
		if constexpr (std::is_same_v<T, const char *>)
			return avfilter_init_str;
		else if constexpr (std::is_same_v<T, AVDictionary **>)
			return avfilter_init_dict;
		else
			static_assert(
				sizeof(T) == 0,
				"Unsupported type for init: use const char* or AVDictionary**");
	}

	template <typename T>
	static constexpr auto get_opt_set_func()
	{
		if constexpr (std::is_same_v<T, const char *>)
			return av_opt_set;
		else if constexpr (std::is_same_v<T, int64_t> || std::is_integral_v<T>)
			return av_opt_set_int;
		else if constexpr (std::is_same_v<T, AVRational>)
			return av_opt_set_q;
		else
			static_assert(sizeof(T) == 0, "Unsupported type for opt_set");
	}

public:
	template <typename T>
	void init(const T args)
	{
		if (const int rc = get_init_func<T>()(ctx, args); rc < 0)
			throw Error("avfilter_init", rc);
	}

	template <typename T>
	void opt_set(
		const char *const name,
		const T val,
		const int search_flags = AV_OPT_SEARCH_CHILDREN)
	{
		if (const int rc = get_opt_set_func<T>()(ctx, name, val, search_flags);
			rc < 0)
			throw Error("av_opt_set", rc);
	}

	void link(
		const unsigned src_pad,
		AVFilterContext *const dst,
		const unsigned dst_pad)
	{
		if (const int rc = avfilter_link(ctx, src_pad, dst, dst_pad); rc < 0)
			throw Error("avfilter_link", rc);
	}
};

} // namespace av
