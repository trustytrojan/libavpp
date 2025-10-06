#include "util.cpp"
#include <SFML/Graphics.hpp>
#include <av/MediaReader.hpp>

void display_attached_pic(const char *const url)
{
	av::MediaReader format(url);

	const auto is_attached_pic = [](const av::Stream &s)
	{
		return s->disposition & AV_DISPOSITION_ATTACHED_PIC;
	};

	// find attached pic stream
	const auto itr = std::ranges::find_if(format.streams(), is_attached_pic);
	if (itr == format.streams().end())
		throw std::runtime_error{"no attached pic found in media file!"};
	const auto &stream = *itr;

	// setup sfml window, load & draw texture
	sf::RenderWindow window(
		sf::VideoMode({
			static_cast<unsigned int>(stream->codecpar->width),
			static_cast<unsigned int>(stream->codecpar->height),
		}),
		"av-test2");
	window.setVerticalSyncEnabled(true);

	sf::Texture texture{
		stream->attached_pic.data,
		static_cast<size_t>(stream->attached_pic.size)};
	sf::Sprite sprite{texture};

	while (window.isOpen())
	{
		window.draw(sprite);
		window.display();
		while (const auto event = window.pollEvent())
			if (event->is<sf::Event::Closed>())
				window.close();
	}
}

int main(const int argc, const char *const *const argv)
{
	return shared_main(argc, argv, display_attached_pic);
}
