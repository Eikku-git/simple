#pragma once

#include <utility>

namespace simple {

	template<typename T1, typename T2>
	struct Pair {
		Pair() noexcept : first(), second() {}
		Pair(const T1& first, const T2& second) noexcept : first(first), second(second) {}
		Pair(T1&& first, T2&& second) noexcept : first(std::move(first)), second(std::move(second)) {}
		Pair(const Pair& other) noexcept : first(other.first), second(other.second) {}
		Pair(Pair&& other) noexcept : first(std::move(other.first)), second(std::move(other.second)) {}
		T1 first{};
		T2 second{};
		simple::Pair<T1, T2>& operator=(const simple::Pair<T1, T2>& other) {
			first = other.first;
			second = other.second;
			return *this;
		}
	};
}
