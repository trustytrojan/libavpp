// SFML+portaudio-based media player with libavpp
// only software decoding & scaling

#include "util.cpp"
#include <SFML/Graphics.hpp>
#include <portaudio.hpp>

#include <av/Frame.hpp>
#include <av/MediaReader.hpp>
#include <av/SwScaler.hpp>

// Copies a padded frame to a tightly-packed buffer and updates the texture.
void update_texture_from_padded_frame(
	sf::Texture &texture, const AVFrame *const frame, const sf::Vector2u &size)
{
	const int linesize = frame->linesize[0];
	const int width_bytes = size.x * 4; // 4 bytes for RGBA

	static std::vector<uint8_t> pixel_buffer;
	pixel_buffer.resize(width_bytes * size.y);

	const uint8_t *src_pixels = frame->data[0];
	uint8_t *dst_pixels = pixel_buffer.data();

	for (unsigned i = 0; i < size.y; ++i)
	{
		// Copy one line of pixels
		memcpy(dst_pixels, src_pixels, width_bytes);
		// Move pointers to the start of the next line
		dst_pixels += width_bytes; // Tightly packed destination
		src_pixels += linesize;	   // Padded source
	}
	texture.update(pixel_buffer.data());
}

// Handles potential stride/linesize issues by copying frame data line-by-line
// if padding is present.
void update_texture_from_frame(
	sf::Texture &texture, const AVFrame *const frame, const sf::Vector2u &size)
{
	// Check if the frame has padding
	const int linesize = frame->linesize[0];
	const int width_bytes = size.x * 4; // 4 bytes for RGBA

	if (linesize == width_bytes)
	{
		// No padding, we can update directly
		texture.update(frame->data[0]);
	}
	else
	{
		// Padding exists, copy line by line
		update_texture_from_padded_frame(texture, frame, size);
	}
}

void play_video(const char *const url)
{
	av::MediaReader format(url);
	format.print_info(0);

	const auto &vstream = format.find_best_stream(AVMEDIA_TYPE_VIDEO);
	const auto &astream = format.find_best_stream(AVMEDIA_TYPE_AUDIO);

	auto vdecoder = vstream.create_decoder();
	auto adecoder = astream.create_decoder();

	vdecoder.open();
	adecoder.open();

	// create portaudio stream using audio decoder's output format
	pa::Init _;
	pa::Stream pa_stream(
		0,
		astream.nb_channels(),
		avsf2pasf((AVSampleFormat)astream->codecpar->format),
		astream.sample_rate());
	pa_stream.start();

	const sf::Vector2u size(
		vstream->codecpar->width, vstream->codecpar->height);

	// create scaler to convert to rgba
	av::SwScaler scaler{
		{size.x, size.y, (AVPixelFormat)vstream->codecpar->format},
		{size.x, size.y, AV_PIX_FMT_RGBA}};
	av::OwnedFrame scaled_frame;

	// create sfml window, texture, and sprite
	sf::RenderWindow window{
		sf::VideoMode{size}, "video-player", sf::Style::Titlebar};
	sf::Texture texture{size};
	sf::Sprite sprite{texture};

	while (const auto packet = format.read_packet())
	{
		if (!window.isOpen())
			break;
		while (const auto event = window.pollEvent())
			if (event->is<sf::Event::Closed>())
				window.close();

		if (packet->stream_index == vstream->index)
		{
			// decode, scale, and render video frame
			vdecoder.send_packet(packet);
			while (const auto frame = vdecoder.receive_frame())
			{
				scaler.scale_frame(scaled_frame.get(), frame);
				update_texture_from_frame(texture, scaled_frame.get(), size);
				window.draw(sprite);
				window.display();
			}
		}
		else if (packet->stream_index == astream->index)
		{
			// decode and play audio samples
			adecoder.send_packet(packet);
			while (const auto frame = adecoder.receive_frame())
				try
				{
					pa_stream.write(
						av_sample_fmt_is_planar(adecoder->sample_fmt)
							? (void *)frame->extended_data
							: (void *)frame->extended_data[0],
						frame->nb_samples);
				}
				catch (const pa::Error &e)
				{
					if (e.code != paOutputUnderflowed)
						throw;
					std::cerr << e.what() << '\n';
				}
		}
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, play_video);
}
