#include "simple.hpp"

int main() {
	simple::WindowSystem::Init();
	simple::Window window{};
	window.Init(540, 540, "Test", nullptr);
	simple::Simple engine(std::move(window));
	simple::WindowSystem::Terminate();
	return 0;
}