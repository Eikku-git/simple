#pragma once

#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "simple_pair.hpp"
#include "simple_tuple.hpp"
#include "simple_logging.hpp"
#include <cstdint>
#include <utility>

namespace simple {

	template<typename T, typename Hasher, uint8_t T_max_bucket_size = 4,
		typename Allocator = DynamicAllocator<Pair<Array<Pair<T, bool>, T_max_bucket_size>, uint8_t>>>
	class Set {
	public:

		typedef size_t HashType;
		typedef uint8_t BucketSize;
		typedef Pair<T, bool> Element;
		typedef Pair<Array<Element, T_max_bucket_size>, BucketSize> Bucket;

		struct Iterator {	

			Element** _ptr;
			simple::DynamicArray<Element*>& _parent;

			Iterator(Element** ptr, simple::DynamicArray<Element*>& parent) noexcept 
				: _ptr(ptr), _parent(parent) {
			}

			inline void operator++() {
				++_ptr;
				while (_ptr != _parent.end() && !(**_ptr).second) {
					++_ptr;
				}
			}

			inline void operator--() {
				--_ptr;
				while (!_ptr->second && _ptr != _parent.begin()) {
					--_ptr;
				}
			}

			inline bool operator==(const Iterator& other) const {
				return _ptr == other._ptr;
			}

			inline Iterator& operator=(const Iterator& other) {
				_ptr = other._ptr;size_t
				_parent = other._parent;
				return *this;
			}

			inline T& operator*() const {
				return (*_ptr)->first;
			}
		};

		typedef const Iterator ConstIterator;

		inline Set() : _capacity(128), _buckets(nullptr), _elements(), _trash(0) {
			_buckets = Allocator().allocate(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				Allocator().construct(&_buckets[i]);
			}
			_elements.Reserve(_capacity);
		}

		inline Set(Set&& other) noexcept
			: _capacity(other._capacity), _buckets(other._buckets), _elements(std::move(other._elements)), 
			_trash(other._trash)  {
			other._capacity = 0;
			other._buckets = nullptr;
		}

		void Cleanup() {
			for (auto iter = _elements.begin(); iter != _elements.end();) {
				if (!(*iter)->second) {
					iter = _elements.Erase(iter);
					continue;
				}
				++iter;
			}
			_trash = 0;
		}

		inline void Reserve(uint32_t capacity) {
			if (capacity <= _capacity) {
				return;
			}
			uint32_t oldCapacity = _capacity;
			while (capacity > _capacity) {
				_capacity *= 2;
			}
			Bucket* temp = _buckets;
			_buckets = Allocator().allocate(_capacity);
			for (uint32_t i = 0; i < _capacity; i++) {
				Allocator().construct(&_buckets[i]);
			}
			_elements.Clear();
			_elements.Reserve(_capacity);
			for (uint32_t i = 0; i < oldCapacity; i++) {
				for (BucketSize j = 0; j < temp[i].second; j++) {
					Emplace(std::move(temp[i].first[j].first));
				}
			}
			Allocator().deallocate(temp, 1);
		}

		inline Tuple<bool, T*> Insert(const T& value) {
			HashType hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const Element& element : bucket.first) {
					if (element.first == value) {
						return { false, nullptr };
					}
				}
				simple::logMessage(this, "bad hash (function simple::Set::Insert)!");
				if (bucket.second >= T_max_bucket_size - 1) {
					return { false, nullptr };
				}
			}
			bucket.first[bucket.second].first = value;
			bucket.first[bucket.second].second = true;
			if ((float)_elements.Size() / _capacity >= 0.8f) {
				Reserve(_capacity * 2);
			}
			Element* element = _elements.PushBack(&bucket.first[bucket.second++]);
			return { true, &element->first } ;
		}

		template<typename... Args>
		inline Tuple<bool, T*> Emplace(Args&&... args) {
			T value(std::forward<Args>(args)...);
			HashType hash = Hasher()(value);
			Bucket& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const Element& element : bucket.first) {
					if (element.first == value) {
						return { false, nullptr };
					}
				}
				simple::logMessage(this, "bad hash (function simple::Set::Emplace)!");
				if (bucket.second >= T_max_bucket_size - 1) {
					return { false, nullptr };
				}
			}
			new(&bucket.first[bucket.second].first) T(std::move(value));
			bucket.first[bucket.second].second = true;
			_elements.PushBack(&bucket.first[bucket.second]);
			if ((float)_elements.Size() / _capacity >= 0.8f) {
				Reserve(_capacity * 2);
			}
			return { true, &bucket.first[bucket.second++].first };
		}

		inline bool Erase(const T& value) {
			if (_trash > 8) {
				Cleanup();
			}
			HashType hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (Element& element : bucket.first) {
					if (element.first == value) {
						&element.first->~T();
						element.second = false;
						bucket.second--;
						_trash++;
						return true;
					}
				}
			}
			return false;
		}

		inline bool Contains(const T& value) {
			HashType hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second == 0) {
				return false;
			}
			for (const Element& element : bucket.first) {
				if (element.first == value) {
					return true;
				}
			}
			return false;
		}

		inline const Bucket* GetBucket(HashType hash) {
			return &_buckets[hash % _capacity];
		}

		inline const T* Find(const T& value) {
			HashType hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second == 0) {
				return nullptr;
			}
			for (const T& element : bucket.first) {
				if (element == value) {
					return &element;
				}
			}
			return nullptr;
		}

		size_t Size() {
			return _elements.size() - _trash;
		}

		inline Iterator begin() {
			Element** ptr = _elements.begin();
			if (!ptr || !_elements.Size()) {
				return Iterator(ptr, _elements);
			}
			while (ptr != _elements.end() && !(*ptr)->second) {
				++ptr;
			}
			return Iterator(ptr, _elements);
		}

		inline ConstIterator end() {
			return Iterator((Element**)_elements.end(), _elements);
		}

		~Set() {
			for (T& value : *this) {
				(&value)->~T();
			}
			Allocator().deallocate(_buckets, 1);
			_buckets = nullptr;
		}

	private:

		uint32_t _capacity;
		Bucket* _buckets;
		simple::DynamicArray<Element*> _elements;
		uint32_t _trash;
	};
}
