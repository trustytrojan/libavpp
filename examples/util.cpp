// shared utility functions for all examples

#pragma once

#include <cstdlib>
#include <iostream>

extern "C"
{
#include <libavutil/samplefmt.h>
#include <portaudio.h>
}

int shared_main(
	const int argc,
	const char *const *const argv,
	void (*func)(const char *const))
{
	if (argc < 2 || !argv[1] || !*argv[1])
	{
		std::cerr << "media url required\n";
		return EXIT_FAILURE;
	}

	try
	{
		func(argv[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

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
		throw std::invalid_argument(
			std::string{"sample format not supported by portaudio: "} +
			av_get_sample_fmt_name(av));
	}

	if (av >= 5)
		pa |= paNonInterleaved;

	return pa;
}
