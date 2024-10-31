#pragma once

#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "tuple.hpp"
#include "engine_logger.hpp"

namespace simple {

	template<typename T, typename Hasher, uint8_t MaxBucketSize = 4,
		typename ElementAllocator = DynamicAllocator<T>, 
		typename BucketAllocator = DynamicAllocator<Tuple<Array<Tuple<T, bool>, MaxBucketSize>, uint8_t>>>
	class Set {
	public:

		typedef Tuple<T, bool> Element;
		typedef Tuple<Array<Element, MaxBucketSize>, uint8_t> Bucket;

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
				_ptr = other._ptr;
				_parent = other._parent;
				return *this;
			}

			inline T& operator*() const {
				return (*_ptr)->first;
			}
		};

		inline Set() : _capacity(128), _buckets(nullptr), _elements(), _trash(0) {
			_buckets = getBucketAllocator().allocate(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				getBucketAllocator().construct(&_buckets[i]);
			}
			_elements.reserve(_capacity);
		}

		inline Set(Set&& other) noexcept
			: _capacity(other._capacity), _buckets(other._buckets), _elements(std::move(other._elements)), 
			_trash(other._trash)  {
			other._capacity = 0;
			other._buckets = nullptr;
		}
		
		constexpr inline ElementAllocator getValueAllocator() noexcept {
			return ElementAllocator();
		}

		constexpr inline BucketAllocator getBucketAllocator() noexcept {
			return BucketAllocator();
		}

		void cleanup() {
			for (auto iter = _elements.begin(); iter != _elements.end();) {
				if (!(*iter)->second) {
					iter = _elements.erase(iter);
					continue;
				}
				++iter;
			}
			_trash = 0;
		}

		inline Tuple<bool, T*> insert(const T& value) {
			if (_trash > 8) {
				cleanup();
			}
			uint64_t hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const Element& element : bucket.first) {
					if (element.first == value) {
						return { false, nullptr };
					}
				}
				simple_engine::logMessage("bad hash!");
				if (bucket.second >= MaxBucketSize - 1) {
					return { false, nullptr };
				}
			}
			bucket.first[bucket.second].first = value;
			bucket.first[bucket.second].second = true;
			Element* element = *_elements.pushBack(&bucket.first[bucket.second++]);
			if ((float)_elements.size() / _capacity >= 0.8f) {
				reserve(_capacity * 2);
			}
			return { true, &element->first } ;
		}

		template<typename... Args>
		inline Tuple<bool, T*> emplace(Args&&... args) {
			if (_trash > 8) {
				cleanup();
			}
			T value(std::forward<Args>(args)...);
			uint64_t hash = Hasher()(value);
			Bucket& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const Element& element : bucket.first) {
					if (element.first == value) {
						return { false, nullptr };
					}
				}
				simple_engine::logMessage("bad hash!");
				if (bucket.second >= MaxBucketSize - 1) {
					return { false, nullptr };
				}
			}
			getValueAllocator().construct(&bucket.first[bucket.second].first, std::move(value));
			bucket.first[bucket.second].second = true;
			_elements.pushBack(&bucket.first[bucket.second]);
			if ((float)_elements.size() / _capacity >= 0.8f) {
				reserve(_capacity * 2);
			}
			return { true, &bucket.first[bucket.second++].first };
		}

		inline bool erase(const T& value) {
			uint64_t hash = Hasher()(value);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (Element& element : bucket.first) {
					if (element.first == value) {
						getValueAllocator().destroy(&element.first);
						element.second = false;
						bucket.second--;
						_trash++;
						return true;
					}
				}
			}
			return false;
		}

		inline bool contains(const T& value) {
			uint64_t hash = Hasher()(value);
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

		inline const T* find(const T& value) {
			uint64_t hash = Hasher()(value);
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

		size_t size() {
			return _elements.size() - _trash;
		}

		void reserve(uint32_t capacity) {
			if (capacity <= _capacity) {
				return;
			}
			uint32_t oldCapacity = _capacity;
			while (capacity > _capacity) {
				_capacity *= 2;
			}
			Bucket* temp = _buckets;
			_buckets = getBucketAllocator().allocate(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				getBucketAllocator().construct(&_buckets[i]);
			}
			_elements.clear();
			_elements.reserve(_capacity);
			for (size_t i = 0; i < oldCapacity; i++) {
				for (size_t j = 0; j < temp[i].second; j++) {
					emplace(std::move(temp[i].first[j].first));
				}
			}
			getBucketAllocator().deallocate(temp, 1);
		}


		inline Iterator begin() {
			Element** ptr = _elements.begin();
			if (!ptr || !_elements.size()) {
				return Iterator(ptr, _elements);
			}
			while (ptr != _elements.end() && !(*ptr)->second) {
				++ptr;
			}
			return Iterator(ptr, _elements);
		}

		inline const Iterator end() {
			return Iterator((Element**)_elements.end(), _elements);
		}

		~Set() {
			for (T& value : *this) {
				getValueAllocator().destroy(&value);
			}
			getBucketAllocator().deallocate(_buckets, 1);
			_buckets = nullptr;
		}

	private:

		uint32_t _capacity;
		Bucket* _buckets;
		simple::DynamicArray<Element*> _elements;
		uint32_t _trash;
	};
}
