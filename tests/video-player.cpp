#include "include/av/MediaFileReader.hpp"
#include "include/av/StreamDecoder.hpp"
#include "include/av/Scaler.hpp"

#include "include/pa/PortAudio.hpp"
#include "include/pa/Stream.hpp"

#include <SFML/Graphics.hpp>

PaSampleFormat avsf2pasf(const AVSampleFormat av)
{
	PaSampleFormat pa;

	switch (av)
	{
	case AV_SAMPLE_FMT_U8:
	case AV_SAMPLE_FMT_U8P:
		pa = paUInt8;
		break;
	case AV_SAMPLE_FMT_S16:
	case AV_SAMPLE_FMT_S16P:
		pa = paInt16;
		break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
		pa = paInt32;
		break;
	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_FLTP:
		pa = paFloat32;
		break;
	default:
		return AV_SAMPLE_FMT_NONE;
		// throw std::runtime_error("sample format not supported by portaudio: " + std::to_string(av));
	}

	if (av >= 5)
		pa |= paNonInterleaved;

	return pa;
}

bool is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
}

void play_video(const char *const url)
{
	av::MediaReader format(url);

	const auto &vstream = format.find_best_stream(AVMEDIA_TYPE_VIDEO);
	const auto &astream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);

	auto vdecoder = vstream.create_decoder();
	auto adecoder = astream.create_decoder();

	vdecoder.open();
	adecoder.open();

	// create portaudio stream using audio decoder's output format
	pa::PortAudio _;
	pa::Stream pa_stream(0, astream->codecpar->ch_layout.nb_channels,
						 avsf2pasf((AVSampleFormat)astream->codecpar->format),
						 astream->codecpar->sample_rate,
						 paFramesPerBufferUnspecified);
	
	// stride issue: if width is not divisible by 8, output will be distorted
	const auto desired_width = 1280;
	const auto desired_height = 720;

	// create scaler to convert from yuv420p to rgba
	av::Scaler scaler(vstream->codecpar->width, vstream->codecpar->height, (AVPixelFormat)vstream->codecpar->format,
					  desired_width, desired_height, AV_PIX_FMT_RGBA);

	// create sfml window, texture, and sprite
	sf::RenderWindow window(sf::VideoMode({desired_width, desired_height}), "video-player", sf::Style::Titlebar);
	window.setVerticalSyncEnabled(true);

	sf::Texture texture;
	if (!texture.create({desired_width, desired_height}))
		throw std::runtime_error("failed to create texture");

	sf::Sprite sprite(texture);

	// destination frame used by scaler
	av::Frame scaled_frame;

	while (const auto packet = format.read_packet())
	{
		if (!window.isOpen())
			break;

		if (packet->stream_index == vstream->index)
		{
			vdecoder.send_packet(packet);
			const auto frame = vdecoder.receive_frame();
			assert(frame->format != AV_PIX_FMT_RGBA);
			scaler.scale_frame(scaled_frame.get(), frame);
			assert(scaled_frame->format == AV_PIX_FMT_RGBA);
			texture.update(scaled_frame->data[0]);
			sprite.setTexture(texture, true);
			window.draw(sprite);
			window.display();
		}
		else if (packet->stream_index == astream->index)
		{
			adecoder.send_packet(packet);
			while (const auto frame = adecoder.receive_frame())
				try
				{
					pa_stream.write(is_interleaved(adecoder.cdctx()->sample_fmt)
										? (void *)frame->extended_data[0]
										: (void *)frame->extended_data,
									frame->nb_samples);
				}
				catch (const pa::Error &e)
				{
					if (e.code != paOutputUnderflowed)
						throw;
					std::cerr << e.what() << "\n";
				}
		}

		sf::Event event;
		while (window.pollEvent(event))
			if (event.is<sf::Event::Closed>())
				window.close();
	}
}

int main(const int argc, const char *const *const argv)
{
	if (argc < 2 || !argv[1] || !*argv[1])
	{
		std::cerr << "media url required\n";
		return EXIT_FAILURE;
	}

	try
	{
		play_video(argv[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}