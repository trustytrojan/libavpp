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

/**
 * @returns Whether `sample_fmt` is an interleaved/non-planar sample format.
 * @note A return value of `false` implies that `sample_fmt` is a
 * non-interleaved/planar sample format.
 */
inline bool is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
}

template <typename T, bool planar>
consteval AVSampleFormat smpfmt_from_type()
{
	int avsf;

	if (std::is_same_v<T, uint8_t>)
		avsf = AV_SAMPLE_FMT_U8;
	else if (std::is_same_v<T, int16_t>)
		avsf = AV_SAMPLE_FMT_S16;
	else if (std::is_same_v<T, int32_t>)
		avsf = AV_SAMPLE_FMT_S32;
	else if (std::is_same_v<T, float>)
		avsf = AV_SAMPLE_FMT_FLT;
	else if (std::is_same_v<T, int64_t>)
		avsf = AV_SAMPLE_FMT_S64;
	else
		return AV_SAMPLE_FMT_NONE;

	if (planar)
	{
		if (std::is_same_v<T, int64_t>)
			// the s64p enum isn't aligned with the other planar enums...
			avsf = AV_SAMPLE_FMT_S64P;
		else
			avsf += 5;
	}

	return (AVSampleFormat)avsf;
}

} // namespace av
