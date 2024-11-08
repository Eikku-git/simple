#pragma once

#include "simple_macros.hpp"
#include "simple_algorithm.hpp"
#include "simple_dynamic_array.hpp"
#include "simple_window.hpp"
#include "simple_logging.hpp"
#include "simple_array.hpp"
#include "simple_set.hpp"
#include "simple_map.hpp"
#include "simple_UID.hpp"
#include "simple_field.hpp"
#include "simple_vulkan.hpp"
#include <assert.h>
#include <thread>
#include <mutex>

namespace simple {

	class Simple;
	class Backend;
	typedef uint32_t Flags;

	typedef VkSampleCountFlagBits ImageSampleBits;
	typedef VkFormat ImageFormat;
	typedef Flags ImageSamples;
	typedef VkExtent3D ImageExtent;
	typedef VkExtent2D Extent2D;
	typedef VkRect2D RenderArea;
	typedef Flags ImageUsageFlags;
	typedef Flags ImageAspectFlags;
	typedef VkImageTiling ImageTiling;
	typedef VkImageType ImageType;
	typedef VkImageViewType ImageViewType;
	typedef VkImageLayout ImageLayout;
	typedef VkImageSubresourceRange ImageSubResourceRange;
	typedef VkComponentMapping ImageComponentMapping;

	typedef std::mutex Mutex;
	typedef std::lock_guard<Mutex> LockGuard;

	class Thread {
	public:

		typedef std::thread::id ID;

		Thread() noexcept : _stdThread(std::thread{}) {}

		inline Thread(Thread&& other) noexcept
			: _stdThread(std::move(other._stdThread)), _ID(other._ID),
				_vkGraphicsCommandPool(other._vkGraphicsCommandPool), _vkTransferCommandPool(other._vkTransferCommandPool) {
			other._vkGraphicsCommandPool = VK_NULL_HANDLE;
			other._vkTransferCommandPool = VK_NULL_HANDLE;
		}

		inline Thread(std::thread&& thread) : _stdThread(std::move(thread)) {
			_ID = _stdThread.get_id();
		}

		inline ID GetID() const {
			return _ID;
		}

		inline VkCommandPool GetGraphicsCommandPool() const {
			return _vkGraphicsCommandPool;
		}

		inline VkCommandPool GetTransferCommandPool() const {
			return _vkTransferCommandPool;
		}

		inline bool operator==(const Thread& other) const {
			return _ID == other._ID;
		}

		struct Hash {

			inline size_t operator()(const Thread& thread) {
				return std::hash<std::thread::id>()(thread._ID);
			}

			inline size_t operator()(std::thread::id thread) {
				return std::hash<std::thread::id>()(thread);
			}
		};

	private:

		ID _ID{};
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

	static inline bool Succeeded(VkResult vkResult) {
		return vkResult == VK_SUCCESS;
	}

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

	struct RenderingAttachment {
	public:

		RenderingAttachment() noexcept : vkImageViewsRef() {}
		RenderingAttachment(Field<FIFarray(VkImageView)>::Reference& imageViewsRef) noexcept : vkImageViewsRef(imageViewsRef) {}
		RenderingAttachment(Field<FIFarray(VkImageView)>& imageViewsField) noexcept : vkImageViewsRef(imageViewsField) {}

		const VkRenderingAttachmentInfo& GetInfo(uint32_t currentRenderFrame) {
			assert(!vkImageViewsRef.IsNull() && "vkImageViewsRef was null (function simple::RenderingAttachment::GetInfo)!");
			vkRenderingAttachmentInfo.imageView = vkImageViewsRef.GetField().value[currentRenderFrame];
			return vkRenderingAttachmentInfo;
		}
		Field<FIFarray(VkImageView)>::Reference vkImageViewsRef;
	private:
		VkRenderingAttachmentInfo vkRenderingAttachmentInfo {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO
		};
	};

	class RenderingContext {
	public:

		struct Mesh {

			Mesh** reference;
			uint64_t UID;
			uint32_t vertexVkBufferCount;
			VkBuffer* vertexVkBuffers;
			VkDeviceSize* vertexBufferOffsets;
			VkBuffer indexVkBuffer;

			inline Mesh(Mesh** reference, uint64_t UID, uint32_t vertexVkBufferCount, VkBuffer* vertexVkBuffers, VkDeviceSize* vertexBufferOffsets, VkBuffer indexVkBuffer) noexcept
				: reference(reference), UID(UID), vertexVkBufferCount(vertexVkBufferCount), vertexVkBuffers(vertexVkBuffers), 
					vertexBufferOffsets(vertexBufferOffsets), indexVkBuffer(indexVkBuffer) {}

			inline Mesh(Mesh&& other) noexcept
				: reference(other.reference), UID(other.UID), vertexVkBufferCount(other.vertexVkBufferCount), vertexVkBuffers(other.vertexVkBuffers),
					indexVkBuffer(other.indexVkBuffer) {
				other.reference = nullptr;
				other.UID = 0;
				other.vertexVkBufferCount = 0;
				other.vertexVkBuffers = VK_NULL_HANDLE;
				indexVkBuffer = VK_NULL_HANDLE;
				*reference = this;
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
				LockGuard lockGuard(other._meshesMutex);
				_meshUIDState = other._meshUIDState.load();
				new(&_meshes) DynamicArray<Mesh>(std::move(other._meshes));
				*_reference = this;
				other._reference = nullptr;
				other._UID = 0;
			}

			template<uint32_t T_vertex_buffer_count>
			inline const Mesh* AddMesh(Mesh** meshReference, VkBuffer vertexBuffers[T_vertex_buffer_count], VkDeviceSize vertexBufferOffsets[T_vertex_buffer_count], VkBuffer indexBuffer) noexcept {
				LockGuard lockGuard(_meshesMutex);
				return &_meshes.EmplaceBack(meshReference, UID::Shuffle(_meshUIDState), T_vertex_buffer_count, vertexBuffers, vertexBufferOffsets, indexBuffer);
			}

			inline bool RemoveMesh(const Mesh& mesh) {
				LockGuard lockGuard(_meshesMutex);
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
			VkClearValue clearValue{};
			uint64_t _UID;
			uint32_t _vkDescriptorSetCount;
			VkDescriptorSet* _vkDescriptorSets;
			DynamicArray<Mesh> _meshes{};
			Mutex _meshesMutex{};
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
				LockGuard lockGuard(other._shaderObjectsMutex);
				_shaderObjectsUIDState = other._shaderObjectsUIDState.load();
				new(&_shaderObjects) DynamicArray<ShaderObject>(std::move(other._shaderObjects));
				*_reference = this;
				other._reference = nullptr;
				other._UID = 0;
			}

			template<uint32_t T_descriptor_set_count>
			inline const ShaderObject* AddShaderObject(ShaderObject** shaderObjectReference, VkDescriptorSet vkDescriptorSets[T_descriptor_set_count]) {
				LockGuard lockGuard(_shaderObjectsMutex);
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
			Mutex _shaderObjectsMutex{};
			std::atomic<uint64_t> _shaderObjectsUIDState;

			friend class Backend;
		};

		template<uint32_t T_color_attachment_count>
		void Init(const SimpleArray(RenderingAttachment, T_color_attachment_count)* colorAttachments, 
			RenderingAttachment* pDepthAttachment, RenderingAttachment* pStencilAttachment) {
			_colorAttachmentCount = T_color_attachment_count;
			_pColorAttachments = colorAttachments ? colorAttachments->Data() : nullptr;
			_vkColorAttachments.Resize(T_color_attachment_count);
			_pDepthAttachment = pDepthAttachment;
			_pStencilAttachment = pStencilAttachment;
		}

		inline const Pipeline* AddPipeline(Pipeline** pipelineReference, VkPipeline vkPipeline, VkPipelineLayout vkPipelineLayout) {
			LockGuard lockGuard(_pipelinesMutex);
			return &_pipelines.EmplaceBack(pipelineReference, UID::Shuffle(_pipelinesUIDState), vkPipeline, vkPipelineLayout);
		}

		inline bool RemovePipeline(const Pipeline& pipeline) noexcept {
			LockGuard lockGuard(_pipelinesMutex);
			auto iter = _pipelines.Find(pipeline);
			if (iter == _pipelines.end()) {
				logError(this, "failed to remove mesh (function simple::RenderingContext::ShaderObject::RemoveMesh), may indicate invalid simple::RenderingContext::Mesh handle");
				return false;
			}
			_pipelines.Erase(iter);
			return true;
		}

		inline void SetRenderArea(const RenderArea& renderArea) noexcept {
			_renderArea = renderArea;
		}

		inline VkRenderingAttachmentInfo* GetColorAttachments(uint32_t currentRenderFrame) {
			for (uint32_t i = 0; i < _colorAttachmentCount; i++) {
				_vkColorAttachments[i] = _pColorAttachments[i].GetInfo(currentRenderFrame);
			}
			return _vkColorAttachments.Data();
		}

	private:

		RenderArea _renderArea{};
		uint32_t _colorAttachmentCount{};
		RenderingAttachment* _pColorAttachments{};
		DynamicArray<VkRenderingAttachmentInfo> _vkColorAttachments{};
		RenderingAttachment* _pDepthAttachment{};
		RenderingAttachment* _pStencilAttachment{};

		DynamicArray<Pipeline> _pipelines{};
		Mutex _pipelinesMutex{};
		std::atomic<uint64_t> _pipelinesUIDState;

		friend class Backend;
	};

	typedef void (*SwapchainRecreateCallback)(Field<void*>& field, uint32_t width, uint32_t height, const FIFarray(VkImageView)& swapchainViews);

	class Backend {
	public:

		static constexpr inline VkPipelineStageFlags image_transition_src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		static constexpr inline VkPipelineStageFlags image_transition_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		static constexpr inline uint32_t max_active_rendering_context_count = 512;

		Backend(Simple& engine);

		constexpr inline const Thread& GetMainThread() const {
			return _mainThread;
		}

		inline const Thread* GetThisThread() {
			Thread::ID threadID = std::this_thread::get_id();
			if (threadID == _mainThread._ID) {
				return &_mainThread;
			}
			LockGuard lockGuard(_threadsMutex);
			auto pair = _threads.Find(threadID);
			if (pair) {
				return &pair->second;
			}
			return nullptr;
		}

		constexpr inline ImageExtent GetSwapchainImageExtent() const {
			return ImageExtent(_swapchainVkExtent2D.width, _swapchainVkExtent2D.height, 1);
		}

		constexpr inline const FIFarray(VkImageView)& GetSwapchainImageViews() const {
			return _swapchainImageViews;
		}

		constexpr inline ImageFormat GetSwapchainImageFormat() const {
			return (ImageFormat)_vkSurfaceFormatKHR.format;
		}
		
		constexpr inline ImageFormat GetDepthOnlyImageFormat() const {
			return _depthOnlyFormat;
		}

		constexpr inline ImageFormat GetStencilImageFormat() const {
			return _stencilFormat;
		}

		constexpr inline ImageFormat GetDepthStencilImageFormat() const {
			return _depthStencilFormat;
		}

		inline bool PushRenderingContextToRendering(RenderingContext* pRenderingContext) {
			LockGuard lockGuard(_activeRenderingContextsMutex);
			if (Backend::max_active_rendering_context_count <= _activeRenderingContextCount) {
				return false;
			}
			_activeRenderingContexts[_activeRenderingContextCount++] = pRenderingContext;
			return true;
		}

		inline bool ThreadExists(std::thread& thread) {
			LockGuard lockGuard(_threadsMutex);
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
		Map<Thread::ID, Thread, Thread::Hash> _threads{};
		Mutex _threadsMutex{};
		Thread _mainThread{};
		DynamicArray<VkCommandBuffer> _queuedGraphicsCommandBuffers{};
		Mutex _queuedGraphicsCommandBuffersMutex{};
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
		FIFarray(VkSemaphore) _frameReadyVkSemaphores{};
		FIFarray(VkSemaphore) _frameFinishedVkSemaphores{};
		FIFarray(VkFence) _inFlightVkFences{};
		FIFarray(VkCommandBuffer) _renderingVkCommandBuffers{};
		VkSwapchainKHR _vkSwapchainKHR{};
		VkExtent2D _swapchainVkExtent2D{};
		VkSurfaceFormatKHR _vkSurfaceFormatKHR{};
		VkPresentModeKHR _vkPresentModeKHR{};
		FIFarray(VkImage) _swapchainImages{};
		FIFarray(VkImageView) _swapchainImageViews{};
		ImageFormat _depthOnlyFormat{};
		ImageFormat _stencilFormat{};
		ImageFormat _depthStencilFormat{};

		std::atomic<uint32_t> _activeRenderingContextCount{};
		SimpleArray(RenderingContext*, max_active_rendering_context_count) _activeRenderingContexts{};
		Mutex _activeRenderingContextsMutex{};

		DynamicArray<Pair<SwapchainRecreateCallback, Field<void*>::Reference>> _swapchainRecreateCallbacks{};
		Mutex _swapchainRecreateCallbacksMutex{};

		uint32_t _currentRenderFrame{};

		inline Thread* _NewThread(std::thread&& thread) {

			LockGuard lockGuard(_threadsMutex);

			Thread::ID threadID = thread.get_id();

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
			LockGuard lockGuard(_queuedGraphicsCommandBuffersMutex);
			_queuedGraphicsCommandBuffers.PushBack(commandBuffer);
		}

		inline simple::DynamicArray<VkCommandBuffer>&& _MoveGraphicsCommandBuffers() {
			LockGuard lockGuard(_queuedGraphicsCommandBuffersMutex);
			return std::move(_queuedGraphicsCommandBuffers);
		}

		void _CreateSwapchain();

		inline void _RecreateSwapchain() {
			vkDestroySwapchainKHR(_vkDevice, _vkSwapchainKHR, _vkAllocationCallbacks);
			for (uint32_t i = 0; i < FramesInFlight; i++) {
				vkDestroyImageView(_vkDevice, _swapchainImageViews[i], _vkAllocationCallbacks);
			}
			_CreateSwapchain();
			auto iter = _swapchainRecreateCallbacks.begin();
			for (; iter != _swapchainRecreateCallbacks.end();) {
				if (iter->second.IsNull()) {
					iter = _swapchainRecreateCallbacks.Erase(iter);
					continue;
				}
				iter->first(iter->second.GetField(), _swapchainVkExtent2D.width, _swapchainVkExtent2D.height, _swapchainImageViews);
				++iter;
			}
		}

		inline void _AddSwapchainRecreateCallback(SwapchainRecreateCallback callback, simple::Field<void*>& caller) {
			assert(callback && "attempting to add swapchain recreate callback (function simple::Backend::_AddSwapchainRecreateCallback) with a function that's null!");
			LockGuard lockGuard(_swapchainRecreateCallbacksMutex);
			_swapchainRecreateCallbacks.EmplaceBack(callback, caller);
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

			LockGuard activeRenderingContextsLock(_activeRenderingContextsMutex);
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
					.pColorAttachments = context->GetColorAttachments(_currentRenderFrame),
					.pDepthAttachment = context->_pDepthAttachment ? &context->_pDepthAttachment->GetInfo(_currentRenderFrame) : nullptr,
					.pStencilAttachment = context->_pStencilAttachment ? &context->_pStencilAttachment->GetInfo(_currentRenderFrame) : nullptr,
				};
				vkCmdBeginRendering(renderCommandBuffer, &vkRenderingInfo);
				for (RenderingContext::Pipeline& pipeline : _activeRenderingContexts[i]->_pipelines) {
					vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._vkPipeline);
					for (RenderingContext::ShaderObject& shaderObject : pipeline._shaderObjects)  {
						vkCmdBindDescriptorSets(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._vkPipelineLayout,
							0, shaderObject._vkDescriptorSetCount, shaderObject._vkDescriptorSets, 0, nullptr);
						for (RenderingContext::Mesh& mesh : shaderObject._meshes) {
							vkCmdBindVertexBuffers(renderCommandBuffer, 0, mesh.vertexVkBufferCount, mesh.vertexVkBuffers, mesh.vertexBufferOffsets);
							vkCmdBindIndexBuffer(renderCommandBuffer, mesh.indexVkBuffer, 0, VK_INDEX_TYPE_UINT32);
						}
					}
				}
				vkCmdEndRendering(renderCommandBuffer);
				_activeRenderingContexts[i] = nullptr;
			}

			_activeRenderingContextCount = 0;

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
			assert(imageIndex < FramesInFlight);
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

			SimpleArray(VkSubmitInfo, 2) graphicsVkSubmitInfos = { graphicsVkSubmitInfo, renderVkSubmitInfo };

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

			_currentRenderFrame = (_currentRenderFrame + 1) % FramesInFlight;

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
			LockGuard lockGuard(_threadsMutex);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkGraphicsCommandPool, _vkAllocationCallbacks);
			vkDestroyCommandPool(_vkDevice, _mainThread._vkTransferCommandPool, _vkAllocationCallbacks);
			for (auto& pair : _threads) {
				pair.second._stdThread.join();
				vkDestroyCommandPool(_vkDevice, pair.second._vkGraphicsCommandPool, _vkAllocationCallbacks);
				vkDestroyCommandPool(_vkDevice, pair.second._vkTransferCommandPool, _vkAllocationCallbacks);
			}
			vkDeviceWaitIdle(_vkDevice);
			for (size_t i = 0; i < FramesInFlight; i++) {
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

		inline bool Quitting() {
			WindowSystem::PollEvents();
			return glfwWindowShouldClose(_window.GetRawPointer());
		}

		inline bool AddSwapchainRecreateCallback(SwapchainRecreateCallback callback, simple::Field<void*>& caller) {
			if (!callback) {
				logError(this, "attempting to add swapchain recreate callback (function simple::Simple::AddSwapchainRecreateCallback) with a function that's null!");
				return false;
			}
			_backend._AddSwapchainRecreateCallback(callback, caller);
			return true;
		}

		inline void DestroyVkImageView(VkImageView& vkImageView) {
			vkDestroyImageView(_backend._vkDevice, vkImageView, _backend._vkAllocationCallbacks);
			vkImageView = nullptr;
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
		
	class Image {
	public:

		static constexpr inline VkComponentSwizzle component_swizzle_identity = VK_COMPONENT_SWIZZLE_IDENTITY;

		inline Image() noexcept : _pEngine(nullptr) {}

		inline Image(Simple& pEngine) noexcept : _pEngine(&pEngine) {}

		inline Image(Image&& other) noexcept;

		inline void Init(Simple& engine) {
			_pEngine = &engine;
		}

		bool IsNull() const noexcept {
			return _vkImage == VK_NULL_HANDLE;
		}

		inline bool CreateImage(ImageExtent extent, ImageFormat format, ImageUsageFlags usage, uint32_t mipLevels = 1,
			uint32_t arrayLayers = 1, ImageSampleBits samples = VK_SAMPLE_COUNT_1_BIT, 
			ImageTiling tiling = ImageTiling::VK_IMAGE_TILING_OPTIMAL, 
			ImageType type = ImageType::VK_IMAGE_TYPE_2D) noexcept {
			if (!IsNull()) {
				logError(this, "attempting to create image (function simple::Image::CreateImage) when an image is already created and not terminated!");
				return false;
			}
			VkDeviceSize deviceSize = (VkDeviceSize)extent.width * extent.height;
			return Succeeded(_CreateImage(nullptr, 0, static_cast<VkImageType>(type), static_cast<VkFormat>(format),
				extent, mipLevels, arrayLayers, static_cast<VkSampleCountFlagBits>(samples), static_cast<VkImageTiling>(tiling), usage,
				VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, static_cast<VkImageLayout>(VK_IMAGE_LAYOUT_UNDEFINED), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		}

		inline VkResult _CreateImage(const void* pNext, VkImageCreateFlags flags, VkImageType imageType, VkFormat format, 
			VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples,
			VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount,
			const uint32_t* pQueueFamilyIndices, VkImageLayout initialLayout, VkMemoryPropertyFlags vkMemoryProperties) {
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
			VkResult vkResult = vkCreateImage(_pEngine->_backend._vkDevice, &createInfo, _pEngine->_backend._vkAllocationCallbacks, &_vkImage);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to create VkImage (function vkCreateImage) for simple::Image");
				return vkResult;
			}
			VkMemoryRequirements vkMemRequirements;
			vkGetImageMemoryRequirements(_pEngine->_backend._vkDevice, _vkImage, &vkMemRequirements);
			VkMemoryAllocateInfo vkAllocInfo {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.pNext = nullptr,
				.allocationSize = vkMemRequirements.size,
			};
			if (!vulkan::FindMemoryType(_pEngine->_backend._vkPhysicalDevice, vkMemRequirements.memoryTypeBits, vkMemoryProperties, vkAllocInfo.memoryTypeIndex)) {
				logError(this, "failed to find memory type (function simple::vulkan::FindMemoryType) to allocate memory for simple::Image");
				vkDestroyImage(_pEngine->_backend._vkDevice, _vkImage, _pEngine->_backend._vkAllocationCallbacks);
				return VK_RESULT_MAX_ENUM;
			}
			vkResult = vkAllocateMemory(_pEngine->_backend._vkDevice, &vkAllocInfo, _pEngine->_backend._vkAllocationCallbacks, &_vkDeviceMemory);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to allocate memory (function vkAllocateMemory) for simple::Image");
				vkDestroyImage(_pEngine->_backend._vkDevice, _vkImage, _pEngine->_backend._vkAllocationCallbacks);
				return vkResult;
			}
			vkResult = vkBindImageMemory(_pEngine->_backend._vkDevice, _vkImage, _vkDeviceMemory, 0);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to bind image memory (function vkBindImageMemory) for simple::Image");
				vkDestroyImage(_pEngine->_backend._vkDevice, _vkImage, _pEngine->_backend._vkAllocationCallbacks);
				vkFreeMemory(_pEngine->_backend._vkDevice, _vkDeviceMemory, _pEngine->_backend._vkAllocationCallbacks);
				return vkResult;
			}
			_layout = static_cast<ImageLayout>(initialLayout);
			_extent = extent;
			_arrayLayers = arrayLayers;
			_format = static_cast<ImageFormat>(format);
			return VK_SUCCESS;
		}

		inline VkImageView CreateVkImageView(const ImageSubResourceRange& subresourceRange, ImageViewType viewType = ImageViewType::VK_IMAGE_VIEW_TYPE_2D, const ImageComponentMapping& components = {}) const noexcept {
			if (IsNull()) {
				logError(this, "attempting to create VkImageView with simple::Image that's null!");
				return VK_NULL_HANDLE;
			}
			VkImageView vkImageView;
			if (Succeeded(_CreateVkImageView(nullptr, 0, static_cast<VkImageViewType>(viewType), components, subresourceRange, vkImageView))) {
				return vkImageView;
			}
			return VK_NULL_HANDLE;
		}

		inline VkResult _CreateVkImageView(const void* pNext, VkImageViewCreateFlags flags, VkImageViewType viewType,
			VkComponentMapping components, VkImageSubresourceRange subresourceRange, VkImageView& out) const {
			assert(!IsNull() && "attempting to create VkImageView with simple::Image that's null!");
			VkImageViewCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = pNext,
				.flags = flags,
				.image = _vkImage,
				.viewType = viewType,
				.format = static_cast<VkFormat>(_format),
				.components = components,
				.subresourceRange = subresourceRange
			};
			VkResult vkResult = vkCreateImageView(_pEngine->_backend._vkDevice, &createInfo, _pEngine->_backend._vkAllocationCallbacks, &out);
			if (!Succeeded(vkResult)) {
				logError(this, "failed to create VkImageView (function vkCreateImageView) for simple::ImageView");
				return vkResult;
			}
			return VK_SUCCESS;
		}

		inline bool TransitionLayout(ImageLayout) {
			const Thread* thread = _pEngine->_backend.GetThisThread();
		}

		void Terminate() noexcept {
			vkFreeMemory(_pEngine->_backend._vkDevice, _vkDeviceMemory, _pEngine->_backend._vkAllocationCallbacks);
			_vkDeviceMemory = VK_NULL_HANDLE;
			vkDestroyImage(_pEngine->_backend._vkDevice, _vkImage, _pEngine->_backend._vkAllocationCallbacks);
			_vkImage = VK_NULL_HANDLE;
			_layout = ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
			_extent = { 0, 0, 0 };
			_arrayLayers = 0;
			_format = ImageFormat::VK_FORMAT_UNDEFINED;
		}

		inline ~Image() noexcept {
			Terminate();
		}

	private:

		Simple* _pEngine;
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
