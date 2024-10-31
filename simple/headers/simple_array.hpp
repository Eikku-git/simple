#pragma once

#include <assert.h>

namespace simple {

	template<typename T, size_t _size>
	struct Array {

		typedef T* Iterator;
		typedef const T* ConstIterator;

		inline size_t size() const {
			return _size;
		}

		inline T* data() const {
			return (T*)_data;
		}

		inline T& getIndexUnsafe(size_t index) {
			return _data[index];
		}

		const T& operator[](size_t index) const {
			assert(index < _size && "attempting to access array index past it's bounds!");
			return _data[index];
		}

		constexpr T& operator[](size_t index) {
			assert(index < _size && "attempting to access array index past it's bounds!");
			return _data[index];
		}

		constexpr inline Iterator begin() const noexcept {
			return &_data[0];
		}

		constexpr inline ConstIterator end() const noexcept {
			return &_data[_size];
		}

		T _data[_size];
	};
}
