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
#include "simple_set.hpp"
#include "vulkan/vulkan.h"
#include <assert.h>
#include <thread>
#include <mutex>
#include <iostream>

namespace simple {

	class Simple;
	class Backend;
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

	namespace vulkan {

		struct PhysicalDeviceInfo {
			VkPhysicalDevice vkPhysicalDevice;
			VkSurfaceKHR vkSurfaceKHR;
			VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
			VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
			DynamicArray<VkExtensionProperties> vkExtensionProperties{};
			DynamicArray<VkQueueFamilyProperties> vkQueueFamilyProperties{};
			uint32_t graphicsQueueFamilyIndex{}, transferQueueFamilyIndex{}, presentQueueFamilyIndex{};
			bool graphicsQueueFound{}, transferQueueFound{}, presentQueueFound{};
			VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKHR;
			DynamicArray<VkSurfaceFormatKHR> vkSurfaceFormatsKHR{};
			DynamicArray<VkPresentModeKHR> vkPresentModesKHR{};

			inline PhysicalDeviceInfo() {}

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
				vkSurfaceFormatsKHR.Resize(vkSurfaceFormatsCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &vkSurfaceFormatsCount, vkSurfaceFormatsKHR.Data());

				uint32_t vkPresentModesCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &vkPresentModesCount, nullptr);
				vkPresentModesKHR.Resize(vkPresentModesCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &vkPresentModesCount, vkPresentModesKHR.Data());

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
							presentQueueFound = true;
						}
					}
					if (graphicsQueueFound && transferQueueFound && presentQueueFound) {
						break;
					}
					++queueFamilyIndex;
				}
			}
		};
	}

	class Thread {
	public:

		Thread() noexcept : _stdThread(std::thread{}) {}

		inline Thread(Thread&& other) noexcept
			: _stdThread(std::move(other._stdThread)), _stdThreadID(other._stdThreadID),
				_vkGraphicsCommandPool(other._vkGraphicsCommandPool), _vkTransferCommandPool(other._vkTransferCommandPool) {
			other._vkGraphicsCommandPool = VK_NULL_HANDLE;
			other._vkTransferCommandPool = VK_NULL_HANDLE;
		}

		inline Thread(std::thread&& thread) : _stdThread(std::move(thread)), _stdThreadID(_stdThread.get_id()) {}

		inline std::thread::id GetID() const {
			return _stdThreadID;
		}

		inline VkCommandPool GetGraphicsCommandPool() const {
			return _vkGraphicsCommandPool;
		}

		inline VkCommandPool GetTransferCommandPool() const {
			return _vkTransferCommandPool;
		}

		inline bool operator==(const Thread& other) const {
			return _stdThreadID == other._stdThreadID;
		}

		struct Hash {

			inline size_t operator()(const Thread& thread) {
				return std::hash<std::thread::id>()(thread._stdThreadID);
			}

			inline size_t operator()(std::thread::id thread) {
				return std::hash<std::thread::id>()(thread);
			}
		};

	private:

		std::thread::id _stdThreadID{};
		std::thread _stdThread;
		VkCommandPool _vkGraphicsCommandPool{};
		VkCommandPool _vkTransferCommandPool{};

		friend class Backend;
	};

	struct Queue {
		VkQueue vkQueue{};
		uint32_t index{};
	};

	enum class ThreadCommandPool {
		None = 0,
		Graphics = 1,
		Transfer = 2,
	};

	class CommandBuffer {
	public:

		inline CommandBuffer(Backend& backend, ThreadCommandPool threadCommandPool, VkCommandPool vkCommandPool) noexcept 
		: _backend(backend), _threadCommandPool(threadCommandPool), _vkCommandPool(vkCommandPool), _vkCommandBuffer(VK_NULL_HANDLE) {}

		void Allocate();
		inline VkCommandBuffer Begin() {
			assert(_vkCommandBuffer != VK_NULL_HANDLE
				&& "attempting to begin a simple::CommandBuffer (function simple::CommandBuffer::Begin) when it hasn't been allocated yet");
			VkCommandBufferBeginInfo beginInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr
			};
			assert(Succeeded(vkBeginCommandBuffer(_vkCommandBuffer, &beginInfo))
				&& "failed to begin simple::CommandBuffer (function simple::CommandBuffer::Begin)");
			return _vkCommandBuffer;
		}
		inline void End() {
			vkEndCommandBuffer(_vkCommandBuffer);
		}
		void Submit();

	private:
		Backend& _backend;
		ThreadCommandPool _threadCommandPool;
		VkCommandPool _vkCommandPool;
		VkCommandBuffer _vkCommandBuffer;
	};

	class Backend {
	public:

		static constexpr inline VkPipelineStageFlags image_transition_src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		static constexpr inline VkPipelineStageFlags image_transition_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		Backend(Simple& engine);

		inline const Thread& GetMainThread() const {
			return _mainThread;
		}

		inline Thread* GetThread(std::thread::id threadID) {
			if (threadID == _mainThread._stdThreadID) {
				return &_mainThread;
			}
			std::lock_guard<std::mutex> lock(_threadsMutex);
			auto bucket = _threads.GetBucket(Thread::Hash()(threadID));
			if (!bucket->second) {
				return nullptr;
			}
			for (size_t i = 0; i < bucket->second; i++) {
				if (bucket->first[i].first.GetID() == threadID) {
					return &bucket->first[i].first;
				}
			}
			return nullptr;
		}

		inline const Thread& NewThread() {
		}

		inline CommandBuffer GetNewGraphicsCommandBuffer(const Thread& thread) {
			return CommandBuffer(*this, ThreadCommandPool::Graphics, thread._vkGraphicsCommandPool);
		}

	private:

		Simple& _engine;
		VkAllocationCallbacks* _vkAllocationCallbacks = VK_NULL_HANDLE;
		Set<Thread, Thread::Hash> _threads{};
		std::mutex _threadsMutex{};
		Thread _mainThread{};
		DynamicArray<VkCommandBuffer> _queuedGraphicsCommandBuffers{};
		std::mutex _queuedGraphicsCommandBuffersMutex{};
		VkInstance _vkInstance{};
		VkPhysicalDevice _vkPhysicalDevice{};
		vulkan::PhysicalDeviceInfo _vulkanPhysicalDeviceInfo;
		VkSurfaceKHR _vkSurfaceKHR{};
		VkDevice _vkDevice{};
		Queue _graphicsQueue{};
		Queue _transferQueue{};
		Queue _presentQueue{};
		ImageSamples _colorMsaaSamples{};
		ImageSamples _depthMsaaSamples{};
		Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameReadyVkSemaphores{};
		Array<VkSemaphore, FRAMES_IN_FLIGHT> _frameFinishedVkSemaphores{};
		Array<VkFence, FRAMES_IN_FLIGHT> _inFlightVkFences{};
		Array<VkCommandBuffer, FRAMES_IN_FLIGHT> _vkRenderingCommandBuffers{};
		VkSwapchainKHR _vkSwapchainKHR{};
		VkExtent2D _swapchainVkExtent2D{};
		VkSurfaceFormatKHR _vkSurfaceFormatKHR{};
		VkPresentModeKHR _vkPresentModeKHR{};
		Array<VkImage, FRAMES_IN_FLIGHT> _swapchainImages{};
		Array<VkImageView, FRAMES_IN_FLIGHT> _swapchainImageViews{};

		inline Thread& _NewThread(std::thread&& thread) {

			std::lock_guard<std::mutex> lock(_threadsMutex);

			Thread newThread(std::move(thread));

			VkCommandPoolCreateInfo graphicsCommandPoolInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = _graphicsQueue.index
			};
			assert(Succeeded(vkCreateCommandPool(_vkDevice, &graphicsCommandPoolInfo, _vkAllocationCallbacks, &newThread._vkGraphicsCommandPool)) && "failed to create vulkan graphics command pool for simple::Thread!");

			VkCommandPoolCreateInfo transformCommandPoolInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = _transferQueue.index
			};
			assert(Succeeded(vkCreateCommandPool(_vkDevice, &transformCommandPoolInfo, _vkAllocationCallbacks, &newThread._vkTransferCommandPool))&& "failed to create vulkan transfer command pool for simple::Thread");

			auto pair = _threads.Emplace(std::move(newThread));
			assert(pair.first && "failed to insert simple::Thread to tracked simple::Backend threads!");

			return *pair.second;
		}

		inline void _QueueGraphicsCommandBuffer(VkCommandBuffer commandBuffer) {
			std::lock_guard<std::mutex> lock(_queuedGraphicsCommandBuffersMutex);
			_queuedGraphicsCommandBuffers.PushBack(commandBuffer);
		}

		void _CreateSwapchain();

		inline void _Terminate() {
			std::lock_guard<std::mutex> lock(_threadsMutex);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkGraphicsCommandPool, _vkAllocationCallbacks);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkTransferCommandPool, _vkAllocationCallbacks);
			for (Thread& thread : _threads) {
				thread._stdThread.join();
				vkDestroyCommandPool(_vkDevice, thread._vkGraphicsCommandPool, _vkAllocationCallbacks);
				vkDestroyCommandPool(_vkDevice, thread._vkTransferCommandPool, _vkAllocationCallbacks);
			}
			vkDeviceWaitIdle(_vkDevice);
			for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
				vkDestroySemaphore(_vkDevice, _frameReadyVkSemaphores[i], _vkAllocationCallbacks);
				vkDestroySemaphore(_vkDevice, _frameFinishedVkSemaphores[i], _vkAllocationCallbacks);
				vkDestroyFence(_vkDevice, _inFlightVkFences[i], _vkAllocationCallbacks);
				vkDestroyImageView(_vkDevice, _swapchainImageViews[i], _vkAllocationCallbacks);
			}
			vkDestroySwapchainKHR(_vkDevice, _vkSwapchainKHR, _vkAllocationCallbacks);
			vkDestroyDevice(_vkDevice, _vkAllocationCallbacks);
		}

		friend class Simple;
		friend class Image;
		friend class ImageView;
		friend class CommandBuffer;
	};


	class Simple {
	public:
		constexpr static inline const char* engine_name = "Simple";
		constexpr static inline const char* app_name = "Simple";

		inline Simple(Window&& window) : _backend(*this), _window(std::move(window)) {}
		Backend& GetBackend();
		~Simple();
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
			return _vkImage == VK_NULL_HANDLE;
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
