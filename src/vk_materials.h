#pragma once

#include "vk_types.h"
#include "vk_loader.h"
#include <filesystem>
#include "vk_images.h"
#include "vk_descriptors.h"

class VulkanMaterial {
public:
	void init_materials(const Context& context, const AllocatedImage& black_image);

	void upload_material(std::filesystem::path filePath);

	void destroy_materials();
	std::vector<Material> _materials;

	bool materials_empty();

	void write_materials(int mat_idx, DescriptorWriter& writer, VkSampler sampler);

	const uint32_t num_tex = 6;

private:
	Context _context;
	AllocatedImage _black_image;
};
