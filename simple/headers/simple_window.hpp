#pragma once

#include "GLFW/glfw3.h"
#include "simple_allocator.hpp"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "simple_math.hpp"
#include <cstdint>
#include <assert.h>

namespace simple {

	class WindowSystem {
	public:

		static inline void Init() {
			glfwInit();
		};

		static inline void Terminate() {
			glfwTerminate();
		};
	};

	typedef GLFWmonitor* Monitor;

	class Window;

	class Lol {
	};

	enum class Key {
		Space = 32,
		Apostrophe = 39,
		Comma = 44,
		Minus = 45,
		Period = 46,
		Slash = 47,
		Zero = 48,
		One = 49,
		Two = 50,
		Three = 51,
		Four = 52,
		Five = 53,
		Six = 54,
		Seven = 55,
		Eight = 56,
		Nine = 57,
		Semicolon = 59,
		Equal = 61,
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 64,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,
		LeftBracket = 91,
		Backslash = 92,
		RightBracket = 93,
		GraveAccent = 96,
		World1 = 161,
		World2 = 162,
		Escape  = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,
		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348,
		MaxEnum = 348,
	};

	enum class MouseButton {
		One = 0,
		Two = 1,
		Three = 2,
		Four = 3,
		Five = 4,
		Six = 6,
		Eight = 7,
		Left = One,
		Right = Two,
		Middle = Three,
		MaxEnum = Eight,
	};

	class Input {
	public:

		static inline constexpr size_t last_key = static_cast<size_t>(Key::MaxEnum);
		static inline constexpr size_t last_mouse_button = static_cast<size_t>(MouseButton::MaxEnum);

	private:

		void Init(Window& window);

		simple::Array<float, last_key> _keyValues{};
		simple::Array<bool, last_key> _pressedKeys{};
		simple::Array<bool, last_key> _heldKeys{};
		simple::Array<bool, last_key> _releasedKeys{};

		simple::DynamicArray<Key> _activeKeys{};

		simple::Array<float, last_mouse_button> _mouseButtonValues{};
		simple::Array<bool, last_mouse_button> _pressedMouseButtons{};
		simple::Array<bool, last_mouse_button> _heldMouseButtons{};
		simple::Array<bool, last_mouse_button> _releasedMouseButtons{};

		simple::DynamicArray<MouseButton> _activeMouseButtons;

		DVec2 _cursorPos;
		DVec2 _deltaCursorPos;
		friend class Window;
	};

	class Window {
	public:

		inline Window(Window&& other) noexcept : _pGlfwWindow(other._pGlfwWindow), _pInput(other._pInput) {
			other._pGlfwWindow = nullptr;
			other._pInput = nullptr;
		}

		inline bool IsNull() const noexcept{
			return !_pGlfwWindow;
		}

		inline bool Init(uint32_t width, uint32_t height, const char* title, Monitor monitor) {
			assert(monitor || width && height && "attempting to create a window with invalid arguments");
			_pGlfwWindow = glfwCreateWindow(width, height, title, monitor, nullptr);
			if (!_pGlfwWindow) {
				return false;
			}
			_pInput->Init(*this);
			return true;
		}

		inline void Terminate() {
			glfwDestroyWindow(_pGlfwWindow);
		}

		GLFWwindow* GetRawPointer() {
			return _pGlfwWindow;
		}

	private:

		GLFWwindow* _pGlfwWindow = nullptr;
		Input* _pInput = nullptr;
	};
}

