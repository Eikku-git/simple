#pragma once

#include "cstdint"

namespace simple {

	template<typename T>
	inline void XorSwap(T& a, T& b) {
		a = a ^ b;
		b = a ^ b;
		a = a ^ b;
	}

	template<typename T>
	inline void Swap(T& a, T& b) {
		T temp = a;
		a = b;
		b = temp;
	}

	template<typename T>
	inline T Clamp(T val, T min, T max) {
		val = val < min ? min : val;
		return val > max ? max : val;
	}

	template<typename T>
	inline void Clamp(T* pVal, T min, T max) {
		*pVal = *pVal < min ? min : *pVal;
		*pVal = *pVal > max ? max : *pVal;
	}

	template<typename T>
	inline T& Max(T* t0, T* t1) {
		return t0 < t1 ? t1 : t0;
	}

	template<typename T>
	inline T Max(T t0, T t1) {
		return t0 < t1 ? t1 : t0;
	}
}
