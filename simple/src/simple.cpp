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

	Backend& Simple::GetBackend() {
		return _backend;
	}

	void Simple::Terminate() {
	}

#define _DEBUG
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

		assert(Succeeded(vkCreateInstance(&instanceCreateInfo, _vkAllocationCallbacks, &_vkInstance)) && "failed to create VkInstance (function vkCreateInstance) for simple::Backend");
		assert(Succeeded(glfwCreateWindowSurface(_vkInstance, _engine._window.GetRawPointer(), _vkAllocationCallbacks, &_vkSurfaceKHR)));

		uint32_t vkPhysicalDeviceCount;
		vkEnumeratePhysicalDevices(_vkInstance, &vkPhysicalDeviceCount, nullptr);
		simple::DynamicArray<VkPhysicalDevice> vkPhysicalDevices(vkPhysicalDeviceCount);
		vkEnumeratePhysicalDevices(_vkInstance, &vkPhysicalDeviceCount, vkPhysicalDevices.Data());

		Tuple<vulkan::PhysicalDeviceInfo*, int> bestPhysicalDevice { nullptr, -1 };
		for (VkPhysicalDevice device : vkPhysicalDevices) {
			vulkan::PhysicalDeviceInfo deviceInfo(device, _vkSurfaceKHR);
			bool allRequiredExtensionsFound = true;
			for (const VkExtensionProperties& properties : deviceInfo.vkExtensionProperties) {
				if (!Find(properties.extensionName, requiredDeviceExtensions.begin(), requiredInstanceExtensions.end())) {
					allRequiredExtensionsFound = false;	
				}
			}
			int score = 10;
			if (!deviceInfo.vkPhysicalDeviceFeatures.samplerAnisotropy || 
				!deviceInfo.graphicsQueueFamilyIndex || !deviceInfo.transferQueueFound || !deviceInfo.presentQueueFound ||
				!allRequiredExtensionsFound || !deviceInfo.vkSurfaceFormats.Size() || !deviceInfo.vkPresentModes.Size()
				|| !deviceInfo.vkPhysicalDeviceFeatures.fillModeNonSolid) {
				score = -1;
			}
			else if (deviceInfo.vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score = 100;
			}
			if (score > bestPhysicalDevice.second) {
				delete bestPhysicalDevice.first;
				bestPhysicalDevice.first = new vulkan::PhysicalDeviceInfo(deviceInfo);
				bestPhysicalDevice.second = score;
			}
		}

		assert(bestPhysicalDevice.first != VK_NULL_HANDLE && "no suitable VkPhysicalDevice found (simple::Backend constructor)!");

		_vkPhysicalDevice = bestPhysicalDevice.first->vkPhysicalDevice;
		_colorMsaaSamples = bestPhysicalDevice.first->vkPhysicalDeviceProperties.limits.sampledImageColorSampleCounts;
		_depthMsaaSamples = bestPhysicalDevice.first->vkPhysicalDeviceProperties.limits.sampledImageDepthSampleCounts;

		simple::Array<VkDeviceQueueCreateInfo, 3> vkDeviceQueueCreateInfos{};
		simple::Array<uint32_t, 3> queueFamilyIndices { 
			bestPhysicalDevice.first->graphicsQueueFamilyIndex, 
			bestPhysicalDevice.first->transferQueueFamilyIndex, 
			bestPhysicalDevice.first->presentQueueFamilyIndex, 
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

		assert(Succeeded(vkCreateDevice(_vkPhysicalDevice, &vkDeviceCreateInfo, _vkAllocationCallbacks, &_vkDevice)) && "failed to create VkDevice (simple::Backend constructor)!");

		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[0], 0, &_graphicsQueue.vkQueue);
		_graphicsQueue.index = queueFamilyIndices[0];
		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[1], 0, &_transferQueue.vkQueue);
		_transferQueue.index = queueFamilyIndices[3];
		vkGetDeviceQueue(_vkDevice, queueFamilyIndices[2], 0, &_presentQueue.vkQueue);
		_presentQueue.index = queueFamilyIndices[3];

		delete bestPhysicalDevice.first;
	}

	const Thread& Backend::GetMainThread() {
		return _mainThread;
	}
}
