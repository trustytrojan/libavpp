#pragma once

#include "Error.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{
	class MediaWriter
	{
		AVFormatContext *_fmtctx = nullptr;

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
		}

		~MediaWriter()
		{
			avformat_free_context(_fmtctx);
		}

		AVStream *add_stream()
		{
			if (const auto stream = avformat_new_stream(_fmtctx, NULL))
				return stream;
			throw std::runtime_error("could not add stream to format!");
		}

		/**
		 * Allocate the stream private data and write the stream header to an output media file.
		 * @retval `AVSTREAM_INIT_IN_WRITE_HEADER` On success, if the codec had not already been fully initialized in avformat_init_output().
		 * @retval `AVSTREAM_INIT_IN_INIT_OUTPUT` On success, if the codec had already been fully initialized in avformat_init_output().
		 * @throws `av::Error` if `avformat_write_header` fails
		 */
		int write_header()
		{
			const auto rc = avformat_write_header(_fmtctx, NULL);
			if (rc < 0)
				throw Error("avformat_write_header", rc);
			return rc;
		}
	};
}