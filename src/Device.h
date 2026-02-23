#pragma once

#include "ValueCache.h"

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

	private:
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_device;

		mutable ValueCache<VkSurfaceCapabilitiesKHR> m_surfaceCapabilities;
		mutable ValueCache<VkPhysicalDeviceProperties2> m_properties;
		mutable ValueCache<VkPhysicalDeviceFeatures2> m_features;
		mutable ValueCache<VkPhysicalDeviceMemoryProperties> m_memProperties;
		mutable ValueCache<std::vector<VkQueueFamilyProperties>> m_queueFamilies;

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

		bool initialize(const InitInfo& info);
		void destroy();
	};
}
