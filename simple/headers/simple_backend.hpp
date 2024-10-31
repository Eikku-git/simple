#pragma once

#include "vulkan/vulkan.h"
#include <simple_array.hpp>
#include <cstdint>
#include <thread>
#ifdef FRAMES_IN_FLIGHT
#else
#define FRAMES_IN_FLIGHT 2U
#endif

namespace simple {

	class Simple;

	class Thread {
	public:

		std::thread::id GetID() const {
			return ID;
		}

		VkCommandPool GetGraphicsCommandPool() {
			return graphicsCommandPool;
		}

		VkCommandPool GetTransferCommandPool() {
			return transferCommandPool;
		}

	private:
		std::thread::id ID;
		VkCommandPool graphicsCommandPool;
		VkCommandPool transferCommandPool;
		inline bool operator=(const Thread& other) const {
			return ID == other.ID;
		}
		friend class Backend;
	};

	class Window {
	};

	struct Queue {
		VkQueue vkQueue{};
		uint32_t index{};
	};

	class Backend {
	public:
		Backend(Simple& simple);
		const Thread& GetMainThread();
	private:
		Thread _mainThread{};
		Window _window{};
		VkInstance _vkInstance{};
		VkPhysicalDevice _vkPhysicalDevice{}; 
		VkSurfaceKHR _vkSurfaceKHR{};
		Queue _graphicsQueue{};
		Queue _transferQueue{};
		Queue _presentQueue{};
		VkSampleCountFlagBits _maxColorMsaaSamples{};
		VkSampleCountFlagBits _maxDepthMsaaSamples{};
		simple::Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameReadySemaphores{};
		simple::Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameFinishedSemaphores{};
		simple::Array<VkFence, FRAMES_IN_FLIGHT> _inFlightFences{};
		simple::Array<VkCommandBuffer, FRAMES_IN_FLIGHT> _graphicsCommandBuffers{};
		friend class Simple;
	};
}
