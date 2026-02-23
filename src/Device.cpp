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
