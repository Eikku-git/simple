#pragma once

#include "simple_array.hpp"

#ifdef FramesInFlight
#else
#define FramesInFlight 2U
#endif
#define SimpleArray(T, size) simple::Array<T, size>
#define FIFarray(T) SimpleArray(T, FramesInFlight)
