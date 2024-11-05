#include "simple.hpp"

static void asyncProcess() {
	for (size_t i = 0; i < 1000000000; i++) {
	}
}

int main() {
	simple::WindowSystem::Init();
	simple::Window window{};
	window.Init(540, 540, "Test", nullptr);
	simple::Simple engine(std::move(window));
	simple::Backend& backend = engine.GetBackend();
	std::thread thread(asyncProcess);
	backend.NewThread(std::move(thread));
	simple::WindowSystem::Terminate();
	return 0;
}