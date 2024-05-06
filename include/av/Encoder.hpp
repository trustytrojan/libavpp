#pragma once

#include <stdexcept>
#include "Error.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace av
{
	class Encoder
	{
		AVCodecContext *_cdctx = nullptr;
		AVPacket *_pkt = nullptr;

	public:
		Encoder(const AVCodec *const codec)
		{
			if (!codec)
				throw std::invalid_argument("codec is null");
			if (!(_cdctx = avcodec_alloc_context3(codec)))
				throw std::runtime_error("avcodec_alloc_context3() failed");
		}

		~Encoder()
		{
			av_packet_free(&_pkt);
			avcodec_free_context(&_cdctx);
		}

		Encoder(const Encoder &) = delete;
		Encoder &operator=(const Encoder &) = delete;
		Encoder(Encoder &&) = delete;
		Encoder &operator=(Encoder &&) = delete;

		/**
		 * Access the internal `AVCodecContext`.
		 * @return A pointer to the internal `AVCodecContext`.
		 * @warning Only modify what you need for calling `open`.
		 */
		AVCodecContext *operator->() { return _cdctx; }

		/**
		 * Access the internal `AVCodecContext`.
		 * @return A read-only pointer to the internal `AVCodecContext`.
		 */
		const AVCodecContext *operator->() const { return _cdctx; }

		/**
		 * Opens the internal `AVCodecContext` for decoding.
		 * @throws `av::Error` if `avcodec_open2` fails
		 */
		void open()
		{
			if (const auto rc = avcodec_open2(_cdctx, NULL, NULL); rc < 0)
				throw Error("avcodec_open2", rc);
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

		const AVPacket *receive_packet()
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
}