#pragma once

extern "C"
{
#include <libavutil/samplefmt.h>
#include <portaudio.h>
}

int shared_main(const int argc, const char *const *const argv, void (*func)(const char *const))
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

bool is_interleaved(const AVSampleFormat sample_fmt)
{
	if (sample_fmt >= 0 && sample_fmt <= 4)
		return true;
	if (sample_fmt >= 5 && sample_fmt <= 11)
		return false;
	throw std::runtime_error("is_interleaved: invalid sample format!");
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
		throw std::runtime_error("sample format not supported by portaudio: " + std::to_string(av));
	}

	if (av >= 5)
		pa |= paNonInterleaved;

	return pa;
}

template <typename T, bool planar>
constexpr AVSampleFormat avsf_from_type()
{
	int avsf;

	if (std::is_same_v<T, uint8_t>)
		avsf = AV_SAMPLE_FMT_U8;
	else if (std::is_same_v<T, int16_t>)
		avsf = AV_SAMPLE_FMT_S16;
	else if (std::is_same_v<T, int32_t>)
		avsf = AV_SAMPLE_FMT_S32;
	else if (std::is_same_v<T, float>)
		avsf = AV_SAMPLE_FMT_FLT;
	else if (std::is_same_v<T, int64_t>)
		avsf = AV_SAMPLE_FMT_S64;
	else
		return AV_SAMPLE_FMT_NONE;

	if (planar)
	{
		if (std::is_same_v<T, int64_t>)
			avsf = AV_SAMPLE_FMT_S64P;
		else
			avsf += 5;
	}

	return (AVSampleFormat)avsf;
}
