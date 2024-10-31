#pragma once

#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include <assert.h>
#include <cstdint>
#include <filesystem>

namespace simple {

	template<typename Allocator = DynamicAllocator<char>>
	struct String { 
	public:

		typedef char* Iterator;
		typedef const char* ConstIterator;

		inline String() noexcept : _capacity(0), _length(0), _pData(nullptr) {}

		inline String(const String& other) : _capacity(other._capacity), _length(other._length), _pData(nullptr) {
			if (other._capacity != 0) {
				_pData = getAllocator().allocate(other._capacity);
				for (size_t i = 0; i < other._length; i++) {
					_pData[i] = other._pData[i];
				}
				_pData[_length] = '\0';
			}
		}

		inline String(String&& other) noexcept : _capacity(other._capacity), _length(other._length), _pData(other._pData) {
			other._capacity = 0;
			other._length = 0;
			other._pData = nullptr;
		}

		inline String(size_t length) : _capacity(length + 1), _length(length), _pData(nullptr) {
			_pData = getAllocator().allocate(_capacity);
			_pData[_length] = '\0';
		}

		inline String(char* buffer, size_t length) : _capacity(0), _length(0), _pData(nullptr) {
			newString(buffer, length);
		}

		inline String(const char* text) : _capacity(0), _length(0), _pData(nullptr) {
			newString(text);
		}

		inline String(const char* text, size_t begin, size_t end) : _capacity(0), _length(0), _pData(nullptr) {
			size_t length = 0;
			for (size_t i = begin; i < end; i++) {
				if (text[i] == '\0') {
					end = i;
					break;
				}
				length++;
			}
			reserve(length + 1);
			for (size_t i = begin; i < end; i++) {
				append(text[i]);
			}
		}

		inline bool empty() {
			return !_length;
		}

		inline Allocator getAllocator() {
			return Allocator();
		}

		inline size_t capacity() const {
			return _capacity;
		}

		inline size_t length() const {
			return _length;
		}

		inline const char* data() const {
			return _pData;
		}

		inline void reserve(size_t capacity) {
			if (capacity <= _capacity) {
				return;
			}
			char* temp = getAllocator().allocate(capacity);
			for (size_t i = 0; i < _length; i++) {
				temp[i] = _pData[i];
			}
			getAllocator().deallocate(_pData, 1);
			_pData = temp;
			_pData[_length] = '\0';
			_capacity = capacity;
		}

		inline void newString(const char* constChar) {
			getAllocator().deallocate(_pData, 1);
			int i = 0;
			while (constChar[i] != '\0') {
				i++;
			}
			_length = i;
			_capacity = _length + 1;
			_pData = getAllocator().allocate(_length + 1);
			for (int j = 0; j < _length; j++) {
				_pData[j] = constChar[j];
			}
			_pData[_length] = '\0';
		}

		void newString(char* buffer, size_t length) {
			getAllocator().deallocate(_pData, 1);
			_pData = getAllocator().allocate(length + 1);
			for (int i = 0; i < length; i++) {
				_pData[i] = buffer[i];
			}
			_pData[length] = '\0';
			_length = length;
			_capacity = _length + 1;
		}

		inline String& append(char c) {
			if (_capacity - _length < 2) {
				reserve(_capacity ? _capacity + 1 : 2);
			}
			_pData[_length++] = c;
			_pData[_length] = '\0';
			return *this;
		}

		inline String& append(const String& other) {
			if (_length + other._length + 1 > _capacity) {
				reserve(_length + other._length + 1);
			}
			for (size_t i = 0; i < other._length; i++) {
				_pData[_length + i] = other._pData[i];
			}
			_length += other._length;
			_pData[_length] = '\0';
			return *this;
		}

		inline String& append(const char* cStr) {
			size_t cStrLength = 0;
			while (cStr[cStrLength] != '\0') {
				cStrLength++;
			}
			if (_length + cStrLength > _capacity) {
				reserve(_length + cStrLength + 1);
			}
			for (size_t i = 0; i < cStrLength; i++) {
				_pData[_length + i] = cStr[i];
			}
			_length += cStrLength;
			_pData[_length] = '\0';
			return *this;
		}

		inline String subString(size_t begin, size_t end) const {
			assert(begin < end && begin < _length && end <= _length && "invalid sub string arguments!");
			String result(end - begin);
			for (size_t i = begin; i < end; i++) {
				result._pData[i - begin] = _pData[i];
			}
			result._pData[end - begin] = '\0';
			return result;
		}

		inline void clear() {
			_capacity = 0;
			_length = 0;
			getAllocator().deallocate(_pData, 1);
			_pData = nullptr;
		}

		Iterator begin() const noexcept {
			return &_pData[0];
		}

		ConstIterator end() const noexcept {
			return &_pData[_length];
		}

		inline char& operator[](size_t index) const {
			return _pData[index];
		}

		template<class Element, class Traits>
		friend inline std::basic_istream<Element, Traits>& operator>>(
			std::basic_istream<Element, Traits>& istream, String<Allocator>& string) {

			using Istream = std::basic_istream<Element, Traits>;
			using ctype = typename Istream::_Ctype;

			typename Istream::iostate state = Istream::goodbit;
			const typename Istream::sentry ok(istream);
			bool changed = false;

			if (ok) {
				const ctype& ctypeFacet = std::use_facet<ctype>(istream.getloc());
				size_t size{};
				if (istream.width() > 0) {
					size = istream.width();
				}
				else {
					size = SIZE_MAX;
				}
				string.clear();
				char character = istream.rdbuf()->sgetc();
				for (; size > 0; --size, character = istream.rdbuf()->snextc()) {
					if (Traits::eq_int_type(Traits::eof(), character)) {
						state |= Istream::eofbit;
						break;
					}
					else if (ctypeFacet.is(ctype::space, Traits::to_char_type(character))) {
						break;
					}
					else {
						string.append(Traits::to_char_type(character));
						changed = true;
					}
				}
			}
			istream.width(0);
			if (!changed) {
				state |= Istream::failbit;
			}
			istream.setstate(state);
			return istream;
		}	

		inline bool operator==(const String& other) const {
			if (!_pData || !other._pData) {
				return false;
			}
			return !strcmp(_pData, other._pData);
		}

		inline bool operator==(const char* other) const {
			if (!_pData) {
				return false;
			}
			return !strcmp(_pData, other);
		}

		inline String& operator=(const String& other) {
			getAllocator().deallocate(_pData, 1);
			_length = other._length;
			_capacity = _length + 1;
			_pData = getAllocator().allocate(_capacity);
			for (int i = 0; i < _length; i++) {
				_pData[i] = other._pData[i];
			}
			_pData[_length] = '\0';
			return *this;
		}

		inline bool operator<(const String& other) const {
			if (!_pData || !other._pData) {
				return false;
			}
			size_t i = 0;
			while ((_pData[i] != '\0' || other._pData[i] != '\0') && _pData[i] == other._pData[i]) {
				i++;
			}
			return _pData[i] < other._pData[i];
		}

		inline friend String operator+(const String& a, const String& b) {
			String result{};
			result.reserve(a._length + b._length);
			result.append(a).append(b);
			return result;
		}

		inline friend String operator+(const String& a, const char* b) {
			size_t bLength = SIZE_MAX;
			for (size_t i = 0; i < bLength; i++) {
				if (b[i] == '\0') {
					bLength = i;
					break;
				}
			}
			simple::String<> result{};
			result.reserve(a.length() + bLength);
			for (char c : a) {
				result.append(c);
			}
			for (size_t i = 0; i < bLength; i++) {
				result.append(b[i]);
			}
			return result;
		}

		inline friend String operator+(const char* a, const String& b) {
			size_t aLength = SIZE_MAX;
			for (size_t i = 0; i < aLength; i++) {
				if (a[i] == '\0') {
					aLength = i;
					break;
				}
			}
			simple::String<> result{};
			result.reserve(aLength + b.length());
			for (size_t i = 0; i < aLength; i++) {
				result.append(a[i]);
			}
			for (char c : b) {
				result.append(c);
			}
			return result;
		}

		struct Hash {
			inline uint64_t operator()(const String& string) const {
				uint64_t h = 37;
				for (char c : string) {
					h = (h * 54059) ^ ((uint64_t)c * 76963);
				}
				return h;
			}
		};

		inline ~String() {
			_capacity = 0;
			_length = 0;
			getAllocator().deallocate(_pData, 1);
			_pData = nullptr;
		}

	private:
		size_t _capacity;
		size_t _length;
		char* _pData;
	};

	inline simple::String<> toString(uint64_t unsignedInt) {
		char buffer[21];
		buffer[20] = '0' + unsignedInt % 10;
		size_t i = 20;
		while (unsignedInt /= 10) {
			buffer[--i] = '0' + unsignedInt % 10;
		}
		return simple::String<>(buffer, i, 21);
	}
}
