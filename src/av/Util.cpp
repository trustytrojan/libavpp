#include "../include/av/Util.hpp"
#include "../include/av/Error.hpp"

void av::frame_free(AVFrame *_f)
{
	av_frame_free(&_f);
}

int av::nearest_multiple_8(const int x)
{
	const auto rem8 = x % 8;

	// move towards the nearest multiple of 8
	if (rem8 < 4)
		return x - rem8;
	else
		return x + 8 - rem8;
}

void av::ch_layout_copy(AVChannelLayout *dst, const AVChannelLayout *src)
{
	if (const auto rc = av_channel_layout_copy(dst, src); rc < 0)
		throw Error("av_channel_layout_copy", rc);
}

void av::parameters_from_context(AVCodecParameters *par, const AVCodecContext *codec)
{
	if (const auto rc = avcodec_parameters_from_context(par, codec); rc < 0)
		throw Error("avcodec_parameters_from_context", rc);
}

void av::parameters_to_context(AVCodecContext *const codec, const AVCodecParameters *const par)
{
	if (const auto rc = avcodec_parameters_to_context(codec, par); rc < 0)
		throw Error("avcodec_parameters_from_context", rc);
}

bool av::is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
}