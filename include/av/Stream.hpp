#pragma once

#include "StreamDecoder.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{
	/**
	 * Non-owning wrapper class over an `AVStream`.
	 * @warning When using `operator->`, **do not free/delete the returned pointer.**
	 * It belongs to an `AVFormatContext`, which is wrapped by `MediaFileReader`.
	 */
	class Stream
	{
		// the pointer must be non-const to keep the class copyable
		const AVStream *_s;

	public:
		Stream(const AVStream *const _s) : _s(_s) {}

		/**
		 * Dereference the contained `AVStream *`.
		 * @return A const-reference to the contained `AVStream`.
		 * @warning **Do not get the address of the returned reference in order to free/delete the pointer.**
		 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaFileReader`.
		 */
		const AVStream &operator*() const { return *_s; }

		/**
		 * Access fields the contained `AVStream`.
		 * @return A read-only pointer to the contained `AVStream`.
		 * @warning **Do not free/delete the returned pointer.**
		 * It belongs to and is managed by an `AVFormatContext`, which is wrapped by `MediaFileReader`.
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
		double duration_sec() const
		{
			return _s->duration * av_q2d(_s->time_base);
		}

		/**
		 * @return The total number of audio samples (per channel) in this stream.
		 */
		int samples() const
		{
			return duration_sec() * sample_rate();
		}

		/**
		 * @return The number of audio channels in this stream.
		 */
		int nb_channels() const
		{
			return _s->codecpar->ch_layout.nb_channels;
		}

		/**
		 * @return The audio sample rate of this stream.
		 */
		int sample_rate() const
		{
			return _s->codecpar->sample_rate;
		}

		/**
		 * @return A `StreamDecoder` using this stream as its source.
		 */
		StreamDecoder create_decoder() const
		{
			return _s;
		}
	};
}