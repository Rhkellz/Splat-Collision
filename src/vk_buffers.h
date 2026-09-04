#pragma once

#include <vulkan/vulkan.h>
#include "vk_types.h"

namespace vkutil {
	AllocatedBuffer create_buffer(const Context& context, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	void destroy_buffer(const Context& context, AllocatedBuffer buffer);
}