#pragma once

extern "C"
{
#include <libavformat/avformat.h>
}

namespace av
{

class FormatContext
{
protected:
	AVFormatContext *_fmtctx{};
	FormatContext() = default;

public:
	~FormatContext() { avformat_free_context(_fmtctx); }

	FormatContext(const FormatContext &) = delete;
	FormatContext &operator=(const FormatContext &) = delete;

	FormatContext(FormatContext &&other) noexcept
	{
		_fmtctx = other._fmtctx;
		other._fmtctx = nullptr;
	}

	FormatContext &operator=(FormatContext &&other) noexcept
	{
		if (this != &other)
		{
			avformat_free_context(_fmtctx);
			_fmtctx = other._fmtctx;
			other._fmtctx = nullptr;
		}
		return *this;
	}

	/**
	 * @return A pointer to the internal `AVFormatContext`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by this class.
	 */
	operator AVFormatContext *() const { return _fmtctx; }

	/**
	 * @brief Access fields of the internal `AVFormatContext`.
	 * @return A pointer to the internal `AVFormatContext`.
	 * @warning **Do not free/delete the returned pointer.**
	 * It belongs to and is managed by this class.
	 */
	AVFormatContext *operator->() const { return _fmtctx; }

	/**
	 * @param index User-defined input/output number. This is printed as `Output
	 * #0, mp4, ...` if `index` is `0`.
	 */
	void print_info(int index)
	{
		av_dump_format(_fmtctx, index, _fmtctx->url, (bool)_fmtctx->oformat);
	}
};

} // namespace av
