/**
 * @file filter-audio.cpp
 * @brief libavpp audio filtering example
 *
 * This example generates a sine wave audio signal, passes it through a filter
 * chain, and computes the MD5 checksum of the output data.
 *
 * The filter chain:
 * (input) -> abuffer -> volume -> aformat -> abuffersink -> (output)
 *
 * Based on FFmpeg's filter_audio.c example, rewritten to use libavpp C++ API.
 */

#define __STDC_CONSTANT_MACROS

#include <av/Error.hpp>
#include <av/FilterContext.hpp>
#include <av/FilterGraph.hpp>
#include <av/Frame.hpp>
#include <av/Util.hpp>

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
#include <libavutil/md5.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>

constexpr int INPUT_SAMPLERATE = 48000;
constexpr AVSampleFormat INPUT_FORMAT = AV_SAMPLE_FMT_FLTP;
constexpr AVChannelLayout INPUT_CHANNEL_LAYOUT = AV_CHANNEL_LAYOUT_5POINT0;
constexpr double VOLUME_VAL = 0.90;
constexpr int FRAME_SIZE = 1024;

class FilterPipeline
{
public:
	av::FilterGraph graph;
	av::FilterContext src;
	av::FilterContext sink;

	FilterPipeline()
		: src{init_filter_graph()}, sink{init_sink()}
	{
		graph.configure();
	}

private:
	AVFilterContext *init_filter_graph()
	{
		// Create and configure the abuffer filter (source)
		const AVFilter *abuffer = av::filter_get_by_name("abuffer");

		av::FilterContext src_ctx = graph.alloc_filter(abuffer, "src");

		// Set abuffer options using AVOptions API
		char ch_layout[64];
		av_channel_layout_describe(&INPUT_CHANNEL_LAYOUT, ch_layout, sizeof(ch_layout));
		src_ctx.opt_set("channel_layout", static_cast<const char *>(ch_layout));
		src_ctx.opt_set("sample_fmt", av_get_sample_fmt_name(INPUT_FORMAT));
		src_ctx.opt_set("time_base", AVRational{1, INPUT_SAMPLERATE});
		src_ctx.opt_set("sample_rate", INPUT_SAMPLERATE);

		// Initialize the abuffer filter
		src_ctx.init(static_cast<const char *>(nullptr));

		// Create and configure the volume filter
		const AVFilter *volume = av::filter_get_by_name("volume");

		av::FilterContext volume_ctx = graph.alloc_filter(volume, "volume");

		// Initialize volume filter with dictionary options
		AVDictionary *options_dict = nullptr;
		av_dict_set(&options_dict, "volume", std::to_string(VOLUME_VAL).c_str(), 0);
		volume_ctx.init(&options_dict);
		av_dict_free(&options_dict);

		// Create and configure the aformat filter
		const AVFilter *aformat = av::filter_get_by_name("aformat");

		av::FilterContext aformat_ctx = graph.alloc_filter(aformat, "aformat");

		// Initialize aformat filter with string options
		char options_str[1024];
		snprintf(
			options_str,
			sizeof(options_str),
			"sample_fmts=%s:sample_rates=%d:channel_layouts=stereo",
			av_get_sample_fmt_name(AV_SAMPLE_FMT_S16),
			44100);
		aformat_ctx.init(static_cast<const char *>(options_str));

		// Create the abuffersink filter (sink)
		const AVFilter *abuffersink = av::filter_get_by_name("abuffersink");

		av::FilterContext sink_ctx = graph.alloc_filter(abuffersink, "sink");

		// Initialize the abuffersink filter
		sink_ctx.init(static_cast<const char *>(nullptr));

		// Link the filters together
		src_ctx.link(0, volume_ctx, 0);
		volume_ctx.link(0, aformat_ctx, 0);
		aformat_ctx.link(0, sink_ctx, 0);

		// Store sink for later use (ugly but necessary due to initialization order)
		_sink = sink_ctx;

		return src_ctx;
	}

	AVFilterContext *init_sink()
	{
		return _sink;
	}

	AVFilterContext *_sink = nullptr;
};

void process_output(AVMD5 *md5, av::Frame &frame)
{
	const int planar = av_sample_fmt_is_planar((AVSampleFormat)frame->format);
	const int channels = frame->ch_layout.nb_channels;
	const int planes = planar ? channels : 1;
	const int bps = av_get_bytes_per_sample((AVSampleFormat)frame->format);
	const int plane_size = bps * frame->nb_samples * (planar ? 1 : channels);

	for (int i = 0; i < planes; ++i)
	{
		uint8_t checksum[16];

		av_md5_init(md5);
		av_md5_sum(checksum, frame->extended_data[i], plane_size);

		std::cout << "plane " << i << ": 0x";
		for (int j = 0; j < 16; ++j)
			std::cout << std::hex << std::setw(2) << std::setfill('0')
					  << static_cast<int>(checksum[j]);
		std::cout << std::dec << '\n';
	}
	std::cout << '\n';
}

void generate_sine_wave(av::Frame &frame, int frame_num)
{
	// Set up frame properties
	frame->sample_rate = INPUT_SAMPLERATE;
	frame->format = INPUT_FORMAT;
	av_channel_layout_copy(&frame->ch_layout, &INPUT_CHANNEL_LAYOUT);
	frame->nb_samples = FRAME_SIZE;
	frame->pts = frame_num * FRAME_SIZE;

	// Allocate buffer for the frame
	frame.get_buffer();

	// Fill each channel with sine wave data
	for (int i = 0; i < 5; ++i)
	{
		float *data = reinterpret_cast<float *>(frame->extended_data[i]);
		for (int j = 0; j < frame->nb_samples; ++j)
			data[j] = std::sin(2 * M_PI * (frame_num + j) * (i + 1) / FRAME_SIZE);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <duration>\n";
		return 1;
	}

	const float duration = std::atof(argv[1]);
	const int nb_frames = duration * INPUT_SAMPLERATE / FRAME_SIZE;
	if (nb_frames <= 0)
	{
		std::cerr << "Invalid duration: " << argv[1] << '\n';
		return 1;
	}

	try
	{
		// Allocate frame and MD5 context
		av::OwnedFrame frame;
		AVMD5 *md5 = av_md5_alloc();
		if (!md5)
			throw av::Error("av_md5_alloc", AVERROR(ENOMEM));

		// Set up the filter pipeline
		FilterPipeline pipeline;

		// Main filtering loop
		for (int i = 0; i < nb_frames; ++i)
		{
			// Generate input frame
			generate_sine_wave(frame, i);

			// Send frame to the filter graph
			pipeline.src.add_frame(frame);

			// Get all filtered output that is available
			while (true)
			{
				try
				{
					pipeline.sink.get_frame(frame);
					process_output(md5, frame);
					frame.unref();
				}
				catch (const av::Error &e)
				{
					if (e.errnum == AVERROR(EAGAIN))
					{
						// Need to feed more frames
						break;
					}
					else if (e.errnum == AVERROR_EOF)
					{
						// No more output
						av_freep(&md5);
						return 0;
					}
					else
					{
						// An error occurred
						throw;
					}
				}
			}
		}

		av_freep(&md5);
		return 0;
	}
	catch (const av::Error &e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}
}
