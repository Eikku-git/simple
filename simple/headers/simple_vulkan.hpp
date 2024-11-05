#pragma once

#include "vulkan/vulkan.h"
#include "simple_dynamic_array.hpp"

namespace simple {
	namespace vulkan {
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
