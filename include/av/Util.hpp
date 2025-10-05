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

// used to solve the swscale stride issue
inline int nearest_multiple_8(const int x)
{
	const auto rem8 = x % 8;

	// move towards the nearest multiple of 8
	if (rem8 < 4)
		return x - rem8;
	else
		return x + 8 - rem8;
}

inline void ch_layout_copy(AVChannelLayout *dst, const AVChannelLayout *src)
{
	if (const auto rc = av_channel_layout_copy(dst, src); rc < 0)
		throw Error("av_channel_layout_copy", rc);
}

inline void
parameters_from_context(AVCodecParameters *par, const AVCodecContext *codec)
{
	if (const auto rc = avcodec_parameters_from_context(par, codec); rc < 0)
		throw Error("avcodec_parameters_from_context", rc);
}

inline void parameters_to_context(
	AVCodecContext *const codec, const AVCodecParameters *const par)
{
	if (const auto rc = avcodec_parameters_to_context(codec, par); rc < 0)
		throw Error("avcodec_parameters_from_context", rc);
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
