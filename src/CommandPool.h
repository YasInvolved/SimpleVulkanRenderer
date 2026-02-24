#pragma once

namespace svr
{
	class CommandPool
	{
	private:
		VkDevice m_device;
		VkCommandPool m_commandPool;
		mutable std::vector<VkCommandBuffer> m_commandBuffers;

	public:
		CommandPool() : m_device(VK_NULL_HANDLE), m_commandPool(VK_NULL_HANDLE) {}
		CommandPool(VkDevice device, VkCommandPool cmdPool)
			: m_device(device), m_commandPool(cmdPool)
		{ }

		std::span<VkCommandBuffer> allocCommandBuffers(VkCommandBufferLevel level, uint32_t count) const;
		void freeCommandBuffers(const std::span<VkCommandBuffer> buffers) const;

		void destroy();
	};
}
