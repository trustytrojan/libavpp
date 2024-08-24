#include "util.cpp"
#include <SFML/Graphics.hpp>
#include <portaudio.hpp>

#include "../../include/av/Frame.hpp"
#include "../../include/av/MediaReader.hpp"
#include "../../include/av/SwScaler.hpp"

// force to include this until i make libavpp a compiled library
#include "../av/Util.cpp"

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
	pa::Stream pa_stream(
		0,
		astream->codecpar->ch_layout.nb_channels,
		avsf2pasf((AVSampleFormat)astream->codecpar->format),
		astream->codecpar->sample_rate,
		paFramesPerBufferUnspecified);
	pa_stream.start();

	// libswscale stride issue: if width is not divisible by 8, output will be distorted
	const sf::Vector2u size = {
		static_cast<unsigned int>(av::nearest_multiple_8(vstream->codecpar->width)),
		static_cast<unsigned int>(vstream->codecpar->height),
	};

	// create scaler to convert from yuv420p to rgba
	av::SwScaler scaler({size.x, size.y, (AVPixelFormat)vstream->codecpar->format}, {size.x, size.y, AV_PIX_FMT_RGBA});
	av::Frame scaled_frame;

	// create sfml window, texture, and sprite
	sf::RenderWindow window(sf::VideoMode(size), "video-player", sf::Style::Titlebar);
	sf::Texture texture(size);

	sf::Sprite sprite(texture);
	while (const auto packet = format.read_packet())
	{
		if (!window.isOpen())
			break;

		if (packet->stream_index == vstream->index)
		{
			// decode, scale, and render video frame
			vdecoder.send_packet(packet);
			const auto frame = vdecoder.receive_frame();
			scaler.scale_frame(scaled_frame.get(), frame);
			texture.update(scaled_frame->data[0]);
			sprite.setTexture(texture, true);
			window.draw(sprite);
			window.display();
		}
		else if (packet->stream_index == astream->index)
		{
			// decode and play audio samples
			adecoder.send_packet(packet);
			while (const auto frame = adecoder.receive_frame())
				try
				{
					pa_stream.write(
						av::is_interleaved(adecoder->sample_fmt) ? (void *)frame->extended_data[0] : (void *)frame->extended_data, frame->nb_samples);
				}
				catch (const pa::Error &e)
				{
					if (e.code != paOutputUnderflowed)
						throw;
					std::cerr << e.what() << '\n';
				}
		}

		while (const auto event = window.pollEvent())
			if (event->is<sf::Event::Closed>())
				window.close();
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, play_video);
}
