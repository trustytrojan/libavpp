#pragma once

#include "Error.hpp"
#include "FormatContext.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{

struct MediaWriter : FormatContext
{
public:
	/**
	 * @param url URL to write the media file to
	 * @param oformat format to use for allocating the context, if NULL format_name and filename are used instead
	 * @throws `av::Error` if `avformat_alloc_output_context2` fails
	 */
	MediaWriter(const char *const url, const AVOutputFormat *const oformat = NULL)
	{
		if (const auto rc = avformat_alloc_output_context2(&_fmtctx, oformat, NULL, url); rc < 0)
			throw Error("avformat_alloc_output_context2", rc);
		if (!(_fmtctx->flags & AVFMT_NOFILE))
		{
			if (const auto rc = avio_open(&_fmtctx->pb, "out.mp4", AVIO_FLAG_WRITE); rc < 0)
				throw av::Error("avio_open", rc);
		}
	}

	Stream add_stream()
	{
		if (const auto stream = avformat_new_stream(_fmtctx, NULL))
			return stream;
		throw std::runtime_error("could not add stream to format!");
	}

	/**
	 * Allocate the stream private data and write the stream header to an output media file.
	 * @param options  An `AVDictionary` filled with `AVFormatContext` and muxer-private options.
	 * On return this parameter will be destroyed and replaced with a dict containing options that were not found. May be `NULL`.
	 * @retval `AVSTREAM_INIT_IN_WRITE_HEADER` On success, if the codec had not already been fully initialized in avformat_init_output().
	 * @retval `AVSTREAM_INIT_IN_INIT_OUTPUT` On success, if the codec had already been fully initialized in avformat_init_output().
	 * @throws `av::Error` if `avformat_write_header` fails
	 */
	int write_header(AVDictionary **options = NULL)
	{
		const auto rc = avformat_write_header(_fmtctx, options);
		if (rc < 0)
			throw Error("avformat_write_header", rc);
		return rc;
	}

	/**
	 * @param index User-defined input/output number. This is printed as `Output #0, mp4, ...` if `index` is `0`.
	 */
	void print_info(int index)
	{
		av_dump_format(_fmtctx, index, _fmtctx->url, 1);
	}

	void write_packet(AVPacket *const pkt)
	{
		if (const auto rc = av_interleaved_write_frame(_fmtctx, pkt); rc < 0)
			throw Error("av_interleaved_write_frame", rc);
	}

	void write_trailer()
	{
		if (const auto rc = av_write_trailer(_fmtctx); rc < 0)
			throw Error("av_write_trailer", rc);
	}
};

} // namespace av
