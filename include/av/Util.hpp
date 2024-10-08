#pragma once

#include <stdexcept>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavfilter/avfilter.h>
}

namespace av
{

// Wrapper over `av_frame_free`. Used by `av::Frame`.
void frame_free(AVFrame *_f);

// used to solve the swscale stride issue
int nearest_multiple_8(const int x);

void ch_layout_copy(AVChannelLayout *dst, const AVChannelLayout *src);
void parameters_from_context(AVCodecParameters *par, const AVCodecContext *ctx);
void parameters_to_context(AVCodecContext *ctx, const AVCodecParameters *par);

void link_filters(AVFilterContext *src, unsigned src_pad, AVFilterContext *dst, unsigned dst_pad);

/**
 * @returns Whether `sample_fmt` is an interleaved/non-planar sample format.
 * @note A return value of `false` implies that `sample_fmt` is a
 * non-interleaved/planar sample format.
 */
constexpr bool is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
}

template <typename T, bool planar>
constexpr AVSampleFormat smpfmt_from_type()
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
