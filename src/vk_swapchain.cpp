#include "vk_swapchain.h"

void VulkanSwapchain::init_swapchain(Context context) {
	_context = context;
	create_swapchain(_context.window_extent.width, _context.window_extent.height);
}

void VulkanSwapchain::create_swapchain(uint32_t width, uint32_t height) {
	vkb::SwapchainBuilder swapchainBuilder{ _context.chosen_GPU, _context.device, _context.surface };

	_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	VkSurfaceFormatKHR desiredFormat{};
	desiredFormat.format = _swapchain_image_format;
	desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainBuilder.set_desired_format(desiredFormat);

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(desiredFormat)
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // important, vsync
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchain_extent = vkbSwapchain.extent;
	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchain_images = vkbSwapchain.get_images().value();
	_swapchain_image_views = vkbSwapchain.get_image_views().value();
}

void VulkanSwapchain::destroy_swapchain() {
	vkDestroySwapchainKHR(_context.device, _swapchain, nullptr);

	for (size_t i = 0; i < _swapchain_images.size(); i++) {
		vkDestroyImageView(_context.device, _swapchain_image_views[i], nullptr);//dont need to destroy _swapchain_images as vkDestroySwapchainKHR already does
	}
}

void VulkanSwapchain::resize_swapchain(bool& resize_requested) {
	vkDeviceWaitIdle(_context.device);

	destroy_swapchain();

	int w, h;
	SDL_GetWindowSize(_context.window, &w, &h);
	_context.window_extent.width = w;
	_context.window_extent.height = h;

	create_swapchain(_context.window_extent.width, _context.window_extent.height);

	resize_requested = false;
}

void VulkanSwapchain::init_present_semaphores(const VkSemaphoreCreateInfo& semaphore_create_info) {// must be called after swapchain init!
	if (_swapchain_images.empty()) {
		throw std::runtime_error("init_present_semaphores() called before init swapchain!");
	}

	_present_semaphores.assign(_swapchain_images.size(), VK_NULL_HANDLE);// init the vector
	
	for (uint32_t i = 0; i < _swapchain_images.size(); i++) {
		vkCreateSemaphore(_context.device, &semaphore_create_info, nullptr, &_present_semaphores[i]);
	}
}

void VulkanSwapchain::destroy_present_semaphores() {
	for (uint32_t i = 0; i < _present_semaphores.size(); i++) {
		vkDestroySemaphore(_context.device, _present_semaphores[i], nullptr);
	}

	_present_semaphores.clear();
}