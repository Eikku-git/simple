#define EDITOR
#include "simple.hpp"
#include "simple_field.hpp"

struct TestCamera {

	simple::Simple& engine;
	simple::Backend& backend;
	simple::Field<void*> thisField{};
	simple::Field<FIFarray(VkImageView)> colorImageViews{};
	SimpleArray(simple::RenderingAttachment, 1) colorAttachment{};
	simple::Array<simple::Image, FramesInFlight> depthImages{};
	simple::Field<FIFarray(VkImageView)> depthImageViews{};
	simple::RenderingAttachment depthAttachment{};
	simple::RenderingContext renderingContext{};

	inline TestCamera(simple::Simple& engine) noexcept : engine(engine), backend(engine.GetBackend()) {
		thisField.value = this;
		const FIFarray(VkImageView)& swapchainImageViews = backend.GetSwapchainImageViews();
		depthAttachment.vkImageViewsRef.SetField(depthImageViews);
		for (size_t i = 0; i < FramesInFlight; i++) {
			colorImageViews.value[i] = swapchainImageViews[i];
			depthImages[i].Init(engine);
			depthImages[i].CreateImage(backend.GetSwapchainImageExtent(), backend.GetDepthOnlyImageFormat(),
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 1, simple::ImageSampleBits::VK_SAMPLE_COUNT_1_BIT);
			simple::ImageSubResourceRange depthImageSubresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
			depthImages[i].TransitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthImageSubresourceRange);
			depthImageViews.value[i] = depthImages[i].CreateVkImageView(depthImageSubresourceRange);
			VkRenderingAttachmentInfo& vkDepthAttachmentInfo = depthAttachment.GetInfo(i);
			vkDepthAttachmentInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.pNext = nullptr,
				.imageView = VK_NULL_HANDLE,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.resolveImageView = VK_NULL_HANDLE,
				.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue = { .depthStencil { .depth { 1.0f }, .stencil { 0 } } }
			};
			vkDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			vkDepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vkDepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			vkDepthAttachmentInfo.clearValue = { .depthStencil { .depth = 1.0f, .stencil = 0 } };
		}
		engine.AddSwapchainRecreateCallback(SwapchainRecreateCallback, thisField);
		renderingContext.Init<0>(nullptr, & depthAttachment, nullptr);
		renderingContext.SetRenderArea({ .offset = { 0, 0 }, .extent = backend.GetSwapchainExtent() });
	}

	inline void OnRender() {
		backend.PushRenderingContextToRendering(&renderingContext);
	}

	inline void Terminate() {
		for (size_t i = 0; i < FramesInFlight; i++) {
			engine.DestroyVkImageView(depthImageViews.value[i]);
			depthImages[i].Terminate();
			(&thisField)->~Field();
		}
	}

	static inline void SwapchainRecreateCallback(simple::Field<void*>& field, uint32_t width, uint32_t height, const FIFarray(VkImageView)& imageViews) {
		TestCamera* test = (TestCamera*)field.value;
		for (size_t i = 0; i < FramesInFlight; i++) {
			test->colorImageViews.value[i] = imageViews[i];
			test->engine.DestroyVkImageView(test->depthImageViews.value[i]);
			test->depthImages[i].Terminate();
			test->depthImages[i].CreateImage({ width, height, 1 }, test->backend.GetDepthOnlyImageFormat(),
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 1, simple::ImageSampleBits::VK_SAMPLE_COUNT_1_BIT);
			simple::ImageSubResourceRange depthImageSubresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
			test->depthImages[i].TransitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthImageSubresourceRange);
			test->depthImageViews.value[i] = test->depthImages[i].CreateVkImageView(depthImageSubresourceRange);
		}
		test->renderingContext.SetRenderArea({ .offset = { 0, 0 }, .extent = { width, height } });
	}
};

int main() {
	simple::WindowSystem::Init();
	simple::Window window{};
	window.Init(540, 540, "Test", nullptr);
	simple::Simple engine(std::move(window));
	TestCamera camera(engine);
	while (!engine.Quitting()) {
		if (!engine.EditorUpdate()) {
			break;
		}
		engine.LogicUpdate();
		camera.OnRender();
		engine.Render();
	}
	camera.Terminate();
	simple::WindowSystem::Terminate();
	return 0;
}