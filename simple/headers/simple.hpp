#pragma once

namespace simple {

	class Backend;

	class Simple {
	public:
		void Init();
		Backend& GetBackend();
		void Terminate();
	private:
		Backend* _backend;
		friend class Backend;
	};
}
