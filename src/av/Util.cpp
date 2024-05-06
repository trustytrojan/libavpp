#include "../include/av/Util.hpp"

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

bool av::is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
}