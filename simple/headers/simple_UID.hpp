#pragma once

#include <random>
#include <atomic>

namespace simple {

	namespace UID {
	
		static uint64_t GenerateState64() {
			std::random_device device{};
			std::mt19937_64 mt(device());
			std::uniform_int_distribution<uint64_t> dist(UINT32_MAX, UINT64_MAX);
			return dist(mt);
		}

		template<typename IntegerType>
		static uint64_t Shuffle(IntegerType& rState) {
			rState ^= rState >> 12;
			rState ^= rState << 25;
			rState ^= rState >> 27;
			return rState;
		}

		template<typename IntegerType>
		static std::atomic<IntegerType>& Shuffle(std::atomic<IntegerType>& rState) {
			rState ^= rState >> 12;
			rState ^= rState << 25;
			rState ^= rState >> 27;
			return rState;
		}
	};
}
