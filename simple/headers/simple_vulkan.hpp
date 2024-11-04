#pragma once

#include "vulkan/vulkan.h"
#include "simple_dynamic_array.hpp"

namespace simple {
	namespace vulkan {

		struct PhysicalDeviceInfo {
			VkPhysicalDevice vkPhysicalDevice;
			VkSurfaceKHR vkSurfaceKHR;
			VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
			VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
			DynamicArray<VkExtensionProperties> vkExtensionProperties{};
			DynamicArray<VkQueueFamilyProperties> vkQueueFamilyProperties{};
			uint32_t graphicsQueueFamilyIndex, transferQueueFamilyIndex, presentQueueFamilyIndex{};
			bool graphicsQueueFound, transferQueueFound, presentQueueFound{};
			VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKHR;
			DynamicArray<VkSurfaceFormatKHR> vkSurfaceFormats{};
			DynamicArray<VkPresentModeKHR> vkPresentModes{};

			inline PhysicalDeviceInfo(VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurfaceKHR) noexcept
			: vkPhysicalDevice(vkPhysicalDevice), vkSurfaceKHR(vkSurfaceKHR) {

				vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &vkPhysicalDeviceFeatures);
				vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);

				uint32_t vkExtensionPropertiesCount;
				vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &vkExtensionPropertiesCount, nullptr);
				vkExtensionProperties.Resize(vkExtensionPropertiesCount);
				vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &vkExtensionPropertiesCount, vkExtensionProperties.Data());

				uint32_t vkQueueFamilyPropertiesCount;
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyPropertiesCount, nullptr);
				vkQueueFamilyProperties.Resize(vkQueueFamilyPropertiesCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyPropertiesCount, vkQueueFamilyProperties.Data());

				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurfaceKHR, &vkSurfaceCapabilitiesKHR);

				uint32_t vkSurfaceFormatsCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &vkSurfaceFormatsCount, nullptr);
				vkSurfaceFormats.Resize(vkSurfaceFormatsCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &vkSurfaceFormatsCount, vkSurfaceFormats.Data());

				uint32_t vkPresentModesCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &vkPresentModesCount, nullptr);
				vkPresentModes.Resize(vkPresentModesCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &vkPresentModesCount, vkPresentModes.Data());

				uint32_t queueFamilyIndex = 0;
				for (const VkQueueFamilyProperties& properties : vkQueueFamilyProperties) {
					bool transferOrGraphicsQueueFound = false;
					if (!transferQueueFound && properties.queueFlags & VK_QUEUE_TRANSFER_BIT
						&& (graphicsQueueFound || !(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT))) {
						transferQueueFamilyIndex = queueFamilyIndex;
						transferQueueFound = true;
						transferOrGraphicsQueueFound = true;
					}
					if (!transferOrGraphicsQueueFound && !graphicsQueueFound && properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						graphicsQueueFamilyIndex = queueFamilyIndex;
						graphicsQueueFound = true;
						transferOrGraphicsQueueFound = true;
					}
					if (!transferOrGraphicsQueueFound && !presentQueueFound) {
						VkBool32 presentSupported{};
						vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueFamilyIndex, vkSurfaceKHR, &presentSupported);
						if (presentSupported) {
							presentQueueFamilyIndex = queueFamilyIndex;
							presentSupported = true;
						}
					}
					if (graphicsQueueFound && transferQueueFound && presentQueueFound) {
						break;
					}
					++queueFamilyIndex;
				}
			}
		};

		template<size_t candidate_count>
		inline VkFormat FindSupportedFormat(VkPhysicalDevice vkPhysicalDevice, const VkFormat candidateFormats[candidate_count], VkImageTiling vkImageTiling, VkFormatFeatureFlags vkFormatFeatureFlags) {
			for (size_t i = 0; i < candidate_count; i++) {
				VkFormatProperties formatProperties;
				vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, candidateFormats[i], &formatProperties);
				switch (vkImageTiling) {
					case VK_IMAGE_TILING_OPTIMAL:
						if ((formatProperties.optimalTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags) {
							return candidateFormats[i];
						}
						break;
					case VK_IMAGE_TILING_LINEAR:
						if ((formatProperties.linearTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags) {
							return candidateFormats[i];
						}
						break;
					default:
						break;
				}
			}
			return VK_FORMAT_UNDEFINED;
		};
	}
}
