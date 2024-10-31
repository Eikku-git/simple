#pragma once

#include "simple_allocator.hpp"
#include <assert.h>

namespace simple {

	template<typename T, class Allocator = DynamicAllocator<T>>
	class DynamicArray {
	public:

		typedef T* Iterator;
		typedef const T* ConstIterator;

		inline DynamicArray() : _capacity(0), _size(0), _pData(nullptr) {}

		inline DynamicArray(const DynamicArray& other) noexcept 
			: _capacity(other._capacity), _size(other._size), _pData(nullptr) {
			if (other._capacity == 0) {
				return;
			}
			_pData = getAllocator().allocate(_capacity);
			for (size_t i = 0; i < other._size; i++) {
				getAllocator().construct(&_pData[i], other._pData[i]);
			}
		}

		inline DynamicArray(DynamicArray&& other) noexcept 
			: _capacity(other._capacity), _size(other._size), _pData(other._pData) {
			other._capacity = 0;
			other._size = 0;
			other._pData = nullptr;
		}

		inline DynamicArray(size_t size) {
			_capacity = 1;
			_size = size;
			while (_capacity < _size) {
				_capacity *= 2;
			}
			_pData = getAllocator().allocate(_capacity);
			for (uint32_t i = 0; i < _size; i++) {
				getAllocator().construct(&_pData[i]);
			}
		}

		constexpr inline Allocator getAllocator() noexcept {
			return Allocator();
		}

		constexpr inline size_t capacity() const noexcept {
			return _capacity;
		}

		constexpr inline size_t size() const noexcept {
			return _size;
		}

		constexpr inline T* data() const noexcept {
			return _pData;
		}

		inline DynamicArray& reserve(uint32_t capacity) {
			if (capacity <= _capacity || capacity == 0) {
				return *this;
			}
			_capacity = _capacity ? _capacity : 1;
			while (_capacity < capacity) {
				_capacity *= 2;
			}
			T* temp = getAllocator().allocate(_capacity);
			assert(temp && "failed to allocate memory!");
			for (size_t i = 0; i < _size; i++) {
				getAllocator().construct(&temp[i], std::move(_pData[i]));
			}
			getAllocator().deallocate(_pData, 1);
			_pData = temp;
			return *this;
		}

		inline DynamicArray& resize(uint32_t size) {
			uint32_t newCapacity = _capacity ? _capacity : 1;
			while (size > newCapacity) {
				newCapacity *= 2;
			}
			reserve(newCapacity);
			_size = size;
			for (size_t i = 0; i < _size; i++) {
				getAllocator().construct(&_pData[i]);
			}
			return *this;
		}

		inline Iterator pushBack(const T& value) {
			if (_size >= _capacity) {
				reserve((_capacity ? _capacity : 1) * 2);
			}
			getAllocator().construct(&_pData[_size]);
			_pData[_size++] = value;
			return back();
		}

		inline Iterator insert(Iterator where, const T& value) {
			if (where == &_pData[_size]) {
				return pushBack(value);
			}
			if (_size >= _capacity) {
				reserve((_capacity ? _capacity : 1) * 2);
			}
			for (auto iter = &_pData[_size]; iter != where; iter--) {
				getAllocator().construct(iter, std::move(*(iter - 1)));
			}
			getAllocator().construct(where, value);
			++_size;
			return where;
		}

		template<typename... Args>
		inline Iterator emplaceBack(Args&&... args) {
			if (_size >= _capacity) {
				reserve((_capacity ? _capacity : 1) * 2);
			}
			getAllocator().construct(&_pData[_size++], std::forward<Args>(args)...);
			return back();
		}

		template<typename... Args>
		inline Iterator emplace(Iterator where, Args&&... args) {
			if (where == &_pData[_size]) {
				return emplaceBack(std::forward<Args>(args)...);
			}
			if (_size >= _capacity) {
				reserve((_capacity ? _capacity : 1) * 2);
			}
			for (auto iter = &_pData[_size]; iter != where; iter--) {
				getAllocator().construct(iter, std::move(*(iter - 1)));
			}
			getAllocator().construct(where, std::forward<Args>(args)...);
			++_size;
			return where;
		}

		inline Iterator erase(Iterator where) {
			Iterator iter = where;
			size_t index = iter - &_pData[0];
			if (iter == &_pData[_size]) {
			}
			while (iter != &_pData[_size - 1]) {
				getAllocator().destroy(iter);
				getAllocator().construct(iter, std::move(*(iter + 1)));
				++iter;
			}
			--_size;
			return &_pData[index];
		}

		inline Iterator back() {
			return &_pData[_size - 1];
		}

		inline void clear() {
			for (size_t i = 0; i < _size; i++) {
				getAllocator().destroy(&_pData[i]);
			}
			_capacity = 0;
			_size = 0;
			getAllocator().deallocate(_pData, 1);
			_pData = nullptr;
		}

		inline Iterator begin() const {
			return &_pData[0];
		}

		inline ConstIterator end() const {
			return &_pData[_size];
		}

		inline DynamicArray& reverse() {
			if (!_size) {
				return *this;
			}
			for (uint32_t i = 0, j = _size - 1; i != j && i < j;) {
				T temp(std::move(_pData[i]));
				getAllocator().construct(&_pData[i], std::move(_pData[j]));
				getAllocator().construct(&_pData[j], std::move(temp));
				++i; --j;
			}
			return *this;
		}

		~DynamicArray() noexcept {
			for (size_t i = 0; i < _size; i++) {
				getAllocator().destroy(&_pData[i]);
			}
			_capacity = 0;
			_size = 0;
			getAllocator().deallocate(_pData, 1);
			_pData = nullptr;
		}

		T& operator[](size_t index) const {
			assert(index < _size && "attempting to access index that's out side the bounds of dyn_array!");
			return _pData[index];
		}

		DynamicArray& operator=(const DynamicArray& other) {
			_capacity = other._capacity;
			_size = other._size;
			_pData = getAllocator().allocate(_capacity);
			for (uint32_t i = 0; i < _size; i++) {
				getAllocator().construct(&_pData[i], other._pData[i]);
			}
			return *this;
		}	

	private:
		uint32_t _capacity;
		uint32_t _size;
		T* _pData;
	};
}
