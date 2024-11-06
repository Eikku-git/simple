#pragma once

#include "simple.hpp"
#include "simple_algorithm.hpp"
#include "simple_dynamic_array.hpp"
#ifdef FRAMES_IN_FLIGHT
#else
#define FRAMES_IN_FLIGHT 2U
#endif
#include "simple_window.hpp"
#include "simple_logging.hpp"
#include "simple_array.hpp"
#include "simple_set.hpp"
#include "simple_map.hpp"
#include "simple_UID.hpp"
#include "vulkan/vulkan.h"
#include <assert.h>
#include <thread>
#include <mutex>

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

		inline Thread(std::thread&& thread) : _stdThread(std::move(thread)) {
			_stdThreadID = _stdThread.get_id();
		}

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

	typedef VkRenderingAttachmentInfo RenderingAttachment;
	typedef VkRect2D RenderArea;

	class RenderingContext {
	public:

		struct Mesh {

			Mesh** _reference;
			uint64_t UID;
			uint32_t vertexVkBufferCount;
			VkBuffer* vertexVkBuffers;
			VkBuffer indexVkBuffer;

			inline Mesh(Mesh** reference, uint64_t UID, uint32_t vertexVkBufferCount, VkBuffer* vertexVkBuffers, VkBuffer indexVkBuffer) noexcept
				: _reference(reference), UID(UID), vertexVkBufferCount(), vertexVkBuffers(vertexVkBuffers), indexVkBuffer(indexVkBuffer) {}

			inline Mesh(Mesh&& other) noexcept
				: _reference(other._reference), UID(other.UID), vertexVkBufferCount(other.vertexVkBufferCount), vertexVkBuffers(other.vertexVkBuffers),
					indexVkBuffer(other.indexVkBuffer) {
				other._reference = nullptr;
				other.UID = 0;
				other.vertexVkBufferCount = 0;
				other.vertexVkBuffers = VK_NULL_HANDLE;
				indexVkBuffer = VK_NULL_HANDLE;
				*_reference = this;
			}

			constexpr inline bool operator==(const Mesh& other) const {
				return UID == other.UID;
			}
		};

		struct ShaderObject {

			inline ShaderObject(ShaderObject** reference, uint64_t UID, uint32_t vkDescriptorSetCount, VkDescriptorSet* vkDescriptorSets) noexcept 
				: _reference(reference), _UID(UID), _vkDescriptorSetCount(vkDescriptorSetCount), _vkDescriptorSets(vkDescriptorSets), _meshUIDState(UID::GenerateState64()) {}
			
			inline ShaderObject(ShaderObject&& other) noexcept 
				: _reference(other._reference), _UID(other._UID), _vkDescriptorSetCount(other._vkDescriptorSetCount), _vkDescriptorSets(other._vkDescriptorSets),
					_meshUIDState() {
				std::lock_guard<std::mutex> lock(other._meshesMutex);
				_meshUIDState = other._meshUIDState.load();
				new(&_meshes) DynamicArray<Mesh>(std::move(other._meshes));
				*_reference = this;
				other._reference = nullptr;
				other._UID = 0;
			}

			inline const Mesh* AddMesh(Mesh** meshReference, uint32_t vertexBufferCount, VkBuffer* vertexBuffers, VkBuffer indexBuffer) noexcept {
				std::lock_guard<std::mutex> lock(_meshesMutex);
				return &_meshes.EmplaceBack(meshReference, UID::Shuffle(_meshUIDState), vertexBufferCount, vertexBuffers, indexBuffer);
			}

			inline bool RemoveMesh(const Mesh& mesh) {
				std::lock_guard<std::mutex> lock(_meshesMutex);
				auto iter = _meshes.Find(mesh);
				if (iter == _meshes.end()) {
					logError(this, "failed to remove mesh (function simple::RenderingContext::ShaderObject::RemoveMesh), may indicate invalid simple::RenderingContext::Mesh handle");
					return false;
				}
				_meshes.Erase(iter);
				return true;
			}

			struct Hash {
				inline uint64_t operator()(const ShaderObject& shaderObject) {
					return shaderObject._UID;
				}
			};

		private:

			ShaderObject** _reference;
			uint64_t _UID;
			uint32_t _vkDescriptorSetCount;
			VkDescriptorSet* _vkDescriptorSets;
			DynamicArray<Mesh> _meshes{};
			std::mutex _meshesMutex{};
			std::atomic<uint64_t> _meshUIDState;

			friend class Backend;
		};

		struct Pipeline {
		public:	

			inline Pipeline(Pipeline** reference, uint64_t UID, VkPipeline vkPipeline, VkPipelineLayout vkPipelineLayout) noexcept
				: _reference(reference), _UID(UID), _vkPipeline(vkPipeline), _vkPipelineLayout(vkPipelineLayout), _shaderObjectsUIDState(UID::GenerateState64()) {}

			inline Pipeline(Pipeline&& other) noexcept 
				: _reference(other._reference), _UID(other._UID), _vkPipeline(other._vkPipeline), _vkPipelineLayout(other._vkPipelineLayout),
					_shaderObjectsUIDState() {
				std::lock_guard<std::mutex> lock(other._shaderObjectsMutex);
				_shaderObjectsUIDState = other._shaderObjectsUIDState.load();
				new(&_shaderObjects) DynamicArray<ShaderObject>(std::move(other._shaderObjects));
				*_reference = this;
				other._reference = nullptr;
				other._UID = 0;
			}

			template<uint32_t T_descriptor_set_count>
			inline const ShaderObject* AddShaderObject(ShaderObject** shaderObjectReference, VkDescriptorSet vkDescriptorSets[T_descriptor_set_count]) {
				std::lock_guard<std::mutex> lock(_shaderObjectsMutex);
				return *_shaderObjects.EmplaceBack(shaderObjectReference, UID::Shuffle(_shaderObjectsUIDState), T_descriptor_set_count, vkDescriptorSets);
			}

			constexpr inline bool operator==(const Pipeline& other) const {
				return _UID == other._UID;
			}

		private:

			Pipeline** _reference;
			uint64_t _UID;
			VkPipeline _vkPipeline;
			VkPipelineLayout _vkPipelineLayout;

			DynamicArray<ShaderObject> _shaderObjects{};
			std::mutex _shaderObjectsMutex{};
			std::atomic<uint64_t> _shaderObjectsUIDState;

			friend class Backend;
		};

		template<uint32_t T_color_attachment_count>
		inline RenderingContext(const RenderArea& renderArea, RenderingAttachment colorAttachments[T_color_attachment_count], 
			RenderingAttachment* pDepthAttachment, RenderingAttachment* pStencilAttachment) noexcept 
			: _renderArea(renderArea), _colorAttachmentCount(T_color_attachment_count), _pColorAttachments(colorAttachments),
				_pDepthAttachment(pDepthAttachment), _pStencilAttachment(pStencilAttachment), _pipelinesUIDState(UID::GenerateState64()) {}

		inline const Pipeline* AddPipeline(Pipeline** pipelineReference, VkPipeline vkPipeline, VkPipelineLayout vkPipelineLayout) {
			std::lock_guard<std::mutex> lock(_pipelinesMutex);
			return &_pipelines.EmplaceBack(pipelineReference, UID::Shuffle(_pipelinesUIDState), vkPipeline, vkPipelineLayout);
		}

		inline bool RemovePipeline(const Pipeline& pipeline) {
			std::lock_guard<std::mutex> lock(_pipelinesMutex);
			auto iter = _pipelines.Find(pipeline);
			if (iter == _pipelines.end()) {
				logError(this, "failed to remove mesh (function simple::RenderingContext::ShaderObject::RemoveMesh), may indicate invalid simple::RenderingContext::Mesh handle");
				return false;
			}
			_pipelines.Erase(iter);
			return true;
		}

	private:

		RenderArea _renderArea;
		uint32_t _colorAttachmentCount;
		RenderingAttachment* _pColorAttachments;
		RenderingAttachment* _pDepthAttachment;
		RenderingAttachment* _pStencilAttachment;

		DynamicArray<Pipeline> _pipelines{};
		std::mutex _pipelinesMutex{};
		std::atomic<uint64_t> _pipelinesUIDState;

		friend class Backend;
	};

	class Backend {
	public:

		static constexpr inline VkPipelineStageFlags image_transition_src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		static constexpr inline VkPipelineStageFlags image_transition_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		static constexpr inline uint32_t max_active_rendering_context_count = 512;

		Backend(Simple& engine);

		inline const Thread& GetMainThread() const {
			return _mainThread;
		}

		inline const Thread* GetThisThread() {
			std::thread::id threadID = std::this_thread::get_id();
			if (threadID == _mainThread._stdThreadID) {
				return &_mainThread;
			}
			std::lock_guard<std::mutex> lock(_threadsMutex);
			auto pair = _threads.Find(threadID);
			if (pair) {
				return &pair->second;
			}
			return nullptr;
		}

		inline bool ThreadExists(std::thread& thread) {
			std::lock_guard<std::mutex> lock(_threadsMutex);
			return _threads.Contains(thread.get_id());
		}

		inline const Thread& NewThread(std::thread&& thread) {
			return *_NewThread(std::move(thread));
		}

		inline CommandBuffer GetNewGraphicsCommandBuffer(const Thread& thread) {
			return CommandBuffer(*this, ThreadCommandPool::Graphics, thread._vkGraphicsCommandPool);
		}

	private:

		Simple& _engine;
		VkAllocationCallbacks* _vkAllocationCallbacks = VK_NULL_HANDLE;
		Map<std::thread::id, Thread, Thread::Hash> _threads{};
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
		Array<VkCommandBuffer, FRAMES_IN_FLIGHT> _renderingVkCommandBuffers{};
		VkSwapchainKHR _vkSwapchainKHR{};
		VkExtent2D _swapchainVkExtent2D{};
		VkSurfaceFormatKHR _vkSurfaceFormatKHR{};
		VkPresentModeKHR _vkPresentModeKHR{};
		Array<VkImage, FRAMES_IN_FLIGHT> _swapchainImages{};
		Array<VkImageView, FRAMES_IN_FLIGHT> _swapchainImageViews{};

		std::atomic<uint32_t> _activeRenderingContextCount{};
		Array<RenderingContext*, max_active_rendering_context_count> _activeRenderingContexts{};
		std::mutex _activeRenderingContextsMutex{};

		uint32_t _currentRenderFrame{};

		inline Thread* _NewThread(std::thread&& thread) {

			std::lock_guard<std::mutex> lock(_threadsMutex);

			std::thread::id threadID = thread.get_id();

			assert(!_threads.Contains(threadID) && "attempting to create simple::Thread when the thread already exists!");

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

			return new(_threads[threadID]) Thread(std::move(newThread));
		}

		inline void _QueueGraphicsCommandBuffer(VkCommandBuffer commandBuffer) {
			std::lock_guard<std::mutex> lock(_queuedGraphicsCommandBuffersMutex);
			_queuedGraphicsCommandBuffers.PushBack(commandBuffer);
		}

		inline simple::DynamicArray<VkCommandBuffer>&& _MoveGraphicsCommandBuffers() {
			std::lock_guard<std::mutex> lock(_queuedGraphicsCommandBuffersMutex);
			return std::move(_queuedGraphicsCommandBuffers);
		}

		void _CreateSwapchain();

		void _RecreateSwapchain() {
			vkDestroySwapchainKHR(_vkDevice, _vkSwapchainKHR, _vkAllocationCallbacks);
			for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
				vkDestroyImageView(_vkDevice, _swapchainImageViews[i], _vkAllocationCallbacks);
			}
			_CreateSwapchain();
		}

		inline void _RenderCmds() {
			VkCommandBuffer renderCommandBuffer = _renderingVkCommandBuffers[_currentRenderFrame];
			VkCommandBufferBeginInfo renderCommandBufferBeginInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr,
			};

			assert(Succeeded(vkBeginCommandBuffer(renderCommandBuffer, &renderCommandBufferBeginInfo)));

			VkImageMemoryBarrier initialSwapchainMemBarrier {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = _swapchainImages[_currentRenderFrame],
				.subresourceRange {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};

			vkCmdPipelineBarrier(renderCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
				0, 0, nullptr, 0, nullptr, 1, &initialSwapchainMemBarrier);

			VkViewport swapchainViewport {
				.x = 0.0f,
				.y = 0.0f,
				.width = static_cast<float>(_swapchainVkExtent2D.width),
				.height = static_cast<float>(_swapchainVkExtent2D.height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			};

			VkRect2D swapchainScissor {
				.offset = { 0, 0 },
				.extent = _swapchainVkExtent2D,
			};

			vkCmdSetViewport(renderCommandBuffer, 0, 1, &swapchainViewport);
			vkCmdSetScissor(renderCommandBuffer, 0, 1, &swapchainScissor);

			VkRenderingAttachmentInfo swapchainClearColorAttachment {
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.pNext = nullptr,
				.imageView = _swapchainImageViews[_currentRenderFrame],
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.resolveImageView = VK_NULL_HANDLE,
				.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue { 0.0f, 0.0f, 0.0f, 0.0f }
			};

			VkRenderingInfo swapchainClearRenderingInfo {
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderArea { { 0, 0 }, _swapchainVkExtent2D },
				.layerCount = 1,
				.viewMask = 0,
				.colorAttachmentCount = 1,
				.pColorAttachments = &swapchainClearColorAttachment,
				.pDepthAttachment = nullptr,
			};

			vkCmdBeginRendering(renderCommandBuffer, &swapchainClearRenderingInfo);
			vkCmdEndRendering(renderCommandBuffer);

			std::lock_guard<std::mutex> activeRenderingContextsLock(_activeRenderingContextsMutex);
			for (uint32_t i = 0; i < _activeRenderingContextCount; i++) {
				RenderingContext* context = _activeRenderingContexts[i];
				VkRenderingInfo vkRenderingInfo {
					.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
					.pNext = nullptr,
					.flags = 0,
					.renderArea = context->_renderArea,
					.layerCount = 1,
					.viewMask = 0,
					.colorAttachmentCount = context->_colorAttachmentCount,
					.pColorAttachments = context->_pColorAttachments,
					.pDepthAttachment = context->_pDepthAttachment,
					.pStencilAttachment = context->_pStencilAttachment,
				};
				vkCmdBeginRendering(renderCommandBuffer, &vkRenderingInfo);
				vkCmdEndRendering(renderCommandBuffer);
			}

			VkImageMemoryBarrier finalSwapchainMemoryBarrier {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = _swapchainImages[_currentRenderFrame],
				.subresourceRange {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};

			vkCmdPipelineBarrier(renderCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
				0, 0, nullptr, 0, nullptr, 1, &finalSwapchainMemoryBarrier);

			vkEndCommandBuffer(renderCommandBuffer);
		}

		inline void _Render() {
			if (_swapchainVkExtent2D.width == 0 || _swapchainVkExtent2D.height == 0) {
				return;
			}
			vkWaitForFences(_vkDevice, 1, &_inFlightVkFences[_currentRenderFrame], VK_TRUE, UINT64_MAX);
			uint32_t imageIndex;
			VkResult result = vkAcquireNextImageKHR(_vkDevice, _vkSwapchainKHR, UINT64_MAX, _frameReadyVkSemaphores[_currentRenderFrame], nullptr, &imageIndex);
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				_RecreateSwapchain();
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				return;
			}
			assert(imageIndex < FRAMES_IN_FLIGHT);
			vkResetFences(_vkDevice, 1, &_inFlightVkFences[_currentRenderFrame]);

			vkResetCommandBuffer(_renderingVkCommandBuffers[_currentRenderFrame], 0);
			_RenderCmds();
	
			simple::DynamicArray<VkCommandBuffer> graphicsCommandBuffers(std::move(_MoveGraphicsCommandBuffers()));

			VkSubmitInfo graphicsVkSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 0,
				.pWaitSemaphores = nullptr,
				.pWaitDstStageMask = nullptr,
				.commandBufferCount = graphicsCommandBuffers.Size(),
				.pCommandBuffers = graphicsCommandBuffers.Data(),
				.signalSemaphoreCount = 0,
				.pSignalSemaphores = nullptr,
			};

			VkPipelineStageFlags waitStages[1] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

			VkSubmitInfo renderVkSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &_frameReadyVkSemaphores[_currentRenderFrame],
				.pWaitDstStageMask = waitStages,
				.commandBufferCount = 1,
				.pCommandBuffers = &_renderingVkCommandBuffers[_currentRenderFrame],
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &_frameFinishedVkSemaphores[_currentRenderFrame],
			};

			simple::Array<VkSubmitInfo, 2> graphicsVkSubmitInfos = { graphicsVkSubmitInfo, renderVkSubmitInfo };

			assert(Succeeded(vkQueueSubmit(_graphicsQueue.vkQueue, graphicsVkSubmitInfos.Size(), graphicsVkSubmitInfos.Data(), _inFlightVkFences[_currentRenderFrame]))
				&& "failed to submit graphics command buffers (function vkQueueSubmit in simple::Backend::_Render)");

			VkPresentInfoKHR vkPresentInfoKHR {
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &_frameFinishedVkSemaphores[_currentRenderFrame],
				.swapchainCount = 1,
				.pSwapchains = &_vkSwapchainKHR,
				.pImageIndices = &imageIndex,
				.pResults = nullptr,
			};
			result = vkQueuePresentKHR(_graphicsQueue.vkQueue, &vkPresentInfoKHR);

			_currentRenderFrame = (_currentRenderFrame + 1) % FRAMES_IN_FLIGHT;

			switch (result) {
				case VK_SUCCESS:
					return;
				case VK_ERROR_OUT_OF_DATE_KHR:
					_RecreateSwapchain();
					return;
				case VK_SUBOPTIMAL_KHR:
					_RecreateSwapchain();
					return;
				default:
					return;
			}
		}

		inline void _Terminate() {
			vkDeviceWaitIdle(_vkDevice);
			std::lock_guard<std::mutex> lock(_threadsMutex);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkGraphicsCommandPool, _vkAllocationCallbacks);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkTransferCommandPool, _vkAllocationCallbacks);
			for (auto& pair : _threads) {
				pair.second._stdThread.join();
				vkDestroyCommandPool(_vkDevice, pair.second._vkGraphicsCommandPool, _vkAllocationCallbacks);
				vkDestroyCommandPool(_vkDevice, pair.second._vkTransferCommandPool, _vkAllocationCallbacks);
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

		inline Simple(Window&& window) : _backend(*this), _window(std::move(window)) {
			_window._pEngine = this;
			glfwSetWindowSizeCallback(_window._pGlfwWindow,
				[](GLFWwindow* pGlfwWindow, int width, int height) {
					Backend& backend = WindowSystem::FindWindow(pGlfwWindow)._pEngine->_backend;
					if (width == 0 || height == 0) {
						backend._swapchainVkExtent2D = { 0, 0 };
						return;
					}
					backend._RecreateSwapchain();
				}
			);
		}

		inline Backend& GetBackend() {
			return _backend;
		}

		inline bool PushRenderingContext(RenderingContext* pRenderingContext) {
			std::lock_guard<std::mutex> lock(_backend._activeRenderingContextsMutex);
			if (Backend::max_active_rendering_context_count <= _backend._activeRenderingContextCount) {
				return false;
			}
			_backend._activeRenderingContexts[_backend._activeRenderingContextCount++] = pRenderingContext;
			return true;
		}

		inline bool Quitting() {
			WindowSystem::PollEvents();
			return glfwWindowShouldClose(_window.GetRawPointer());
		}

#ifdef EDITOR
		inline bool EditorUpdate() {
			return true;
		}
#endif

		inline void LogicUpdate() {
		}

		inline void Render() {
			_backend._Render();
		}

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
