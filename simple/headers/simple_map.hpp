#pragma once

#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "tuple.hpp"
#include "engine_logger.hpp"

namespace simple {

	template<typename Key, typename Val, typename Hasher, uint8_t MaxBucketSize = 4,
		typename KeyValAllocator = DynamicAllocator<Tuple<Key, Val>>, 
		typename BucketAllocator = DynamicAllocator<Tuple<Array<Tuple<Key, Val>, MaxBucketSize>, uint8_t>>,
		typename KeyAllocator = DynamicAllocator<Key>>
	class Map {
	public:

		typedef Tuple<Key, Val> KeyValPair;
		typedef Tuple<Array<KeyValPair, MaxBucketSize>, uint8_t> Bucket;

		struct Iterator {

			Iterator(KeyValPair** ptr) {
				_ptr = ptr;
			}

			KeyValPair** _ptr;

			inline void operator++() {
				++_ptr;
			}

			inline void operator--() {
				return --_ptr;
			}

			inline bool operator==(const Iterator& other) const {
				return _ptr == other._ptr;
			}

			inline Iterator& operator=(const Iterator& other) {
				_ptr = other._ptr;
				return *this;
			}

			inline KeyValPair& operator*() const {
				return **_ptr;
			}
		};

		inline Map() : _capacity(0), _buckets(nullptr), _elements() {
		}

		inline Map(Map&& other) noexcept 
			: _capacity(other._capacity), _buckets(other._buckets), _elements(std::move(other._elements)) {
			other._capacity = 0;
			other._buckets = nullptr;
		}

		inline Map(const Map& other) noexcept : _capacity(other._capacity), _buckets(nullptr), _elements() {
			_buckets = getBucketAllocator().allocate(_capacity);
			_elements.reserve(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				getBucketAllocator().construct(&_buckets[i]);
				for (size_t j = 0; j < other._buckets[i].second; j++) {
					getKeyValAllocator().construct(&_buckets[i].first[j], other._buckets[i].first[j]);
					_buckets[i].second++;
					_elements.pushBack(&_buckets[i].first[j]);
				};
			}
		}

		constexpr inline KeyValAllocator getKeyValAllocator() noexcept {
			return KeyValAllocator();
		}

		constexpr inline BucketAllocator getBucketAllocator() noexcept {
			return BucketAllocator();
		}

		constexpr inline KeyAllocator getKeyAllocator() noexcept {
			return KeyAllocator();
		}

		inline Tuple<bool, KeyValPair*> insert(const KeyValPair& pair) {
			if (!_capacity || (float)size() / _capacity >= 0.8f) {
				reserve(_capacity ? _capacity * 2 : 128);
			}
			uint64_t hash = Hasher()(pair.first);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (KeyValPair& element : bucket.first) {
					if (element.first == pair.first) {
						return { false, &element };
					}
				}
				simple_engine::logMessage("bad hash!");
				if (bucket.second >= MaxBucketSize - 1) {
					return { false, nullptr };
				}
			}
			getKeyValAllocator().construct(&bucket.first[bucket.second]);
			bucket.first[bucket.second].first = pair.first;
			bucket.first[bucket.second].second = pair.second;
			_elements.pushBack(&bucket.first[bucket.second]);
			return { true, &bucket.first[bucket.second++] };
		}

		template<typename... Args>
		inline Tuple<bool, KeyValPair*> emplace(Args&&... args) {
			if (!_capacity || (float)size() / _capacity >= 0.8f) {
				reserve(_capacity ? _capacity * 2 : 128);
			}
			Key key(std::forward<Args>(args)...);
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const KeyValPair& element : bucket.first) {
					if (element.first == key) {
						return { false, nullptr };
					}
				}
				simple_engine::logMessage("bad hash!");
				if (bucket.second >= 3) {
					return { false, nullptr };
				}
			}
			getKeyValAllocator().construct(&bucket.first[bucket.second]);
			getKeyAllocator().construct(&bucket.first[bucket.second].first, std::move(key));
			_elements.pushBack(&bucket.first[bucket.second]);
			return { true, &bucket.first[bucket.second++] };
		}

		inline bool contains(const Key& key) const noexcept {
			if (!_buckets) {
				return false;
			}
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second == 0) {
				return false;
			}
			for (const KeyValPair& element : bucket.first) {
				if (element.first == key) {
					return true;
				}
			}
			return false;
		}

		inline KeyValPair* find(const Key& key) const noexcept {
			if (!_buckets) {
				return nullptr;
			}
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second == 0) {
				return nullptr;
			}
			for (KeyValPair& element : bucket.first) {
				if (element.first == key) {
					return &element;
				}
			}
			return nullptr;
		}

		constexpr size_t size() noexcept {
			return _elements.size();
		}

		void reserve(uint32_t capacity) {
			if (capacity <= _capacity) {
				return;
			}
			uint32_t oldCapacity = _capacity;
			_capacity = _capacity ? _capacity : 1;
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
					KeyValPair* pair = emplace(std::move(temp[i].first[j].first)).second;
					pair->second = temp[i].first[j].second;
				}
			}
			getBucketAllocator().deallocate(temp, 1);
		}

		inline Iterator begin() {
			return _elements.begin();
		}

		inline const Iterator end() {
			return (KeyValPair**)_elements.end();
		}

		Val* operator[](const Key& key) {
			if (!_buckets) {
				reserve(128);
			}
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (KeyValPair& element : bucket.first) {
					if (element.first == key) {
						return &element.second;
					}
				}
				simple_engine::logMessage("bad hash!");
				if (bucket.second >= MaxBucketSize - 1) {
					return nullptr;
				}
			}
			if ((float)size() / _capacity > 0.8f) {
				reserve(_capacity * 2);
			}
			getKeyValAllocator().construct(&bucket.first[bucket.second]);
			bucket.first[bucket.second].first = key;
			_elements.pushBack(&bucket.first[bucket.second]);
			return &bucket.first[bucket.second++].second;
		}

		Map& operator=(const Map& other) {
			_capacity = other._capacity;
			_buckets = getBucketAllocator().allocate(_capacity);
			_elements.reserve(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				getBucketAllocator().construct(&_buckets[i]);
				for (size_t j = 0; j < other._buckets[i].second; j++) {
					getKeyValAllocator().construct(&_buckets[i].first[j], other._buckets[i].first[j]);
					_elements.pushBack(&_buckets[i].first[j]);
				};
			}
			return *this;
		}

		~Map() {
			for (KeyValPair& element : *this) {
				getKeyValAllocator().destroy(&element);
			}
			getBucketAllocator().deallocate(_buckets, 1);
			_buckets = nullptr;
		}

	private:
		uint32_t _capacity;
		Bucket* _buckets;
		simple::DynamicArray<KeyValPair*> _elements;
	};
}
