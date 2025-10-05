#include <cassert>
#include <iostream>
#include <optional>
#include <portaudio.hpp>

#include <av/Frame.hpp>
#include <av/MediaReader.hpp>
#include <av/Resampler.hpp>

#include "util.cpp"

template <typename SampleFormat, bool planar = true>
void with_resampling(const char *const url)
{
	av::MediaReader format(url);
	const auto stream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);
	const auto *const cdpar = stream->codecpar;
	auto decoder = stream.create_decoder();

	std::optional<av::Frame> rs_frame_opt;
	std::optional<av::Resampler> rs_opt;

	// sample format that the caller wants to play
	decoder->request_sample_fmt = av::smpfmt_from_type<SampleFormat, planar>();
	decoder.open();

	// the decoder cannot always provide the requested sample format! we account
	// for this below

	const auto interleaved = av::is_interleaved(decoder->sample_fmt);
	const auto different_formats =
		(decoder->sample_fmt != decoder->request_sample_fmt);

	std::cout << "requested sample format: "
			  << av_get_sample_fmt_name(decoder->request_sample_fmt) << '\n'
			  << "codec/sample_fmt: " << avcodec_get_name(decoder->codec_id)
			  << '/' << av_get_sample_fmt_name(decoder->sample_fmt) << '\n'
			  << "decoder->sample_rate: " << decoder->sample_rate << '\n'
			  << "cdpar->sample_rate: " << cdpar->sample_rate << '\n';

	if (different_formats)
	{
		rs_frame_opt.emplace();				 // create empty frame
		auto rs_frame = rs_frame_opt->get(); // get AVFrame *
		rs_frame->format = decoder->request_sample_fmt;
		rs_frame->sample_rate = cdpar->sample_rate;
		rs_frame->ch_layout = cdpar->ch_layout;
		// create resampler
		rs_opt = {
			{&cdpar->ch_layout, // output params
			 decoder->request_sample_fmt,
			 cdpar->sample_rate},
			{&cdpar->ch_layout, // input params
			 decoder->sample_fmt,
			 cdpar->sample_rate}};
		std::cout << "resampling to: "
				  << av_get_sample_fmt_name(decoder->request_sample_fmt)
				  << '\n';
	}

	pa::Stream pa_stream(
		0,
		cdpar->ch_layout.nb_channels,
		avsf2pasf(decoder->request_sample_fmt),
		cdpar->sample_rate);
	pa_stream.start();

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
				if (!rs_opt || !rs_frame_opt)
					throw std::runtime_error{
						"resampler optionals are not valid!"};
				const auto rs_frame = rs_frame_opt->get();
				rs_opt->convert_frame(rs_frame, frame);
				frame = rs_frame;
			}
			pa_stream.write(
				interleaved ? (void *)frame->extended_data[0]
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

	pa::Stream pa_stream(
		0,
		stream->codecpar->ch_layout.nb_channels,
		avsf2pasf(decoder->sample_fmt),
		stream->codecpar->sample_rate);
	pa_stream.start();

	std::cout << "codec: " << avcodec_get_name(decoder->codec_id) << '\n'
			  << "sample format: "
			  << av_get_sample_fmt_name(decoder->sample_fmt) << '\n';

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (const auto frame = decoder.receive_frame())
		{
			assert(decoder->sample_fmt == frame->format);
			// clang-format off
			pa_stream.write(
				av::is_interleaved(decoder->sample_fmt)
					? (void *)frame->extended_data[0]
					: (void *)frame->extended_data, frame->nb_samples);
			// clang-format on
		}
	}
}

int main(const int argc, const char *const *const argv)
{
	pa::Init _;
	// return shared_main(argc, argv, with_resampling<uint8_t>);
	return shared_main(argc, argv, without_resampling);
}
