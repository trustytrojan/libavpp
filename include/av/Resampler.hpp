#pragma once

#include "Error.hpp"

extern "C"
{
#include <libswresample/swresample.h>
}

namespace av
{

class Resampler
{
	SwrContext *_ctx = nullptr;

public:
	Resampler(
		const AVChannelLayout *out_ch_layout,
		AVSampleFormat out_sample_fmt,
		int out_sample_rate,
		const AVChannelLayout *in_ch_layout,
		AVSampleFormat in_sample_fmt,
		int in_sample_rate,
		int log_offset = 0,
		void *log_ctx = nullptr)
	{
		if (const auto rc = swr_alloc_set_opts2(&_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, log_offset, log_ctx); rc < 0)
			throw Error("swr_alloc_set_opts2", rc);
		if (const auto rc = swr_init(_ctx); rc < 0)
			throw Error("swr_init", rc);
	}

	~Resampler()
	{
		swr_free(&_ctx);
	}

	void convert_frame(AVFrame *const output, const AVFrame *const input)
	{
		if (const auto rc = swr_convert_frame(_ctx, output, input); rc < 0)
			throw Error("swr_convert_frame", rc);
	}
};

} // namespace av
