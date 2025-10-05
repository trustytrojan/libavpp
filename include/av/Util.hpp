#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
}

#include "Error.hpp"

namespace av
{

inline const AVCodec *find_encoder_by_name(const char *const name)
{
	if (const auto codec = avcodec_find_encoder_by_name(name))
		return codec;
	throw Error("avcodec_find_encoder_by_name", AVERROR_ENCODER_NOT_FOUND);
}

inline void link_filters(
	AVFilterContext *src,
	unsigned src_pad,
	AVFilterContext *dst,
	unsigned dst_pad)
{
	if (const int rc = avfilter_link(src, src_pad, dst, dst_pad); rc < 0)
		throw Error("avfilter_link", rc);
}

} // namespace av
