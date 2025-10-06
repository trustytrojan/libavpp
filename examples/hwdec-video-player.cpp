#include "util.cpp"
#include <SFML/Graphics.hpp>
#include <portaudio.hpp>

#include <av/Frame.hpp>
#include <av/MediaReader.hpp>
#include <av/SwScaler.hpp>

void play_video(const char *const url)
{
	av::MediaReader format{url};
	const auto vstream = format.find_best_stream(AVMEDIA_TYPE_VIDEO);

	vstream->codecpar->codec_id;

	av::HWDeviceContext hwdevctx{AV_HWDEVICE_TYPE_VAAPI};
	// finish later
}
