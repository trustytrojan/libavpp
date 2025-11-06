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

inline const AVFilter *filter_get_by_name(const char *const name)
{
	if (const auto filter = avfilter_get_by_name(name))
		return filter;
	throw Error("avfilter_get_by_name", AVERROR_FILTER_NOT_FOUND);
}

/**
 * A range-based for loop compatible wrapper for `av_hwdevice_iterate_types`.
 *
 * Example usage:
 * @code
 * for (const auto type : av::HWDeviceTypes{})
 * {
 *     std::cout << av_hwdevice_get_type_name(type) << '\n';
 * }
 * @endcode
 */
class HWDeviceTypes
{
	class Iterator
	{
		AVHWDeviceType type;

	public:
		Iterator(const AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE)
			: type(type)
		{
		}

		AVHWDeviceType operator*() const { return type; }

		Iterator &operator++()
		{
			type = av_hwdevice_iterate_types(type);
			return *this;
		}

		bool operator==(const Iterator &other) const
		{
			return type == other.type;
		}
	};

public:
	Iterator begin() const
	{
		return Iterator{av_hwdevice_iterate_types(AV_HWDEVICE_TYPE_NONE)};
	}

	Iterator end() const { return Iterator{}; }
};

} // namespace av
