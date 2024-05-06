#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace av
{
	class Decoder
	{
		AVCodecContext *_cdctx = nullptr;
		AVFrame *_frm = nullptr;

	public:
		/**
		 * @param codec non-`NULL` codec to initialize decoder with
		 * @throws `std::invalid_argument` if `codec` is `NULL`
		 * @throws `std::runtime_error` if either `avcodec_alloc_context3` or `av_frame_alloc` fails
		 */
		Decoder(const AVCodec *const codec)
		{
			if (!codec)
				throw std::invalid_argument("codec is null");
			if (!(_cdctx = avcodec_alloc_context3(codec)))
				throw std::runtime_error("avcodec_alloc_context3() failed");
		}

		~Decoder()
		{
			av_frame_free(&_frm);
			avcodec_free_context(&_cdctx);
		}

		Decoder(const Decoder&) = delete;
		Decoder& operator=(const Decoder&) = delete;
		Decoder(Decoder&&) = delete;
		Decoder& operator=(Decoder&&) = delete;

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

		/**
		 * Sends a packet to the decoder.
		 * @param packet non-`NULL` pointer to packet to send to decoder.
		 * @return whether the send was successful
		 * @note `true` is also returned in the case that the decoder has
		 * output frames to return to the caller.
		 * @throws `std::invalid_argument` if `pkt` is `NULL`
		 * @throws `std::runtime_error` if either the codec context is not open,
		 * or the packet's stream index does not match the decoder's stream index
		 * @throws `av::Error` if any `avcodec_*` functions fail
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
		 * @throws `std::runtime_error` if the codec context is not open
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
}
