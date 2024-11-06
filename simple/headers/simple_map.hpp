#pragma once

#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "simple_tuple.hpp"
#include "simple_pair.hpp"
#include "simple_logging.hpp"

namespace simple {

	template<typename Key, typename Val, typename Hasher, uint8_t T_max_bucket_size = 4,
		typename BucketAllocator = DynamicAllocator<Pair<Array<Pair<Key, Val>, T_max_bucket_size>, uint8_t>>>
	class Map {
	public:

		typedef Pair<Key, Val> KeyValPair;
		typedef Pair<Array<KeyValPair, T_max_bucket_size>, uint8_t> Bucket;

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
			: _allocator(), _capacity(other._capacity), _buckets(other._buckets), _elements(std::move(other._elements)) {
			other._capacity = 0;
			other._buckets = nullptr;
		}

		inline Map(const Map& other) noexcept : _allocator(), _capacity(other._capacity), _buckets(nullptr), _elements() {
			_buckets = _allocator.allocate(_capacity);
			_elements.reserve(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				_allocator.construct(&_buckets[i]);
				for (size_t j = 0; j < other._buckets[i].second; j++) {
					new(&_buckets[i].first[j]) KeyValPair(other._buckets[i].first[j]);
					_buckets[i].second++;
					_elements.pushBack(&_buckets[i].first[j]);
				};
			}
		}

		inline void Reserve(uint32_t capacity) {
			if (capacity <= _capacity) {
				return;
			}
			uint32_t oldCapacity = _capacity;
			_capacity = _capacity ? _capacity : 1;
			while (capacity > _capacity) {
				_capacity *= 2;
			}
			Bucket* temp = _buckets;
			_buckets = _allocator.allocate(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				_allocator.construct(&_buckets[i]);
			}
			_elements.Clear();
			_elements.Reserve(_capacity);
			for (size_t i = 0; i < oldCapacity; i++) {
				for (size_t j = 0; j < temp[i].second; j++) {
					KeyValPair* pair = Emplace(std::move(temp[i].first[j].first)).second;
					new(&pair->second) Val(std::move(temp[i].first[j].second));
				}
			}
			_allocator.deallocate(temp, 1);
		}

		inline Tuple<bool, KeyValPair*> Insert(const KeyValPair& pair) {
			uint64_t hash = Hasher()(pair.first);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (KeyValPair& element : bucket.first) {
					if (element.first == pair.first) {
						return { false, &element };
					}
				}
				logWarning(this, "bad hash (function simple::Map::Insert)!");
				if (bucket.second >= T_max_bucket_size - 1) {
					return { false, nullptr };
				}
			}
			if (!_capacity || (float)Size() / _capacity >= 0.8f) {
				Reserve(_capacity ? _capacity * 2 : 128);
			}
			new(&bucket.first[bucket.second]) KeyValPair();
			bucket.first[bucket.second].first = pair.first;
			bucket.first[bucket.second].second = pair.second;
			_elements.pushBack(&bucket.first[bucket.second]);
			return { true, &bucket.first[bucket.second++] };
		}

		template<typename... Args>
		inline Tuple<bool, KeyValPair*> Emplace(Args&&... args) {
			Key key(std::forward<Args>(args)...);
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (const KeyValPair& element : bucket.first) {
					if (element.first == key) {
						return { false, nullptr };
					}
				}
				logWarning(this, "bad hash (function simple::Map::Emplace)!");
				if (bucket.second >= 3) {
					return { false, nullptr };
				}
			}
			if (!_capacity || (float)Size() / _capacity >= 0.8f) {
				Reserve(_capacity ? _capacity * 2 : 128);
			}
			new(&bucket.first[bucket.second]) KeyValPair();
			new(&bucket.first[bucket.second].first) Key(std::move(key));
			_elements.PushBack(&bucket.first[bucket.second]);
			return { true, &bucket.first[bucket.second++] };
		}

		inline bool Contains(const Key& key) const noexcept {
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

		inline KeyValPair* Find(const Key& key) const noexcept {
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

		constexpr inline size_t Size() const noexcept {
			return _elements.Size();
		}

		inline Val* operator[](const Key& key) {
			if (!_buckets) {
				Reserve(128);
			}
			uint64_t hash = Hasher()(key);
			auto& bucket = _buckets[hash % _capacity];
			if (bucket.second) {
				for (KeyValPair& element : bucket.first) {
					if (element.first == key) {
						return &element.second;
					}
				}
				logWarning(this, "bad hash (simple::Map::operator[])!");
				if (bucket.second >= T_max_bucket_size - 1) {
					return nullptr;
				}
			}
			if ((float)Size() / _capacity > 0.8f) {
				Reserve(_capacity * 2);
			}
			new(&bucket.first[bucket.second]) KeyValPair();
			bucket.first[bucket.second].first = key;
			_elements.PushBack(&bucket.first[bucket.second]);
			return &bucket.first[bucket.second++].second;
		}

		inline Map& operator=(const Map& other) {
			_capacity = other._capacity;
			_buckets = _allocator.allocate(_capacity);
			_elements.reserve(_capacity);
			for (size_t i = 0; i < _capacity; i++) {
				_allocator.construct(&_buckets[i]);
				for (size_t j = 0; j < other._buckets[i].second; j++) {
					new(&_buckets[i].first[j]) KeyValPair(other._buckets[i].first[j]);
					_elements.pushBack(&_buckets[i].first[j]);
				};
			}
			return *this;
		}

		inline Iterator begin() {
			return _elements.begin();
		}

		inline const Iterator end() {
			return (KeyValPair**)_elements.end();
		}

		~Map() {
			for (KeyValPair& element : *this) {
				(&element)->~KeyValPair();
			}
			_allocator.deallocate(_buckets, 1);
			_buckets = nullptr;
		}

	private:
		BucketAllocator _allocator;
		uint32_t _capacity;
		Bucket* _buckets;
		simple::DynamicArray<KeyValPair*> _elements;
	};
}
