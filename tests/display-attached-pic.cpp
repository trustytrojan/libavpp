#include "util.hpp"
#include <SFML/Graphics.hpp>
#include <av.hpp>
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
		while (const auto event = window.pollEvent())
			if (event.is<sf::Event::Closed>())
				window.close();
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, display_attached_pic);
}