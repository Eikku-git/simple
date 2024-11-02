#pragma once

#include <cstdint>
#include <utility>

namespace simple {

	template<typename T>
	class DynamicAllocator {
	public:

		inline DynamicAllocator() = default;

		constexpr inline DynamicAllocator(const DynamicAllocator&) noexcept { }

		T* allocate(size_t size) {
			return static_cast<T*>(::operator new(size * sizeof(T)));
		}

		void deallocate(T* ptr, size_t size) noexcept {
			::operator delete(ptr);
		}

		template<typename... Args>
		void construct(T* ptr, Args&&... args) {
			new(ptr) T(std::forward<Args>(args)...);
		}

		void destroy(T* ptr) noexcept {
			ptr->~T();
		}

		friend bool operator==(const DynamicAllocator&, const DynamicAllocator&) {
			return true;
		}

		friend bool operator!=(const DynamicAllocator&, const DynamicAllocator&) {
			return false;
		}
	};
}
