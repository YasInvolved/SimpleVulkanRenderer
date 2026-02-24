#pragma once

#define BITFIELD_TRUE(x, y) (x & y) == y

#include "ValueCache.h"
#include "CommandPool.h"

namespace svr
{
	class Device
	{
	public:
		struct InitInfo
		{
			uint32_t queueCreateInfosCount;
			const VkDeviceQueueCreateInfo* queueCreateInfos;
			uint32_t extensionsCount;
			const char* const* extensions;
			bool dynamicRenderingEnabled;
			const VkPhysicalDeviceDynamicRenderingFeatures* dynamicRenderingFeatures;
		};

		struct SwapchainCreateInfo
		{
			VkSurfaceKHR surface;
			uint32_t minImageCount;
			VkFormat imageFormat;
			VkColorSpaceKHR colorSpace;
			VkExtent2D imageExtent;
			uint32_t queueFamilyIndicesCount;
			const uint32_t* queueFamilyIndices;
			VkSurfaceTransformFlagBitsKHR transform;
			VkPresentModeKHR presentMode;
			VkSwapchainKHR oldSwapchain;
		};

		struct ImageViewCreateInfo
		{
			VkImageViewType viewType;
			VkFormat format;
			VkImageSubresourceRange subresourceRange;
		};

	private:
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_device;

		mutable ValueCache<VkSurfaceCapabilitiesKHR> m_surfaceCapabilities;
		mutable ValueCache<VkPhysicalDeviceProperties2> m_properties;
		mutable ValueCache<VkPhysicalDeviceFeatures2> m_features;
		mutable ValueCache<VkPhysicalDeviceMemoryProperties> m_memProperties;
		mutable ValueCache<std::vector<VkQueueFamilyProperties>> m_queueFamilies;

		mutable std::vector<VkDeviceMemory> m_allocatedMemory;

		bool m_initialized;

	public:
		Device(VkPhysicalDevice dev);

		VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		VkDevice getDevice() const
		{
			if (!m_initialized)
				throw std::runtime_error("Failed to get the device: not initialized");

			return m_device;
		}

		const std::vector<VkExtensionProperties> getExtensionProperties() const;

		const VkSurfaceCapabilitiesKHR& getSurfaceCapabilities(VkSurfaceKHR surface) const;
		const VkPhysicalDeviceProperties2& getProperties() const;
		const VkPhysicalDeviceFeatures2& getFeatures() const;
		const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
		const std::vector<VkQueueFamilyProperties>& getQueueFamilyProperties() const;
		const std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkSurfaceKHR surface) const;
		const std::vector<VkPresentModeKHR> getPresentModes(VkSurfaceKHR surface) const;

		VkSwapchainKHR createSwapchain(const SwapchainCreateInfo& createInfo) const;
		VkSemaphore createSemaphore() const;
		VkFence createFence(bool signaled = false) const;

		VkImage createImage(const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memoryProperties) const;
		VkImageView createImageView(VkImage image, const ImageViewCreateInfo& createInfo) const;

		CommandPool createCommandPool(uint32_t queueFamilyindex) const;

		bool initialize(const InitInfo& info);
		void destroy();

	private:
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		inline void failIfNotInitialized() const
		{
			if (!m_initialized)
				throw std::runtime_error("The device should be initialized to perform this action");
		}
	};
}
