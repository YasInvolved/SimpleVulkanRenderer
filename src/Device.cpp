#include "Device.h"

using namespace svr;

Device::Device(VkPhysicalDevice dev)
	: m_physicalDevice(dev), m_device(VK_NULL_HANDLE), m_initialized(false)
{
	assert(m_physicalDevice != VK_NULL_HANDLE);
}

const std::vector<VkExtensionProperties> Device::getExtensionProperties() const
{
	uint32_t count;
	std::vector<VkExtensionProperties> extensions;
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, nullptr);
	extensions.resize(count);
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, extensions.data());

	return extensions;
}

const VkSurfaceCapabilitiesKHR& Device::getSurfaceCapabilities(VkSurfaceKHR surface) const
{
	if (!m_surfaceCapabilities.isEmpty())
		return m_surfaceCapabilities.getValue();

	auto& value = m_surfaceCapabilities.getValue();

	VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &value);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to get surface capabilities from the device");

	m_surfaceCapabilities.forceUnsetEmpty();
	return value;
}

const VkPhysicalDeviceProperties2& Device::getProperties() const
{
	if (!m_properties.isEmpty())
		return m_properties.getValue();

	auto& value = m_properties.getValue();
	value.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	vkGetPhysicalDeviceProperties2(m_physicalDevice, &value);
	m_properties.forceUnsetEmpty();
	return value;
}

const VkPhysicalDeviceFeatures2& Device::getFeatures() const
{
	if (!m_features.isEmpty())
		return m_features.getValue();

	auto& value = m_features.getValue();
	value.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_features.getValue());
	m_features.forceUnsetEmpty();
	return m_features.getValue();
}

const VkPhysicalDeviceMemoryProperties& Device::getMemoryProperties() const
{
	if (!m_memProperties.isEmpty())
		return m_memProperties.getValue();

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memProperties.getValue());
	m_memProperties.forceUnsetEmpty();
	return m_memProperties.getValue();
}

const std::vector<VkQueueFamilyProperties>& Device::getQueueFamilyProperties() const
{
	if (!m_queueFamilies.isEmpty())
		return m_queueFamilies.getValue();

	auto& value = m_queueFamilies.getValue();

	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
	value.resize(count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, value.data());
	m_queueFamilies.forceUnsetEmpty();

	return value;
}

const std::vector<VkSurfaceFormatKHR> Device::getSurfaceFormats(VkSurfaceKHR surface) const
{
	uint32_t count;
	std::vector<VkSurfaceFormatKHR> formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, nullptr);
	formats.resize(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, formats.data());

	return formats;
}

const std::vector<VkPresentModeKHR> Device::getPresentModes(VkSurfaceKHR surface) const
{
	uint32_t count;
	std::vector<VkPresentModeKHR> modes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, nullptr);
	modes.resize(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, modes.data());

	return modes;
}

VkSwapchainKHR Device::createSwapchain(const SwapchainCreateInfo& createInfo) const
{
	failIfNotInitialized();

	VkSwapchainKHR swapchain;
	const VkSwapchainCreateInfoKHR info =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = createInfo.surface,
		.minImageCount = createInfo.minImageCount,
		.imageFormat = createInfo.imageFormat,
		.imageColorSpace = createInfo.colorSpace,
		.imageExtent = createInfo.imageExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = createInfo.queueFamilyIndicesCount,
		.pQueueFamilyIndices = createInfo.queueFamilyIndices,
		.preTransform = createInfo.transform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = createInfo.presentMode,
		.clipped = VK_FALSE,
		.oldSwapchain = createInfo.oldSwapchain
	};

	if (vkCreateSwapchainKHR(m_device, &info, nullptr, &swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a swapchain");

	return swapchain;
}

VkSemaphore Device::createSemaphore() const
{
	failIfNotInitialized();

	constexpr VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore;
	if (vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a semaphore");

	return semaphore;
}

VkFence Device::createFence(bool signaled) const
{
	failIfNotInitialized();

	VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
	if (signaled)
		createInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence fence;
	if (vkCreateFence(m_device, &createInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a fence");

	return fence;
}

CommandPool Device::createCommandPool(uint32_t queueFamilyIndex) const
{
	failIfNotInitialized();

	const VkCommandPoolCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndex
	};

	VkCommandPool pool;
	if (vkCreateCommandPool(m_device, &createInfo, nullptr, &pool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a command pool");

	return CommandPool(m_device, pool);
}

bool Device::initialize(const InitInfo& info)
{
	const VkDeviceCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = info.dynamicRenderingFeatures,
		.flags = 0,
		.queueCreateInfoCount = info.queueCreateInfosCount,
		.pQueueCreateInfos = info.queueCreateInfos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = info.extensionsCount,
		.ppEnabledExtensionNames = info.extensions,
		.pEnabledFeatures = &m_features.getValue().features, // TODO: handle ver. 2
	};

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		return false;

	m_initialized = true;
	return true;
}

void Device::destroy()
{
	if (!m_initialized)
		return;

	vkDestroyDevice(m_device, nullptr);
	m_initialized = false;
}
