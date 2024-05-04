#include <iostream>
#include <optional>
#include <cassert>

#include "include/av/MediaFileReader.hpp"
#include "include/av/StreamDecoder.hpp"
#include "include/av/Resampler.hpp"
#include "include/av/Frame.hpp"

#include "include/pa/PortAudio.hpp"
#include "include/pa/Stream.hpp"

#include "util.hpp"

template <typename SampleFormat, bool planar = true>
void with_resampling(const char *const url)
{
	av::MediaReader format(url);
	const auto stream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);
	const auto cdpar = stream->codecpar;
	auto decoder = stream.create_decoder();

	std::optional<av::Frame> rs_frame_opt;
	std::optional<av::Resampler> rs_opt;

	const auto cdctx = decoder.cdctx();
	cdctx->request_sample_fmt = avsf_from_type<SampleFormat, planar>();
	decoder.open();

	const auto interleaved = is_interleaved(cdctx->sample_fmt);
	const auto different_formats = (cdctx->sample_fmt != cdctx->request_sample_fmt);

	std::cout << "requested sample format: " << av_get_sample_fmt_name(cdctx->request_sample_fmt)
			  << "\ncodec/sample_fmt: " << avcodec_get_name(decoder.codec->id) << '/' << av_get_sample_fmt_name(cdctx->sample_fmt) << '\n';

	if (different_formats)
	{
		rs_frame_opt.emplace(); // create empty frame
		auto rs_frame = rs_frame_opt->get();
		rs_frame->format = cdctx->request_sample_fmt;
		rs_frame->sample_rate = stream->codecpar->sample_rate;
		rs_frame->ch_layout = stream->codecpar->ch_layout;
		rs_opt.emplace(&cdctx->ch_layout, cdctx->request_sample_fmt, cdctx->sample_rate, // output params
					   &cdctx->ch_layout, cdctx->sample_fmt, cdctx->sample_rate);		 // input params
		std::cout << "resampling to: " << av_get_sample_fmt_name(cdctx->request_sample_fmt) << '\n';
	}

	pa::PortAudio _;
	pa::Stream pa_stream(0, cdpar->ch_layout.nb_channels,
						 avsf2pasf(cdctx->request_sample_fmt),
						 cdpar->sample_rate,
						 paFramesPerBufferUnspecified);

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (auto frame = decoder.receive_frame())
		{
			assert(cdctx->sample_fmt == frame->format);
			if (different_formats)
			{
				const auto rs_frame = rs_frame_opt->get();
				rs_opt->convert_frame(rs_frame, frame);
				frame = rs_frame;
			}
			pa_stream.write(interleaved
								? (void *)frame->extended_data[0]
								: (void *)frame->extended_data,
							frame->nb_samples);
		}
	}
}

void without_resampling(const char *const url)
{
	av::MediaReader format(url);
	const auto stream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);
	auto decoder = stream.create_decoder();

	const auto cdctx = decoder.cdctx();
	decoder.open();

	pa::PortAudio _;
	pa::Stream pa_stream(0, stream->codecpar->ch_layout.nb_channels,
						 avsf2pasf(cdctx->sample_fmt),
						 stream->codecpar->sample_rate,
						 paFramesPerBufferUnspecified);

	std::cout << "codec: " << avcodec_get_name(decoder.codec->id) << '\n'
			  << "sample format: " << av_get_sample_fmt_name(cdctx->sample_fmt) << '\n';

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != decoder.stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (const auto frame = decoder.receive_frame())
		{
			assert(cdctx->sample_fmt == frame->format);
			pa_stream.write(is_interleaved(cdctx->sample_fmt)
								? (void *)frame->extended_data[0]
								: (void *)frame->extended_data,
							frame->nb_samples);
		}
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, with_resampling<uint8_t, false>);
	// with_resampling<uint8_t>(argv[1]);
	// without_resampling(argv[1]);
}
