#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
}

#include "Error.hpp"

namespace av
{

inline const AVCodec *find_encoder_by_name(const char *const name)
{
	if (const auto codec = avcodec_find_encoder_by_name(name))
		return codec;
	throw Error("avcodec_find_encoder_by_name", AVERROR_ENCODER_NOT_FOUND);
}

inline void link_filters(
	AVFilterContext *src,
	unsigned src_pad,
	AVFilterContext *dst,
	unsigned dst_pad)
{
	if (const int rc = avfilter_link(src, src_pad, dst, dst_pad); rc < 0)
		throw Error("avfilter_link", rc);
}

/**
 * An iterator for `av_hwdevice_iterate_types`.
 * Allows for using a range-based for loop to iterate over available hardware
 * device types.
 */
class HWDeviceTypeIterator
{
	AVHWDeviceType type;

public:
	using iterator_category = std::input_iterator_tag;
	using value_type = AVHWDeviceType;
	using difference_type = std::ptrdiff_t;
	using pointer = const value_type *;
	using reference = const value_type &;

	HWDeviceTypeIterator(const AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE)
		: type(type)
	{
	}

	reference operator*() const { return type; }

	HWDeviceTypeIterator &operator++()
	{
		type = av_hwdevice_iterate_types(type);
		return *this;
	}

	HWDeviceTypeIterator operator++(int)
	{
		HWDeviceTypeIterator tmp = *this;
		++(*this);
		return tmp;
	}

	bool operator==(const HWDeviceTypeIterator &other) const
	{
		return type == other.type;
	}

	bool operator!=(const HWDeviceTypeIterator &other) const
	{
		return !(*this == other);
	}
};

/**
 * A range-based for loop compatible wrapper for `av_hwdevice_iterate_types`.
 *
 * Example usage:
 * @code
 * for (const auto type : av::for_each_hwdevice_type())
 * {
 *     std::cout << av_hwdevice_get_type_name(type) << '\n';
 * }
 * @endcode
 */
class HWDeviceTypes
{
public:
	HWDeviceTypeIterator begin() const
	{
		return HWDeviceTypeIterator{
			av_hwdevice_iterate_types(AV_HWDEVICE_TYPE_NONE)};
	}
	HWDeviceTypeIterator end() const { return HWDeviceTypeIterator{}; }
};

} // namespace av
