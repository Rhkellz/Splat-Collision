#pragma once

#include "vk_types.h"
#include "vk_images.h"
#include "vk_buffers.h"
#include "vk_initializers.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"
#include "vk_loader.h"
#include "vk_swapchain.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "ply_loader.hpp"

constexpr unsigned int FRAME_OVERLAP = 2;

struct SplatDepth { uint32_t index; float z; };
struct RadixSortPC {
	uint32_t num_elements;
	uint32_t shift;              // pass * 8
	uint32_t num_workgroups;
	uint32_t num_blocks_per_workgroup;
};

struct RadixDepthsPC {
	glm::mat4 view;
	glm::mat4 proj;
	float near_plane;
};
struct FrameData {
	VkSemaphore _swapchain_semaphore;
	VkFence _render_fence;

	VkCommandPool _command_pool;
	VkCommandBuffer _main_command_buffer;

	DeletionQueue _deletion_queue;
	DescriptorAllocatorGrowable _frame_descriptors;

	AllocatedBuffer _GPU_scene_data_buffer;
};


class VulkanEngine {
public:

	bool _is_initialized{ false };
	int _frame_number{ 0 };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	Context _context;

	VulkanSwapchain _vk_swapchain;

	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };

	VkQueue _graphics_queue;
	uint32_t _graphics_queue_family;

	DeletionQueue _main_deletion_queue;

	AllocatedImage _draw_image;
	AllocatedImage _depth_image;
	VkExtent2D _draw_extent;

	float screen_width = 0.0;
	float screen_height = 0.0;

	bool stop_rendering{ false };

	VkFence _imm_fence;
	VkCommandBuffer _imm_command_buffer;
	VkCommandPool _imm_command_pool;

	DescriptorAllocator global_descriptor_allocator;

	VkPipeline _splat_pipeline;
	VkPipeline _rdx_histogram_pipeline;
	VkPipeline _rdx_scan_scat_pipeline;
	VkPipeline _rdx_compute_depths_pipeline;

	VkPipelineLayout _compute_rdx_pipeline_layout;
	VkPipelineLayout _compute_depths_pipeline_layout;
	VkPipelineLayout _splat_pipeline_layout;

	VkDescriptorSetLayout _splat_data_descriptor_layout;
	VkDescriptorSetLayout _gpu_scene_data_descriptor_layout;
	VkDescriptorSetLayout _splat_indicies_descriptor_layout;
	VkDescriptorSetLayout _compute_rdx_descriptor_layout;
	VkDescriptorSetLayout _compute_rdx_depths_descriptor_layout;

	bool resize_requested;
	GPUSceneData scene_data;

	VkClearColorValue clear_color;

	float rad = 0.23f;
	float phi = 1.45f;
	float theta = 0.0f;
	glm::vec3 center = { 0.0, 0.04, 0.0 };

	bool first_mouse = true;
	int last_mouse_x = 0;
	int last_mouse_y = 0;
	bool lmb_held = false;

	std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();;
	std::chrono::steady_clock::time_point curr_time;
	int frame_time = 0;

	float min_opacity = 0.001;
	float scroll_sensitivity = 0.02;
	float clipping_plane = 0.05;
	int num_workgroups = 64;

	VkDescriptorSet splat_set;

	Scene scene;

	AllocatedBuffer splat_buffer;
	AllocatedBuffer splat_centroids;

	AllocatedBuffer rdx_buffers[2];
	AllocatedBuffer rdx_count_buffer;
	AllocatedBuffer visible_ele_buffer;

	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cam_pos_cartesian;
	std::vector<SplatDepth> depths;
	int final_rdx_buffer_idx = 0;


private:

	void init_vulkan();
	void init_images();
	void init_commands();
	void init_sync_structures();

	void init_descriptors();

	void init_pipelines();

	void init_imgui();

	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	void start_rendering(VkCommandBuffer cmd);

	void end_rendering(VkCommandBuffer cmd);
	
	void sort_splats(VkCommandBuffer cmd);

	void draw_geometry(VkCommandBuffer cmd);

	void init_splat_pipeline();

	void init_compute_pipelines();

	void init_default_data();

	void init_splats();

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	void resize_draw_images();

	void dispatch_rdx_histogram(VkDescriptorSet radix_descriptor, VkCommandBuffer imm_cmd, RadixSortPC pc);

	void dispatch_rdx_scan_scat(VkDescriptorSet radix_descriptor, VkCommandBuffer imm_cmd, RadixSortPC pc);

	void dispatch_rdx_depths(VkDescriptorSet radix_descriptor, VkCommandBuffer imm_cmd, RadixDepthsPC pc, int dispatch_size);
};
