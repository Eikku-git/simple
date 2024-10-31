#include "simple.hpp"
#include "simple_backend.hpp"
#include <assert.h>
#include <thread>

namespace simple {

	void Simple::Init() {
	}	

	Backend& Simple::GetBackend() {
		return *_backend;
	}

	void Simple::Terminate() {
	}

	Backend::Backend(Simple& simple) {
		assert(!simple._backend && "don't try to initialize backend manually!");
		
	}

	const Thread& Backend::GetMainThread() {
		return _mainThread;
	}
}

