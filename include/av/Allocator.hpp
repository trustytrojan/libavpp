#pragma once

#include <new>

extern "C"
{
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
}

namespace av
{
	template <typename _Tp>
	class Allocator
	{
	public:
		// necessary for stl
		using value_type = _Tp;

		_Tp *allocate(std::size_t n)
		{
			if (const auto p = static_cast<_Tp *>(av_malloc(n * sizeof(_Tp))))
				return p;
			throw std::bad_alloc();
		}

		void deallocate(_Tp *p, std::size_t) noexcept
		{
			av_free(p);
		}
	};
}
