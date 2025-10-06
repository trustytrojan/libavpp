#include <cassert>
#include <portaudio.hpp>

#include <av/Frame.hpp>
#include <av/MediaReader.hpp>

#include "util.cpp"

void play_audio(const char *const url)
{
	av::MediaReader format{url};
	const auto stream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);
	auto decoder = stream.create_decoder();
	decoder.open();

	pa::Init _;
	pa::Stream pa_stream(
		0,
		stream->codecpar->ch_layout.nb_channels,
		avsf2pasf(decoder->sample_fmt),
		stream->codecpar->sample_rate);
	pa_stream.start();

	while (const auto packet = format.read_packet())
	{
		if (packet->stream_index != stream->index)
			continue;
		if (!decoder.send_packet(packet))
			break;
		while (const auto frame = decoder.receive_frame())
		{
			assert(decoder->sample_fmt == frame->format);
			pa_stream.write(
				av_sample_fmt_is_planar(decoder->sample_fmt)
					? (void *)frame->extended_data
					: (void *)frame->extended_data[0],
				frame->nb_samples);
		}
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, play_audio);
}
