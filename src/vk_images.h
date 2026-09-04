#pragma once 

#include <vulkan/vulkan.h>
#include "vk_types.h"

namespace vkutil {

	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
	void generate_mips(VkCommandBuffer cmd, VkImage source, VkExtent3D extent, uint32_t mip_levels);

	AllocatedImage create_image(const Context& context, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped,
		VkSampleCountFlagBits MSAA_samples);
	AllocatedImage create_image(const Context& context, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped,
		VkSampleCountFlagBits MSAA_samples);
	void destroy_image(const Context& context, AllocatedImage& img);

	AllocatedImage create_skybox(const Context& context, VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);
	AllocatedImage create_skybox(const Context& context, std::vector<void*> data, VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);

}