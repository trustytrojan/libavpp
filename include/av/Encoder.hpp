#pragma once

#include "CodecContext.hpp"
#include "Error.hpp"
#include <stdexcept>

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace av
{

class Encoder : public CodecContext
{
	AVPacket *_pkt = nullptr;

public:
	Encoder(const AVCodec *const codec = NULL)
		: CodecContext(codec) {}

	~Encoder()
	{
		av_packet_free(&_pkt);
	}

	bool send_frame(const AVFrame *const frm)
	{
		switch (const auto rc = avcodec_send_frame(_cdctx, frm))
		{
		case 0:
		case AVERROR(EAGAIN):
			return true;
		case AVERROR_EOF:
			return false;
		default:
			throw Error("avcodec_send_frame", rc);
		}
	}

	AVPacket *receive_packet()
	{
		if (!_pkt && !(_pkt = av_packet_alloc()))
			throw std::runtime_error("av_packet_alloc() failed");
		switch (const auto rc = avcodec_receive_packet(_cdctx, _pkt))
		{
		case 0:
			return _pkt;
		case AVERROR(EAGAIN):
		case AVERROR_EOF:
			return NULL;
		default:
			throw Error("avcodec_receive_packet", rc);
		}
	}
};

} // namespace av
