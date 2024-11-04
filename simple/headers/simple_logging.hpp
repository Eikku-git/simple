#pragma once

#include <iostream>

namespace simple {

	static inline void logMessage(const void* object, const char* message) noexcept {
		std::clog << "Message from object at address " << object << ": " << message << "\n";
	}

	static inline void logWarning(const void *object, const char *warn) noexcept {
		std::clog << "Warning from object at address " << object << ": " << warn << "\n";
	}

	static inline void logError(const void* object, const char* err) noexcept {
		std::cerr << "Error thrown by object at address " << object << ": " << err << "\n";
	};
}
