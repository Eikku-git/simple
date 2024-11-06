#include "simple_window.hpp"

namespace simple {

	void WindowSystem::_AddpWindow(Window* pWindow) {
		_activeWindows.PushBack(pWindow);
	}

	bool WindowSystem::_RemoveWindow(GLFWwindow* pGlfwWindow) {
		auto iter = _activeWindows.begin();
		for (; iter != _activeWindows.end();) {
			assert(*iter && "bug in code!");
			if ((*iter)->_pGlfwWindow == pGlfwWindow) {
				_activeWindows.Erase(iter);
				return true;
			}
		}
		return false;
	}

	Window** WindowSystem::_GetppWindow(GLFWwindow* pGlfwWindow) {
		auto iter = _activeWindows.begin();
		for (; iter != _activeWindows.end();) {
			assert(*iter && "bug in code!");
			if ((*iter)->_pGlfwWindow == pGlfwWindow) {
				return iter;
			}
		}
		return iter;
	}
};
