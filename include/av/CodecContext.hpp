#pragma once

#include <stdexcept>
#include "Error.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace av
{
	class CodecContext
	{
	protected:
		AVCodecContext *_cdctx = nullptr;

	public:
		/**
		 * @param codec if non-`NULL`, allocate private data and initialize defaults for the given codec.
		 * It is illegal to then call `avcodec_open2()` with a different codec. If `NULL`, then the
		 * codec-specific defaults won't be initialized, which may result in suboptimal default settings
		 * (this is important mainly for encoders, e.g. libx264).
		 * @throws `std::runtime_error` if `avcodec_alloc_context3` fails
		 */
		CodecContext(const AVCodec *const codec = NULL)
		{
			if (!(_cdctx = avcodec_alloc_context3(codec)))
				throw std::runtime_error("avcodec_alloc_context3() failed");
		}

		~CodecContext()
		{
			avcodec_free_context(&_cdctx);
		}

		CodecContext(const CodecContext &) = delete;
		CodecContext &operator=(const CodecContext &) = delete;
		CodecContext(CodecContext &&) = delete;
		CodecContext &operator=(CodecContext &&) = delete;

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
		 * @return A pointer to the internal `AVCodecContext`.
		 * @warning Only modify what you need for calling `open`.
		 */
		AVCodecContext *get() { return _cdctx; }

		/**
		 * @return A read-only pointer to the internal `AVCodecContext`.
		 */
		const AVCodecContext *get() const { return _cdctx; }

		/**
		 * Fill the codec context based on the values from the supplied codec parameters.
		 * Any allocated fields in codec that have a corresponding field in par are freed
		 * and replaced with duplicates of the corresponding field in par. Fields in codec
		 * that do not have a counterpart in par are not touched.
		 * @throws `av::Error` if `avcodec_parameters_to_context` fails
		 */
		void copy_params(const AVCodecParameters *const cdpar)
		{
			if (const auto rc = avcodec_parameters_to_context(_cdctx, cdpar); rc < 0)
				throw Error("avcodec_parameters_to_context", rc);
		}

		/**
		 * @param codec  The codec to open this context for. If a non-`NULL` codec
		 * has been previously passed to `avcodec_alloc_context3()` or for this context,
		 * then this parameter MUST be either `NULL` or equal to the previously passed codec.
		 * @param options A dictionary filled with `AVCodecContext` and codec-private options,
		 * which are set on top of the options already set in avctx, can be `NULL`.
		 * On return this object will be filled with options that were not found in the avctx codec context.
		 * @throws `av::Error` if `avcodec_open2` fails
		 */
		void open(const AVCodec *codec = NULL, AVDictionary **options = NULL)
		{
			if (const auto rc = avcodec_open2(_cdctx, codec, options); rc < 0)
				throw Error("avcodec_open2", rc);
		}
	};
}