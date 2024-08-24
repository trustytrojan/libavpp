#pragma once

#include "Decoder.hpp"
#include "Encoder.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{

// Non-owning wrapper class over an `AVStream`.
class Stream
{
	// the pointer must be non-const to keep the class copyable
	AVStream *_s;

public:
	Stream(AVStream *const _s)
		: _s(_s)
	{
	}

	/**
	 * @return A pointer to the contained `AVStream`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaReader`.
	 */
	AVStream *get() { return _s; }

	/**
	 * @return A read-only pointer to the contained `AVStream`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaReader`.
	 */
	const AVStream *get() const { return _s; }

	/**
	 * Access fields the contained `AVStream`.
	 * @return A pointer to the contained `AVStream`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaReader`.
	 */
	AVStream *operator->() { return _s; }

	/**
	 * Access fields the contained `AVStream`.
	 * @return A read-only pointer to the contained `AVStream`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaReader`.
	 */
	const AVStream *operator->() const { return _s; }

	/**
	 * Wrapper over `av_dict_get` using the contained `AVStream`'s `metadata` field.
	 * @returns The value associated with `key` in stream `i`, or `NULL` if no entry exists for `key`.
	 */
	const char *metadata(const char *const key, const AVDictionaryEntry *prev = NULL, const int flags = 0) const
	{
		const auto entry = av_dict_get(_s->metadata, key, prev, flags);
		return entry ? entry->value : NULL;
	}

	/**
	 * @return The duration of the stream, in seconds.
	 */
	double duration_sec() const { return _s->duration * av_q2d(_s->time_base); }

	/**
	 * @return The total number of audio samples (per channel) in this stream.
	 */
	int samples() const { return duration_sec() * sample_rate(); }

	/**
	 * @return The number of audio channels in this stream.
	 */
	int nb_channels() const { return _s->codecpar->ch_layout.nb_channels; }

	/**
	 * @return The audio sample rate of this stream.
	 */
	int sample_rate() const { return _s->codecpar->sample_rate; }

	/**
	 * @return A `Decoder` using this stream's codec.
	 * @note Codec parameters are not copied to the context.
	 */
	Decoder create_decoder() const { return avcodec_find_decoder(_s->codecpar->codec_id); }

	/**
	 * @return An `Encoder` using this stream's codec.
	 * @note Codec parameters are not copied to the context.
	 */
	Encoder create_encoder() const { return avcodec_find_encoder(_s->codecpar->codec_id); }
};

} // namespace av
