#include "vk_buffers.h"

AllocatedBuffer vkutil::create_buffer(const Context& context, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	// create info
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(context.allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info));

	return newBuffer;
}

void vkutil::destroy_buffer(const Context& context, AllocatedBuffer buffer) {
	vmaDestroyBuffer(context.allocator, buffer.buffer, buffer.allocation);
}
