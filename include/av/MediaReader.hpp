#pragma once

#include <vector>

#include "Error.hpp"
#include "FormatContext.hpp"
#include "Stream.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{

/**
 * A format context in "read" or "input" mode. Fetches stream info automatically
 * into a vector, accesible with `streams()`.
 */
class MediaReader : public FormatContext
{
	AVPacket *_pkt = nullptr;
	std::vector<Stream> _streams;

public:
	MediaReader(const char *const url)
	{
		if (const auto rc = avformat_open_input(&_fmtctx, url, NULL, NULL);
			rc < 0)
			throw Error("avformat_open_input", rc);
		if (const auto rc = avformat_find_stream_info(_fmtctx, NULL); rc < 0)
			throw Error("avformat_find_stream_info", rc);
		_streams.assign(
			_fmtctx->streams, _fmtctx->streams + _fmtctx->nb_streams);
	}

	MediaReader(const std::string &url)
		: MediaReader(url.c_str())
	{
	}

	~MediaReader() { av_packet_free(&_pkt); }

	/**
	 * Print detailed information about the input or output format, such as
	 * duration, bitrate, streams, container, programs, metadata, side data,
	 * codec and time base.
	 * @param index index of the stream to dump information about
	 */
	void print_info(int index)
	{
		av_dump_format(_fmtctx, index, _fmtctx->url, 0);
	}

	/**
	 * @returns The streams contained in this media source.
	 */
	const std::vector<Stream> &streams() const { return _streams; }

	/**
	 * Wrapper over `av_dict_get(fmtctx->metadata, ...)`.
	 * @returns The string value associated with `key`, or `nullptr` if no entry
	 * exists for `key`.
	 */
	const char *metadata(
		const char *const key,
		const AVDictionaryEntry *prev = NULL,
		const int flags = 0) const
	{
		const auto *const entry =
			av_dict_get(_fmtctx->metadata, key, prev, flags);
		return entry ? entry->value : nullptr;
	}

	/**
	 * Wrapper over `av_find_best_stream(fmtctx, ...)`.
	 * @returns The highest quality stream of media type `type`.
	 * @throws `av::Error` containing an `errnum` of `AVERROR_STREAM_NOT_FOUND`
	 * or `AVERROR_DECODER_NOT_FOUND`
	 */
	Stream find_best_stream(
		const AVMediaType type,
		int wanted_stream_nb = -1,
		int related_stream = -1,
		const AVCodec **decoder_ret = NULL,
		int flags = 0) const
	{
		const auto idx = av_find_best_stream(
			_fmtctx,
			type,
			wanted_stream_nb,
			related_stream,
			decoder_ret,
			flags);
		if (idx < 0)
			throw Error("av_find_best_stream", idx);
		return _fmtctx->streams[idx];
	}

	// Wrapper over `av_seek_frame`.
	void seek_frame(int stream_index, int64_t timestamp, int flags)
	{
		if (const auto rc =
				av_seek_frame(_fmtctx, stream_index, timestamp, flags);
			rc < 0)
			throw Error("av_seek_frame", rc);
	}

	// Wrapper over `avformat_seek_file`. Recommended over `seek_frame`.
	void seek_file(
		int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
	{
		if (const auto rc = avformat_seek_file(
				_fmtctx, stream_index, min_ts, ts, max_ts, flags);
			rc < 0)
			throw Error("avformat_seek_file", rc);
	}

	/**
	 * @brief Read a packet of data from the media source.
	 *
	 * @return A pointer to the internal `AVPacket`, or `NULL` if end-of-file
	 * has been reached.
	 *
	 * @warning **Do not free/delete the returned pointer.** It belongs to and
	 * is managed by this class.
	 *
	 * @note Each packet read from the source belongs to a stream.
	 * If you are decoding a stream, make sure to check the `stream_index` field
	 * of the `AVPacket` to make sure you are giving the right packet to the
	 * right decoder.
	 */
	const AVPacket *read_packet()
	{
		if (!_pkt && !(_pkt = av_packet_alloc()))
			throw Error("av_packet_alloc", AVERROR(ENOMEM));
		switch (const auto rc = av_read_frame(_fmtctx, _pkt))
		{
		case 0:
			return _pkt;
		case AVERROR_EOF:
			return NULL;
		default:
			throw Error("av_read_frame", rc);
		}
	}
};

} // namespace av
