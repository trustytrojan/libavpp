#pragma once

#include "CodecContext.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace av
{

class Decoder : public CodecContext
{
	AVFrame *_frm = nullptr;

public:
	Decoder(const AVCodec *const codec = NULL)
		: CodecContext(codec) {}

	~Decoder()
	{
		av_frame_free(&_frm);
	}

	/**
	 * Sends a packet to the decoder.
	 * @param packet can be `NULL` (or an `AVPacket` with data set to `NULL` and size set to `0`);
	 * in this case, it is considered a flush packet, which signals the end of the stream.
	 * @return whether the send was successful
	 * @throws `av::Error` if any `avcodec_*` functions fail
	 * @note `true` is also returned in the case that the decoder has output frames to return to the caller.
	 */
	bool send_packet(const AVPacket *const pkt)
	{
		switch (const auto rc = avcodec_send_packet(_cdctx, pkt))
		{
		case 0:
		case AVERROR(EAGAIN):
			return true;
		case AVERROR_EOF:
			return false;
		default:
			throw Error("avcodec_send_packet", rc);
		}
	}

	/**
	 * Receive a frame from the decoder.
	 * For audio, this method should be called in a loop since
	 * one packet can yield several `AVFrame`s of audio.
	 * @retval On success, a pointer to the internal `AVFrame`.
	 * @retval `NULL` if either the codec requires a new packet to be sent, or the codec has been fully flushed.
	 * @throws `av::Error` if any `avcodec_*` functions fail
	 * @warning **Do not free/delete the returned pointer.** It belongs to and is managed by this class.
	 */
	const AVFrame *receive_frame()
	{
		if (!_frm && !(_frm = av_frame_alloc()))
			throw std::runtime_error("av_frame_alloc() failed");
		switch (const auto rc = avcodec_receive_frame(_cdctx, _frm))
		{
		case 0:
			return _frm;
		case AVERROR(EAGAIN):
		case AVERROR_EOF:
			return NULL;
		default:
			throw Error("avcodec_receive_frame", rc);
		}
	}
};

} // namespace av
