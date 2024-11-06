#define EDITOR
#include "simple.hpp"

int main() {
	simple::WindowSystem::Init();
	simple::Window window{};
	window.Init(540, 540, "Test", nullptr);
	simple::Simple engine(std::move(window));
	simple::Backend& backend = engine.GetBackend();
	while (!engine.Quitting()) {
		if (!engine.EditorUpdate()) {
			break;
		}
		engine.LogicUpdate();
		engine.Render();
	}
	simple::WindowSystem::Terminate();
	return 0;
}