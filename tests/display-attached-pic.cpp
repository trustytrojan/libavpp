#include "include/av/MediaFileReader.hpp"
#include "include/av/StreamDecoder.hpp"
#include "include/av/Scaler.hpp"

#include "include/pa/PortAudio.hpp"
#include "include/pa/Stream.hpp"

#include <SFML/Graphics.hpp>

#include <iostream>
#include <ranges>

void display_attached_pic(const char *const url)
{
	av::MediaReader format(url);

	// find attached pic stream
	const auto itr = std::ranges::find_if(format.streams(), [](const av::Stream &s)
										  { return s->disposition & AV_DISPOSITION_ATTACHED_PIC; });
	if (itr == format.streams().cend())
		throw std::runtime_error("no attached pic found in media file!");
	const auto &stream = *itr;

	// setup sfml window, load & draw texture
	sf::RenderWindow window(sf::VideoMode({stream->codecpar->width, stream->codecpar->height}), "av-test2");
	window.setVerticalSyncEnabled(true);

	sf::Texture texture;
	if (!texture.loadFromMemory(stream->attached_pic.data, stream->attached_pic.size))
		return;
	sf::Sprite sprite(texture);

	while (window.isOpen())
	{
		window.draw(sprite);
		window.display();
		{ // handle events
			sf::Event event;
			while (window.pollEvent(event))
				if (event.is<sf::Event::Closed>())
					window.close();
		}
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
		display_attached_pic(argv[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}