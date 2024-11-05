#include "simple.hpp"
#include "GLFW/glfw3.h"
#include "simple_array.hpp"
#include "simple_dynamic_array.hpp"
#include "simple_logging.hpp"
#include "simple_string.hpp"
#include "simple_vulkan.hpp"
#include "simple_tuple.hpp"
#include "vulkan/vulkan_core.h"

namespace simple {

	void simple::CommandBuffer::Allocate() {
		assert(_vkCommandBuffer == VK_NULL_HANDLE && "attempting to allocate simple::CommandBuffer that's already allocated!");
		VkCommandBufferAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = _vkCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		assert(Succeeded(vkAllocateCommandBuffers(_backend._vkDevice, &allocInfo, &_vkCommandBuffer))
			&& "failed to allocate command buffer (function simple::CommandBuffer::Allocate)");
	}

	void simple::CommandBuffer::Submit() {
		assert(_vkCommandBuffer != VK_NULL_HANDLE && "attempting to submit simple::CommandBuffer that hasn't been allocated yet!");
		switch (_threadCommandPool) {
			case ThreadCommandPool::None:
				// immediate submit 
				break;
			case ThreadCommandPool::Graphics:
				_backend._QueueGraphicsCommandBuffer(_vkCommandBuffer);
				break;
			case ThreadCommandPool::Transfer:
				// submit to transfer queue
				break;
		};
	}

#ifdef _DEBUG
	constexpr size_t layers_to_enable_count = 1;
#else
	constexpr size_t layer_count = 0;
#endif

	const String<> layersToEnable[] {
		"VK_LAYER_KHRONOS_validation",
	};

	const Array<String<>, 1> requiredDeviceExtensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	void Backend::_CreateSwapchain() {

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_vkPhysicalDevice, _vkSurfaceKHR, &_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR);

		if (_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.currentExtent.width != UINT32_MAX) {
			_swapchainVkExtent2D = _vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(_engine._window.GetRawPointer(), &width, &height);
			VkExtent2D actualExtent{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
			};
			actualExtent.width
				= Clamp(
					actualExtent.width,
					_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.minImageExtent.width,
					_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.maxImageExtent.width
				);
			actualExtent.width
				= Clamp(
					actualExtent.width,
					_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.minImageExtent.width,
					_vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.maxImageExtent.width
				);
			_swapchainVkExtent2D = actualExtent;
		}

		assert(FRAMES_IN_FLIGHT < _vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.maxImageCount
			&& "FRAMES_IN_FLIGHT exceeds the maximum supported maximum image count given by vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		assert(FRAMES_IN_FLIGHT > _vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.minImageCount
			&& "FRAMES_IN_FLIGHT are less than the minimum supported image count given by vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

		uint32_t queueFamilies[2] {
			_graphicsQueue.index,
			_presentQueue.index,
		};

		VkSwapchainCreateInfoKHR vkSwapchainCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = _vkSurfaceKHR,
			.minImageCount = FRAMES_IN_FLIGHT,
			.imageFormat = _vkSurfaceFormatKHR.format,
			.imageColorSpace = _vkSurfaceFormatKHR.colorSpace,
			.imageExtent = _swapchainVkExtent2D,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = 2,
			.pQueueFamilyIndices = queueFamilies,
			.preTransform = _vulkanPhysicalDeviceInfo.vkSurfaceCapabilitiesKHR.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = _vkPresentModeKHR,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		assert(Succeeded(vkCreateSwapchainKHR(_vkDevice, &vkSwapchainCreateInfo, _vkAllocationCallbacks, &_vkSwapchainKHR))
			&& "failed to create swapchains (function vkCreateSwapchainKHR in function simple::Backend::_CreateSwapchain)");

		uint32_t imageCount = FRAMES_IN_FLIGHT;
		assert(Succeeded(vkGetSwapchainImagesKHR(_vkDevice, _vkSwapchainKHR, &imageCount, _swapchainImages.Data()))
			&& "failed to get vulkan swapchain images (function vkGetSwapchainImagesKHR in function simple::Backend::_CreateSwapchain)");

		for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			VkImageViewCreateInfo imageViewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = _swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = _vkSurfaceFormatKHR.format,
				.components {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				}
			};
			assert(Succeeded(vkCreateImageView(_vkDevice, &imageViewInfo, _vkAllocationCallbacks, &_swapchainImageViews[i]))
				&& "failed to create vulkan swapchain image view (function vkCreateImageView in function simple::Backend::_CreateSwapchain)");
		}

		CommandBuffer imageLayoutTransitionCommandBuffer(GetNewGraphicsCommandBuffer(GetThread(std::this_thread::get_id())));
		imageLayoutTransitionCommandBuffer.Allocate();
		Array<VkImageMemoryBarrier, FRAMES_IN_FLIGHT> imageTransitionMemoryBarriers{};
		for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			imageTransitionMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageTransitionMemoryBarriers[i].pNext = nullptr;
			imageTransitionMemoryBarriers[i].srcAccessMask = 0;
			imageTransitionMemoryBarriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageTransitionMemoryBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageTransitionMemoryBarriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageTransitionMemoryBarriers[i].srcQueueFamilyIndex = _graphicsQueue.index;
			imageTransitionMemoryBarriers[i].dstQueueFamilyIndex = _graphicsQueue.index;
			imageTransitionMemoryBarriers[i].image = _swapchainImages[i];
			imageTransitionMemoryBarriers[i].subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
		}
		vkCmdPipelineBarrier(imageLayoutTransitionCommandBuffer.Begin(), image_transition_src_stage_mask, image_transition_dst_stage_mask, 
		0, 0, nullptr, 0, nullptr, FRAMES_IN_FLIGHT, imageTransitionMemoryBarriers.Data());
		imageLayoutTransitionCommandBuffer.End();
		imageLayoutTransitionCommandBuffer.Submit();
	}

	Backend::Backend(Simple& engine) : _engine(engine) {

		uint32_t glfwRequiredExtensionsCount{};
		const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
		DynamicArray<const char*> requiredInstanceExtensions(glfwRequiredExtensions, glfwRequiredExtensions + glfwRequiredExtensionsCount);
#ifdef _DEBUG
		requiredInstanceExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		uint32_t vkLayerPropertyCount;
		vkEnumerateInstanceLayerProperties(&vkLayerPropertyCount, nullptr);
		DynamicArray<VkLayerProperties> vkLayerProperties(vkLayerPropertyCount);
		vkEnumerateInstanceLayerProperties(&vkLayerPropertyCount, vkLayerProperties.Data());
		DynamicArray<const char*> enabledVkLayers{};
		enabledVkLayers.Reserve(layers_to_enable_count);
		for (VkLayerProperties& layerProperties : vkLayerProperties) {
			auto iter = Find(layerProperties.layerName, &layersToEnable[0], &layersToEnable[layers_to_enable_count]);
			if (iter != &layersToEnable[layers_to_enable_count]) {
				enabledVkLayers.PushBack(layerProperties.layerName);
			}

		}
		if (enabledVkLayers.Size() != layers_to_enable_count) {
			logWarning(this, "some vulkan layer(s) requested are not supported (warning from simple::Backend constructor)!");
		}

		VkApplicationInfo appInfo {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = Simple::app_name,
			.applicationVersion = VK_MAKE_API_VERSION(0, 0, 5, 0),
			.pEngineName = Simple::engine_name,
			.engineVersion = VK_MAKE_API_VERSION(0, 0, 5, 0),
			.apiVersion = VK_API_VERSION_1_3,
		};

		VkInstanceCreateInfo instanceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = enabledVkLayers.Size(),
			.ppEnabledLayerNames = enabledVkLayers.Data(),
			.enabledExtensionCount = requiredInstanceExtensions.Size(),
			.ppEnabledExtensionNames = requiredInstanceExtensions.Data(),
		};

		assert(Succeeded(vkCreateInstance(&instanceCreateInfo, _vkAllocationCallbacks, &_vkInstance)) 
			&& "failed to create VkInstance (function vkCreateInstance) for simple::Backend");
		assert(Succeeded(glfwCreateWindowSurface(_vkInstance, _engine._window.GetRawPointer(), _vkAllocationCallbacks, &_vkSurfaceKHR))
			&& "failed to create window surface (function glfwCreateWindowSurface) for simple::Backend");

		uint32_t vkPhysicalDeviceCount;
		vkEnumeratePhysicalDevices(_vkInstance, &vkPhysicalDeviceCount, nullptr);
		simple::DynamicArray<VkPhysicalDevice> vkPhysicalDevices(vkPhysicalDeviceCount);
		vkEnumeratePhysicalDevices(_vkInstance, &vkPhysicalDeviceCount, vkPhysicalDevices.Data());

		Tuple<vulkan::PhysicalDeviceInfo, int> bestPhysicalDevice{};
		for (VkPhysicalDevice device : vkPhysicalDevices) {
			vulkan::PhysicalDeviceInfo deviceInfo(device, _vkSurfaceKHR);
			bool allRequiredExtensionsFound = true;
			for (const VkExtensionProperties& properties : deviceInfo.vkExtensionProperties) {
				if (!Find(properties.extensionName, requiredDeviceExtensions.begin(), requiredDeviceExtensions.end())) {
					allRequiredExtensionsFound = false;	
				}
			}
			int score = 10;
			if (!deviceInfo.vkPhysicalDeviceFeatures.samplerAnisotropy || 
				!deviceInfo.graphicsQueueFamilyIndex || !deviceInfo.transferQueueFound || !deviceInfo.presentQueueFound ||
				!allRequiredExtensionsFound || !deviceInfo.vkSurfaceFormatsKHR.Size() || !deviceInfo.vkPresentModesKHR.Size()
				|| !deviceInfo.vkPhysicalDeviceFeatures.fillModeNonSolid) {
				score = -1;
			}
			else if (deviceInfo.vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score = 100;
			}
			if (score > bestPhysicalDevice.second) {
				bestPhysicalDevice.first = vulkan::PhysicalDeviceInfo(deviceInfo);
				bestPhysicalDevice.second = score;
			}
		}

		assert(bestPhysicalDevice.first.vkPhysicalDevice != VK_NULL_HANDLE 
			&& "no suitable VkPhysicalDevice found (simple::Backend constructor)!");

		_vulkanPhysicalDeviceInfo = bestPhysicalDevice.first;
		_vkPhysicalDevice = _vulkanPhysicalDeviceInfo.vkPhysicalDevice;
		_colorMsaaSamples = _vulkanPhysicalDeviceInfo.vkPhysicalDeviceProperties.limits.sampledImageColorSampleCounts;
		_depthMsaaSamples = _vulkanPhysicalDeviceInfo.vkPhysicalDeviceProperties.limits.sampledImageDepthSampleCounts;

		simple::Array<VkDeviceQueueCreateInfo, 3> vkDeviceQueueCreateInfos{};
		simple::Array<uint32_t, 3> queueFamilyIndices { 
			_vulkanPhysicalDeviceInfo.graphicsQueueFamilyIndex, 
			_vulkanPhysicalDeviceInfo.transferQueueFamilyIndex, 
			_vulkanPhysicalDeviceInfo.presentQueueFamilyIndex, 
		};
		float queuePriority = 1.0f;
		for (uint32_t i = 0; i < 3; i++) {
			vkDeviceQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			vkDeviceQueueCreateInfos[i].queueCount = 1;
			vkDeviceQueueCreateInfos[i].queueFamilyIndex = queueFamilyIndices[i];
			vkDeviceQueueCreateInfos[i].pQueuePriorities = &queuePriority;
		}

		VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};
		vkPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
		vkPhysicalDeviceFeatures.sampleRateShading = VK_TRUE;
		vkPhysicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

		VkPhysicalDeviceVulkan13Features vkPhysicalDeviceVulkan13Features{};
		vkPhysicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vkPhysicalDeviceVulkan13Features.pNext = nullptr;
		vkPhysicalDeviceVulkan13Features.dynamicRendering = VK_TRUE;

		VkDeviceCreateInfo vkDeviceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &vkPhysicalDeviceVulkan13Features,
			.queueCreateInfoCount = vkDeviceQueueCreateInfos.Size(),
			.pQueueCreateInfos = vkDeviceQueueCreateInfos.Data(),
			.enabledExtensionCount = requiredDeviceExtensions.Size(),
			.ppEnabledExtensionNames = requiredInstanceExtensions.Data(),
			.pEnabledFeatures = &vkPhysicalDeviceFeatures,
		};

		assert(Succeeded(vkCreateDevice(_vkPhysicalDevice, &vkDeviceCreateInfo, _vkAllocationCallbacks, &_vkDevice)) 
			&& "failed to create VkDevice (function vkCreateDevice in simple::Backend constructor)!");

		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[0], 0, &_graphicsQueue.vkQueue);
		_graphicsQueue.index = queueFamilyIndices[0];
		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[1], 0, &_transferQueue.vkQueue);
		_transferQueue.index = queueFamilyIndices[3];
		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[2], 0, &_presentQueue.vkQueue);
		_presentQueue.index = queueFamilyIndices[3];

		_mainThread = &_NewThread(std::this_thread::get_id());

		VkCommandBufferAllocateInfo vkRenderingCommandBufferAllocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = _mainThread->_vkGraphicsCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = FRAMES_IN_FLIGHT,
		};

		assert(Succeeded(vkAllocateCommandBuffers(_vkDevice, &vkRenderingCommandBufferAllocInfo, _vkRenderingCommandBuffers.Data())) && "failed to allocate rendering command buffers (simple::Backend constructor)!");

		VkSemaphoreCreateInfo vkSemaphoreCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		VkFenceCreateInfo vkFenceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			assert(
				Succeeded(
					vkCreateSemaphore(_vkDevice, &vkSemaphoreCreateInfo, _vkAllocationCallbacks, &_frameReadyVkSemaphores[i])
				) &&
				Succeeded(
					vkCreateSemaphore(_vkDevice, &vkSemaphoreCreateInfo, _vkAllocationCallbacks, &_frameFinishedVkSemaphores[i])
				) &&
				Succeeded(
					vkCreateFence(_vkDevice, &vkFenceCreateInfo, _vkAllocationCallbacks, &_inFlightVkFences[i])
				) &&
				"failed to create VkSemaphores/VkFences (vkCreateSemaphore/vkCreateFence) (simple::Backend constructor)"
			);
		}

		_vkSurfaceFormatKHR = _vulkanPhysicalDeviceInfo.vkSurfaceFormatsKHR[0];
		for (VkSurfaceFormatKHR format : _vulkanPhysicalDeviceInfo.vkSurfaceFormatsKHR) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				_vkSurfaceFormatKHR = format;
				break;
			}
		}
		_vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;
		for (VkPresentModeKHR presentMode : _vulkanPhysicalDeviceInfo.vkPresentModesKHR) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				_vkPresentModeKHR = presentMode;
				break;
			}
		}

		_CreateSwapchain();
	}

	Backend& Simple::GetBackend() {
		return _backend;
	}

	Simple::~Simple() {
	}
}
