#pragma once

#include <iostream>

namespace simple {
	static inline void logError(void* object, const char* err) noexcept {
		std::cerr << "Error thrown by object at memory location " << object << ": " << err;
	};
}
