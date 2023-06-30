#pragma once

#include "runtime/core/base/macro.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#define MAX_FRAMES_IN_FLIGHT 2

namespace Bamboo
{
	// get VkResult error code string
	const char* vkErrorString(VkResult result);

	// get physical device type string from VkPhysicalDeviceType
	std::string vkPhysicalDeviceTypeString(VkPhysicalDeviceType type);

	// assert VkResult and log error code
#define CHECK_VULKAN_RESULT(result, msg) \
    if (result != 0) \
    { \
        LOG_FATAL("failed to {}, error: {}", msg, vkErrorString(result)); \
    }

	// VMA Buffer
	struct VmaBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation;

		void destroy();
	};

	// VMA Image
	struct VmaImage
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation;

		void destroy();
	};

	// VMA Image and Image view
	struct VmaImageView
	{
		VmaImage vma_image;
		VkImageView view = VK_NULL_HANDLE;

		void destroy();
		VkImage image() { return vma_image.image; }
	};

	// VMA Image + Image view + Sampler
	struct VmaImageViewSampler
	{
		VmaImage vma_image;
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		void destroy();
		VkImage image() { return vma_image.image; }
	};

	class VulkanUtil
	{
	public:
		// begin and end instant transient commandbuffer
		static VkCommandBuffer beginInstantCommands();
		static void endInstantCommands(VkCommandBuffer command_buffer);

		static void createBuffer(VkDeviceSize size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaBuffer& buffer);
		static void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

		static void createImageViewSampler(uint32_t width, uint32_t height, uint8_t* image_data,
			uint32_t mip_levels, bool is_srgb, VkFilter min_filter, VkFilter mag_filter,
			VkSamplerAddressMode address_mode, VmaImageViewSampler& vma_image_view_sampler);

		static void createImageAndView(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VkImageAspectFlags aspect_flags, VmaImageView& vma_image_view);
		static void createImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VmaImage& vma_image);
		static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
		static VkSampler createSampler(VkFilter min_filter, VkFilter mag_filter, uint32_t mip_levels,
			VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w);

		static void createVertexBuffer(uint32_t buffer_size, void* vertex_data, VmaBuffer& vertex_buffer);
		static void createIndexBuffer(const std::vector<uint32_t>& indices, VmaBuffer& index_buffer);

		static void transitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkFormat format = VK_FORMAT_B8G8R8A8_SRGB, uint32_t mip_levels = 1);
		static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		static void createImageMipmaps(VkImage image, VkFormat image_format, uint32_t width, uint32_t height, uint32_t mip_levels);

		static bool hasStencil(VkFormat format);
	};
}