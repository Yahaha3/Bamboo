#include "base_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/core/base/macro.h"
#include <array>

namespace Bamboo
{

	void BasePass::init()
	{
		createRenderPass();

	}

	void BasePass::prepare()
	{
		
	}

	void BasePass::record()
	{
		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.framebuffer = m_framebuffer;
		render_pass_bi.renderArea.offset = { 0, 0 };
		render_pass_bi.renderArea.extent = { m_width, m_height };

		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color = { {0.0f, 0.3f, 0.0f, 1.0f} };
		clear_values[1].depthStencil = { 1.0f, 0 };
		render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_bi.pClearValues = clear_values.data();

		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
		vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

		// 1.bind pipeline
		// TODO

		// 2.set viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_width);
		viewport.height = static_cast<float>(m_height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		// 3.set scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_width, m_height };
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		vkCmdEndRenderPass(command_buffer);
	}

	void BasePass::destroy()
	{
		RenderPass::destroy();
	}

	void BasePass::createRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments{};

		// color attachment
		attachments[0].format = VulkanRHI::get().getColorFormat();
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// depth attachment
		attachments[1].format = VulkanRHI::get().getDepthFormat();
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_reference{};
		color_reference.attachment = 0;
		color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_reference{};
		depth_reference.attachment = 1;
		depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// subpass
		VkSubpassDescription subpass_desc{};
		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments = &color_reference;
		subpass_desc.pDepthStencilAttachment = &depth_reference;
		subpass_desc.inputAttachmentCount = 0;
		subpass_desc.pInputAttachments = nullptr;
		subpass_desc.preserveAttachmentCount = 0;
		subpass_desc.pPreserveAttachments = nullptr;
		subpass_desc.pResolveAttachments = nullptr;

		// subpass dependencies
		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_ci.pAttachments = attachments.data();
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_ci.pDependencies = dependencies.data();

		vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
	}

	void BasePass::createPipeline()
	{
		VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci{};
		input_assembly_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_state_ci.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterize_state_ci{};
		rasterize_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterize_state_ci.depthClampEnable = VK_FALSE;
		rasterize_state_ci.rasterizerDiscardEnable = VK_FALSE;
		rasterize_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
		rasterize_state_ci.lineWidth = 1.0f;
		rasterize_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterize_state_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterize_state_ci.depthBiasEnable = VK_FALSE;
		rasterize_state_ci.depthBiasConstantFactor = 0.0f;
		rasterize_state_ci.depthBiasClamp = 0.0f;
		rasterize_state_ci.depthBiasSlopeFactor = 0.0f;
	}

	void BasePass::createFramebuffer()
	{
		// 1.create depth stencil image and view
		createImageAndView(m_width, m_height, 1, VK_SAMPLE_COUNT_1_BIT, VulkanRHI::get().getDepthFormat(),
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_IMAGE_ASPECT_DEPTH_BIT, m_depth_stencil_image_view);

		// 2.create color images and view
		createImageAndView(m_width, m_height, 1, VK_SAMPLE_COUNT_1_BIT, VulkanRHI::get().getColorFormat(),
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_COLOR_BIT, m_color_image_view);

		// 3.create framebuffer
		std::array<VkImageView, 2> attachments{};
		attachments[0] = m_color_image_view.view;
		attachments[1] = m_depth_stencil_image_view.view;

		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_ci.pAttachments = attachments.data();
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
	}

	void BasePass::createResizableObjects(uint32_t width, uint32_t height)
	{
		RenderPass::createResizableObjects(width, height);

		createFramebuffer();
	}

	void BasePass::destroyResizableObjects()
	{
		// 1.destroy depth stencil image and view
		m_depth_stencil_image_view.destroy();

		// 2.destroy color image and view
		m_color_image_view.destroy();

		// 3.destroy framebuffers
		vkDestroyFramebuffer(VulkanRHI::get().getDevice(), m_framebuffer, nullptr);
	}

	VkImageView BasePass::getColorImageView()
	{
		return m_color_image_view.view;
	}

}