#include "simple.hpp"

int main() {
	simple::Window window{};
	window.Init(540, 540, "Test", nullptr);
	simple::Simple engine(std::move(window));
	return 0;
}