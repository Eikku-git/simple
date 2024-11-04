#pragma once

#include "simple.hpp"
#include "simple_algorithm.hpp"
#include "simple_dynamic_array.hpp"
#include "vulkan/vulkan_core.h"
#include <cinttypes>
#ifdef FRAMES_IN_FLIGHT
#else
#define FRAMES_IN_FLIGHT 2U
#endif
#include "simple_window.hpp"
#include "simple_logging.hpp"
#include "simple_array.hpp"
#include "vulkan/vulkan.h"
#include <assert.h>
#include <thread>
#include <iostream>

namespace simple {
	
	typedef uint32_t Flags;

	enum ImageSampleBits {
		ImageSample_1Bit = VK_SAMPLE_COUNT_1_BIT,
		ImageSample_2Bit = VK_SAMPLE_COUNT_2_BIT,
		ImageSample_4Bit = VK_SAMPLE_COUNT_4_BIT,
		ImageSample_8Bit = VK_SAMPLE_COUNT_8_BIT,
		ImageSample_16Bit = VK_SAMPLE_COUNT_16_BIT,
		ImageSample_32Bit = VK_SAMPLE_COUNT_32_BIT,
		ImageSample_64Bit = VK_SAMPLE_COUNT_64_BIT,
	};

	enum class ImageFormat {
		Undefined = VK_FORMAT_UNDEFINED,
		R8G8B8A8_SRGB = VK_FORMAT_R8G8B8A8_SRGB,
		B8G8R8A8_SRGB = VK_FORMAT_B8G8R8A8_SRGB,
	};

	typedef Flags ImageSamples;

	static inline bool Succeeded(VkResult vkResult) {
		return vkResult == VK_SUCCESS;
	}

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

	struct Queue {
		VkQueue vkQueue{};
		uint32_t index{};
	};

	class Simple;

	class Backend {
	public:
		Backend(Simple& engine);
		const Thread& GetMainThread();
	private:
		Simple& _engine;
		VkAllocationCallbacks* _vkAllocationCallbacks = VK_NULL_HANDLE;
		Thread _mainThread{};
		VkInstance _vkInstance{};
		VkPhysicalDevice _vkPhysicalDevice{}; 
		VkSurfaceKHR _vkSurfaceKHR{};
		VkDevice _vkDevice{};
		Queue _graphicsQueue{};
		Queue _transferQueue{};
		Queue _presentQueue{};
		ImageSamples _colorMsaaSamples{};
		ImageSamples _depthMsaaSamples{};
		simple::Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameReadySemaphores{};
		simple::Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameFinishedSemaphores{};
		simple::Array<VkFence, FRAMES_IN_FLIGHT> _inFlightFences{};
		simple::Array<VkCommandBuffer, FRAMES_IN_FLIGHT> _renderingCommandBuffers{};
		VkSwapchainKHR _vkSwapchainKHR{};
		VkExtent2D _vkSwapchainExtent2D{};
		VkSurfaceFormatKHR _vkSurfaceFormatKHR{};
		VkPresentModeKHR _vkPresentModeKHR{};
		simple::Array<VkImage, FRAMES_IN_FLIGHT> _swapchainImages{};

		inline void CreateSwapchain() {

		}

		friend class Simple;
		friend class Image;
		friend class ImageView;
	};


	class Simple {
	public:
		constexpr static inline const char* engine_name = "Simple";
		constexpr static inline const char* app_name = "Simple";
		inline Simple(Window&& window) : _backend(*this), _window(std::move(window)) {}
		Backend& GetBackend();
		void Terminate();
	private:
		Window _window;
		Backend _backend;
		friend class Backend;
		friend class Image;
		friend class ImageView;
	};
	
	typedef VkImageUsageFlags ImageUsages;
	typedef VkImageAspectFlags ImageAspects;
	typedef VkExtent3D ImageExtent;

	enum class ImageTiling {
		Optimal = VK_IMAGE_TILING_OPTIMAL,
		Linear = VK_IMAGE_TILING_LINEAR,
	};

	enum class ImageType {
		OneD = VK_IMAGE_TYPE_1D,
		TwoD = VK_IMAGE_TYPE_2D,
		ThreeD = VK_IMAGE_TYPE_3D,
	};

	enum class ImageLayout {
		Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
		General = VK_IMAGE_LAYOUT_GENERAL,
		ColorAttachmentOptimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		DepthStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DepthStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		ShaderReadOnlyOptimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		TransferSrcOptimal = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		TransferDstOptimal = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	};

	typedef VkImageSubresourceRange ImageSubResourceRange;
	typedef VkComponentMapping ImageComponentMapping;

	enum class ImageViewType {
		OneD = VK_IMAGE_VIEW_TYPE_1D,
		TwoD = VK_IMAGE_VIEW_TYPE_2D,
		ThreeD = VK_IMAGE_VIEW_TYPE_3D,
		Cube = VK_IMAGE_VIEW_TYPE_CUBE,
		OneDArray = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
		TwoDArray = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		CubeArray = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
	};

	class Image {
	public:

		inline Image(Simple& engine) noexcept : _engine(engine) {}

		inline Image(Image&& other) noexcept;

		bool IsNull() const noexcept {
			return _vkImage == VK_NULL_HANDLE || _vkDeviceMemory == VK_NULL_HANDLE;
		}

		inline bool CreateImage(ImageExtent extent, ImageFormat format, ImageUsages usage, 
			ImageAspects aspect, uint32_t mipLevels = 1,
			uint32_t arrayLayers = 1, ImageSampleBits samples = ImageSample_1Bit, 
			ImageLayout initialLayout = ImageLayout::Undefined, ImageTiling tiling = ImageTiling::Optimal, 
			ImageType type = ImageType::TwoD) noexcept {
			if (!IsNull()) {
				logError(this, "attempting to create image (function simple::Image::CreateImage) when an image is already created and not terminated!");
				return false;
			}
			VkDeviceSize deviceSize = (VkDeviceSize)extent.width * extent.height;
			return Succeeded(_CreateImage(nullptr, 0, static_cast<VkImageType>(type), static_cast<VkFormat>(format), 
				extent, mipLevels, arrayLayers, static_cast<VkSampleCountFlagBits>(samples), static_cast<VkImageTiling>(tiling), usage,
				VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, static_cast<VkImageLayout>(initialLayout)));
		}

		inline VkResult _CreateImage(const void* pNext, VkImageCreateFlags flags, VkImageType imageType, VkFormat format, 
			VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples,
			VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount,
			const uint32_t* pQueueFamilyIndices, VkImageLayout initialLayout) {
			assert(IsNull() && "attempting to create image for simple::Image that already has a VkImage created!");
			VkImageCreateInfo createInfo {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = pNext,
				.flags = flags,
				.imageType = imageType,
				.format = format,
				.extent = extent,
				.mipLevels = mipLevels,
				.arrayLayers = arrayLayers,
				.samples = samples,
				.tiling = tiling,
				.usage = usage,
				.sharingMode = sharingMode,
				.queueFamilyIndexCount = queueFamilyIndexCount,
				.pQueueFamilyIndices = pQueueFamilyIndices,
				.initialLayout = initialLayout,
			};
			VkResult vkResult = vkCreateImage(_engine._backend._vkDevice, &createInfo, _engine._backend._vkAllocationCallbacks, &_vkImage);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to create VkImage (function vkCreateImage) for simple::Image");
				return vkResult;
			}
			VkMemoryRequirements vkMemRequirements;
			vkGetImageMemoryRequirements(_engine._backend._vkDevice, _vkImage, &vkMemRequirements);
			VkMemoryAllocateInfo vkAllocInfo {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.pNext = nullptr,
				.allocationSize = vkMemRequirements.size,
			};
			vkResult = vkAllocateMemory(_engine._backend._vkDevice, &vkAllocInfo, _engine._backend._vkAllocationCallbacks, &_vkDeviceMemory);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to allocate memory (function vkAllocateMemory) for simple::Image");
				vkDestroyImage(_engine._backend._vkDevice, _vkImage, _engine._backend._vkAllocationCallbacks);
				return vkResult;
			}
			vkResult = vkBindImageMemory(_engine._backend._vkDevice, _vkImage, _vkDeviceMemory, 0);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to bind image memory (function vkBindImageMemory) for simple::Image");
				vkDestroyImage(_engine._backend._vkDevice, _vkImage, _engine._backend._vkAllocationCallbacks);
				vkFreeMemory(_engine._backend._vkDevice, _vkDeviceMemory, _engine._backend._vkAllocationCallbacks);
				return vkResult;
			}
			_layout = static_cast<ImageLayout>(initialLayout);
			_extent = extent;
			_arrayLayers = arrayLayers;
			_format = static_cast<ImageFormat>(format);
			return VK_SUCCESS;
		}

		inline VkImageView CreateImageView(ImageViewType viewType, const ImageComponentMapping& components, const ImageSubResourceRange& subresourceRange) const noexcept {
			if (IsNull()) {
				logError(this, "attempting to create VkImageView with simple::Image that's null!");
				return VK_NULL_HANDLE;
			}
			VkImageView vkImageView;
			if (Succeeded(_CreateImageView(nullptr, 0, static_cast<VkImageViewType>(viewType), components, subresourceRange, vkImageView))) {
				return vkImageView;
			}
			return VK_NULL_HANDLE;
		}

		inline VkResult _CreateImageView(const void* pNext, VkImageViewCreateFlags flags, VkImageViewType viewType,
			VkComponentMapping components, VkImageSubresourceRange subresourceRange, VkImageView& out) const {
			assert(IsNull() && "attempting to create VkImageView with simple::Image that's null!");
			VkImageViewCreateInfo createInfo {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = pNext,
				.flags = flags,
				.image = _vkImage,
				.viewType = viewType,
				.format = static_cast<VkFormat>(_format),
				.components = components,
				.subresourceRange = subresourceRange
			};
			VkResult vkResult = vkCreateImageView(_engine._backend._vkDevice, &createInfo, _engine._backend._vkAllocationCallbacks, &out);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to create VkImageView (function vkCreateImageView) for simple::ImageView");
				return vkResult;
			}
			return VK_SUCCESS;
		}

		void Terminate() noexcept {
			vkFreeMemory(_engine._backend._vkDevice, _vkDeviceMemory, _engine._backend._vkAllocationCallbacks);
			_vkDeviceMemory = VK_NULL_HANDLE;
			vkDestroyImage(_engine._backend._vkDevice, _vkImage, _engine._backend._vkAllocationCallbacks);
			_vkImage = VK_NULL_HANDLE;
			_layout = ImageLayout::Undefined;
			_extent = { 0, 0, 0 };
			_arrayLayers = 0;
			_format = ImageFormat::Undefined;
		}

		inline ~Image() noexcept {
			Terminate();
		}

	private:

		Simple& _engine;
		VkImage _vkImage = VK_NULL_HANDLE;
		VkDeviceMemory _vkDeviceMemory = VK_NULL_HANDLE;
		ImageLayout _layout{};
		ImageExtent _extent{};
		uint32_t _arrayLayers{};
		ImageFormat _format{};

		friend class Simple;
		friend class ImageView;
	};
}
