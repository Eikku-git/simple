#pragma once

#include <assert.h>

namespace simple {

	template<typename T, size_t size>
	struct Array {

		typedef T* Iterator;
		typedef const T* ConstIterator;

		constexpr inline size_t Size() const {
			return size;
		}

		inline T* Data() const {
			return (T*)_data;
		}

		inline T& GetIndexUnsafe(size_t index) {
			return _data[index];
		}

		constexpr inline T& operator[](size_t index) const {
			assert(index < size && "attempting to access array index past it's bounds!");
			return *(T*)(&_data[index]);
		}

		inline Iterator begin() const noexcept {
			return (T*)&_data[0];
		}

		constexpr inline ConstIterator end() const noexcept {
			return &_data[size];
		}

		T _data[size];
	};
}
