#pragma once

#include "Error.hpp"
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>

extern "C"
{
#include <libswresample/swresample.h>
}

namespace av
{

class Resampler
{
public:
	struct InOutParams
	{
		const AVChannelLayout *ch_layout;
		AVSampleFormat sample_fmt;
		int sample_rate;
	};

private:
	SwrContext *_ctx = nullptr;

public:
	Resampler(
		const InOutParams &out,
		const InOutParams &in,
		int log_offset = 0,
		void *log_ctx = nullptr)
	{
		// clang-format off
		if (const auto rc = swr_alloc_set_opts2(
				&_ctx,
				out.ch_layout, out.sample_fmt, out.sample_rate,
				in.ch_layout, in.sample_fmt, in.sample_rate,
				log_offset, log_ctx); rc < 0)
			throw Error("swr_alloc_set_opts2", rc);
		// clang-format on
		if (const auto rc = swr_init(_ctx); rc < 0)
			throw Error("swr_init", rc);
	}

	~Resampler() { swr_free(&_ctx); }

	void convert_frame(AVFrame *const output, const AVFrame *const input)
	{
		if (const auto rc = swr_convert_frame(_ctx, output, input); rc < 0)
			throw Error("swr_convert_frame", rc);
	}
};

} // namespace av
