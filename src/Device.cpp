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

VkImage Device::createImage(const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memoryProperties) const
{
	failIfNotInitialized();

	VkImage image;
	if (vkCreateImage(m_device, &createInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Failed to create an image");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, image, &memRequirements);

	const VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits,
			memoryProperties
		)
	};

	VkDeviceMemory memory;

	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate image's memory");

	if (vkBindImageMemory(m_device, image, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("Failed to bind memory to the image");

	m_allocations.emplace_back(std::move(memory));
	m_buffersAndImagesMemory[image] = m_allocations.size() - 1;

	return image;
}

VkImageView Device::createImageView(VkImage image, const ImageViewCreateInfo& createInfo) const
{
	failIfNotInitialized();

	const VkImageViewCreateInfo vkCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = createInfo.viewType,
		.format = createInfo.format,
		.subresourceRange = createInfo.subresourceRange
	};

	VkImageView view;
	if (vkCreateImageView(m_device, &vkCreateInfo, nullptr, &view) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view");

	return view;
}

VkBuffer Device::createBuffer(const BufferCreateInfo& createInfo) const
{
	const VkBufferCreateInfo vkCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = createInfo.size,
		.usage = createInfo.usage,
		.sharingMode = createInfo.sharing,
		.queueFamilyIndexCount = createInfo.queueFamilyIxCount,
		.pQueueFamilyIndices = createInfo.queueFamilyIxs
	};

	VkBuffer buffer;
	if (vkCreateBuffer(m_device, &vkCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

	const VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits,
			createInfo.memoryProperties
		)
	};

	VkDeviceMemory memory;
	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate buffer memory");

	vkBindBufferMemory(m_device, buffer, memory, 0);
	m_allocations.emplace_back(memory);
	m_buffersAndImagesMemory[buffer] = m_allocations.size() - 1;

	return buffer;
}

std::vector<VkBuffer> Device::createBuffers(const BufferCreateInfo& createInfo, size_t bufCount) const
{
	std::vector<VkBuffer> buffers(bufCount);
	m_allocations.reserve(m_allocations.size() + bufCount);
	m_buffersAndImagesMemory.reserve(m_buffersAndImagesMemory.size() + bufCount);

	const VkBufferCreateInfo vkCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = createInfo.size,
		.usage = createInfo.usage,
		.sharingMode = createInfo.sharing,
		.queueFamilyIndexCount = createInfo.queueFamilyIxCount,
		.pQueueFamilyIndices = createInfo.queueFamilyIxs
	};

	for (size_t i = 0; i < bufCount; i++)
		if (vkCreateBuffer(m_device, &vkCreateInfo, nullptr, &buffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create buffers");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, buffers[0], &memRequirements);

	const VkMemoryAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size * bufCount,
		.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits,
			createInfo.memoryProperties
		)
	};

	VkDeviceMemory allocation;
	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &allocation) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate memory for buffers");

	m_allocations.emplace_back(std::move(allocation));

	VkDeviceSize offset = 0;
	for (const auto& buffer : buffers)
	{
		vkBindBufferMemory(m_device, buffer, allocation, offset);
		m_buffersAndImagesMemory[buffer] = m_allocations.size() - 1;
		offset += createInfo.size;
	}

	return buffers;
}

Device::MappedMemory Device::mapBufferMemory(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) const
{
	MappedMemory retval{};
	const size_t memoryArrayIndex = m_buffersAndImagesMemory[buffer];
	retval.memory = m_allocations[memoryArrayIndex];
	vkMapMemory(m_device, retval.memory, offset, size, 0, &retval.ptr);

	return retval;
}

void Device::unmapBufferMemory(MappedMemory& memory) const
{
	vkUnmapMemory(m_device, memory.memory);
	memory.ptr = nullptr;
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

VkShaderModule Device::createShaderModule(size_t codeSize, const uint32_t* pCode) const
{
	failIfNotInitialized();

	const VkShaderModuleCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = codeSize,
		.pCode = pCode,
	};

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a shader module");

	return shaderModule;
}

VkDescriptorSetLayout Device::createDescriptorSetLayout(size_t bindingCount, const VkDescriptorSetLayoutBinding* pBindings) const
{
	failIfNotInitialized();

	VkDescriptorSetLayout layout;

	const VkDescriptorSetLayoutCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindingCount),
		.pBindings = pBindings
	};

	if (vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &layout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a Descriptor Set Layout");

	return layout;
}

void Device::destroy()
{
	if (!m_initialized)
		return;

	for (const auto& [_, allocationIx] : m_buffersAndImagesMemory)
	{
		auto& allocation = m_allocations[allocationIx];
		if (allocation != nullptr)
		{
			vkFreeMemory(m_device, allocation, nullptr);
			allocation = VK_NULL_HANDLE;
		}
	}
		
	vkDestroyDevice(m_device, nullptr);
	m_initialized = false;
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	auto& memoryProperties = getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		auto memoryType = memoryProperties.memoryTypes[i];

		if (typeFilter & (1 << i) &&
			BITFIELD_TRUE(memoryType.propertyFlags, properties))
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find a suitable memory type");
}
