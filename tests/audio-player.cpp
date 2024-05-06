#include <iostream>
#include <optional>
#include <cassert>
#include <av.hpp>
#include "../../portaudio-pp/portaudio.hpp"
#include "util.hpp"

#include "../src/av/Util.cpp"

template <typename SampleFormat, bool planar = true>
void with_resampling(const char *const url)
{
	av::MediaReader format(url);
	const auto stream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);
	const auto cdpar = stream->codecpar;
	auto decoder = stream.create_decoder();

	std::optional<av::Frame> rs_frame_opt;
	std::optional<av::Resampler> rs_opt;

	decoder->request_sample_fmt = av::smpfmt_from_type<SampleFormat, planar>();
	decoder.open();

	const auto interleaved = av::is_interleaved(decoder->sample_fmt);
	const auto different_formats = (decoder->sample_fmt != decoder->request_sample_fmt);

	std::cout << "requested sample format: " << av_get_sample_fmt_name(decoder->request_sample_fmt)
			  << "\ncodec/sample_fmt: " << avcodec_get_name(decoder->codec_id) << '/' << av_get_sample_fmt_name(decoder->sample_fmt) << '\n';

	if (different_formats)
	{
		rs_frame_opt.emplace(); // create empty frame
		auto rs_frame = rs_frame_opt->get();
		rs_frame->format = decoder->request_sample_fmt;
		rs_frame->sample_rate = stream->codecpar->sample_rate;
		rs_frame->ch_layout = stream->codecpar->ch_layout;
		rs_opt.emplace(&decoder->ch_layout, decoder->request_sample_fmt, decoder->sample_rate, // output params
					   &decoder->ch_layout, decoder->sample_fmt, decoder->sample_rate);		 // input params
		std::cout << "resampling to: " << av_get_sample_fmt_name(decoder->request_sample_fmt) << '\n';
	}

	pa::PortAudio _;
	pa::Stream pa_stream(0, cdpar->ch_layout.nb_channels,
						 avsf2pasf(decoder->request_sample_fmt),
						 cdpar->sample_rate,
						 paFramesPerBufferUnspecified);
	
	std::cout << "pasf: " << avsf2pasf(decoder->sample_fmt) << '\n';

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (auto frame = decoder.receive_frame())
		{
			assert(decoder->sample_fmt == frame->format);
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

	decoder.open();

	pa::PortAudio _;
	pa::Stream pa_stream(0, stream->codecpar->ch_layout.nb_channels,
						 avsf2pasf(decoder->sample_fmt),
						 stream->codecpar->sample_rate,
						 paFramesPerBufferUnspecified);

	std::cout << "codec: " << avcodec_get_name(decoder->codec_id) << '\n'
			  << "sample format: " << av_get_sample_fmt_name(decoder->sample_fmt) << '\n';

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (const auto frame = decoder.receive_frame())
		{
			assert(decoder->sample_fmt == frame->format);
			pa_stream.write(av::is_interleaved(decoder->sample_fmt)
								? (void *)frame->extended_data[0]
								: (void *)frame->extended_data,
							frame->nb_samples);
		}
	}
}

int main(const int argc, const char *const *const argv)
{
	// return shared_main(argc, argv, with_resampling<uint8_t, false>);
	return shared_main(argc, argv, without_resampling);
}
