#pragma once

#include "Error.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{
	class FormatContext
	{
	protected:
		AVFormatContext *_fmtctx = nullptr;
		FormatContext(){};

	public:
		~FormatContext()
		{
			avformat_free_context(_fmtctx);
		}

		FormatContext(const FormatContext &) = delete;
		FormatContext &operator=(const FormatContext &) = delete;
		FormatContext(FormatContext &&) = delete;
		FormatContext &operator=(FormatContext &&) = delete;

		/**
		 * @return A pointer to the internal `AVFormatContext`.
		 * @warning **Do not free/delete the returned pointer.**
		 * It belongs to and is managed by this class.
		 */
		AVFormatContext *get() { return _fmtctx; }

		/**
		 * @return A read-only pointer to the internal `AVFormatContext`.
		 * @warning **Do not free/delete the returned pointer.**
		 * It belongs to and is managed by this class.
		 */
		const AVFormatContext *get() const { return _fmtctx; }

		/**
		 * @brief Access fields of the internal `AVFormatContext`.
		 * @return A pointer to the internal `AVFormatContext`.
		 * @warning **Do not free/delete the returned pointer.**
		 * It belongs to and is managed by this class.
		 */
		AVFormatContext *operator->() { return _fmtctx; }

		/**
		 * @brief Access fields of the internal `AVFormatContext`.
		 * @return A read-only pointer to the internal `AVFormatContext`.
		 * @warning **Do not free/delete the returned pointer.**
		 * It belongs to and is managed by this class.
		 */
		const AVFormatContext *operator->() const { return _fmtctx; }
	};
}