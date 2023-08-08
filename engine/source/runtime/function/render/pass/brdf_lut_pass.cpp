#include "brdf_lut_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/core/base/macro.h"
#include "runtime/resource/shader/shader_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/platform/timer/timer.h"
#include "runtime/resource/asset/texture_2d.h"

#include <array>
#include <fstream>

namespace Bamboo
{

	BRDFLUTPass::BRDFLUTPass()
	{
		m_format = VK_FORMAT_R16G16_SFLOAT;
	}

	void BRDFLUTPass::render()
	{
		StopWatch stop_watch;
		stop_watch.start();

		// render to framebuffer
		VkClearValue clear_values[1];
		clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.renderArea.extent.width = m_width;
		render_pass_bi.renderArea.extent.height = m_height;
		render_pass_bi.clearValueCount = 1;
		render_pass_bi.pClearValues = clear_values;
		render_pass_bi.framebuffer = m_framebuffer;

		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
		vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.width = static_cast<float>(m_width);
		viewport.height = static_cast<float>(m_height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = m_width;
		scissor.extent.height = m_height;

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);
		vkCmdDraw(command_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(command_buffer);

		VulkanUtil::endInstantCommands(command_buffer);

		// save to texture2d
		// create staging image
		VulkanUtil::createImage(m_width, m_height, 1, VK_SAMPLE_COUNT_1_BIT, m_format, VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, m_staging_image);

		// copy to staging image
		VkImageCopy image_copy{};
		image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_copy.srcSubresource.layerCount = 1;
		image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_copy.dstSubresource.layerCount = 1;
		image_copy.extent.width = m_width;
		image_copy.extent.height = m_height;
		image_copy.extent.depth = 1;

		// transition image layouts
		VulkanUtil::transitionImageLayout(m_color_image_view.image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		VulkanUtil::transitionImageLayout(m_staging_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		command_buffer = VulkanUtil::beginInstantCommands();

		// copy image
		vkCmdCopyImage(
			command_buffer,
			m_color_image_view.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_staging_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&image_copy);

		VulkanUtil::endInstantCommands(command_buffer);

		// reset image layouts
		VulkanUtil::transitionImageLayout(m_color_image_view.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		LOG_INFO("generate brdf lut time: {}ms", stop_watch.stop());

		// get staging image layout
		VkImageSubresource sub_res{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout sub_res_layout;
		vkGetImageSubresourceLayout(VulkanRHI::get().getDevice(), m_staging_image.image, &sub_res, &sub_res_layout);
		size_t size = sub_res_layout.size;

		// mapping data
		void* mapped_data = nullptr;
		vmaMapMemory(VulkanRHI::get().getAllocator(), m_staging_image.allocation, &mapped_data);

		// write to file
// 		std::ofstream ofs("D:/Test/brdf_lut.bin", std::ios::binary);
// 		ofs.write((const char*)mapped_data, size);
// 		ofs.close();

		// write to texture2d
		std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
		texture->setURL(BRDF_TEX_URL);

		texture->m_width = m_width;
		texture->m_height = m_height;
		texture->m_address_mode_u = texture->m_address_mode_v = texture->m_address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		texture->m_texture_type = ETextureType::IBL;
		texture->m_pixel_type = EPixelType::RG16;

		size_t image_size = m_width * m_height * 4;
		texture->m_image_data.resize(image_size);
		memcpy(texture->m_image_data.data(), mapped_data, image_size);

		texture->inflate();
		g_runtime_context.assetManager()->serializeAsset(texture);

		vmaUnmapMemory(VulkanRHI::get().getAllocator(), m_staging_image.allocation);
	}

	void BRDFLUTPass::createRenderPass()
	{
		// color attachment
		VkAttachmentDescription color_attachment{};
		color_attachment.format = m_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		// subpass
		VkSubpassDescription subpass_desc{};
		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments = &color_reference;

		// subpass dependencies
		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 1;
		render_pass_ci.pAttachments = &color_attachment;
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = 2;
		render_pass_ci.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create render pass");
	}

	void BRDFLUTPass::createDescriptorSetLayouts()
	{
		VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
		desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		m_desc_set_layouts.resize(1);
		VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create descriptor set layout");
	}

	void BRDFLUTPass::createPipelineLayouts()
	{
		VkPipelineLayoutCreateInfo pipeline_layout_ci{};
		pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_ci.setLayoutCount = 1;
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];

		m_pipeline_layouts.resize(1);
		VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create pipeline layout");
	}

	void BRDFLUTPass::createPipelines()
	{
		// disable culling and depth testing
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_depth_stencil_ci.depthTestEnable = VK_FALSE;
		m_depth_stencil_ci.depthWriteEnable = VK_FALSE;

		// vertex input state
		VkPipelineVertexInputStateCreateInfo vertex_input_ci{};
		vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// shader stages
		const auto& shader_manager = g_runtime_context.shaderManager();
		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
			shader_manager->getShaderStageCI("brdf_lut.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("brdf_lut.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		
		// create graphics pipeline
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[0];
		m_pipeline_ci.renderPass = m_render_pass;
		m_pipeline_ci.subpass = 0;

		m_pipelines.resize(1);
		VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
		CHECK_VULKAN_RESULT(result, "create brdf lut graphics pipeline");
	}

	void BRDFLUTPass::createFramebuffer()
	{
		// 1.create color images and view
		VulkanUtil::createImageAndView(m_width, m_height, 1, VK_SAMPLE_COUNT_1_BIT, m_format,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_COLOR_BIT, m_color_image_view);

		// 2.create framebuffer
		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 1;
		framebuffer_ci.pAttachments = &m_color_image_view.view;
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
		CHECK_VULKAN_RESULT(result, "create brdf lut graphics pipeline");
	}

	void BRDFLUTPass::destroyResizableObjects()
	{
		m_color_image_view.destroy();
		m_staging_image.destroy();

		RenderPass::destroyResizableObjects();
	}

}