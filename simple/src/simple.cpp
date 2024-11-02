#include "simple.hpp"

namespace simple {

	Backend& Simple::GetBackend() {
		return _backend;
	}

	void Simple::Terminate() {
	}

	Backend::Backend(Simple& engine) : _engine(engine) {
		
	}

	const Thread& Backend::GetMainThread() {
		return _mainThread;
	}
}

