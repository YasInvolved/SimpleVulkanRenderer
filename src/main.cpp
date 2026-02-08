#include <shaders.h>

#define BITFIELD_TRUE(x, y) (x & y) == y

static constexpr auto SVR_DEBUG_UTILS_MESSAGE_TYPES =
VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

static constexpr auto SVR_DEBUG_UTILS_MESSAGE_SEVERITY =
VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;

	static constexpr VkVertexInputBindingDescription getBindingDescription()
	{
		constexpr VkVertexInputBindingDescription desc =
		{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		return desc;
	}

	using AttrDesc_t = std::array<VkVertexInputAttributeDescription, 2>;
	static constexpr AttrDesc_t getAttributeDesc()
	{
		constexpr AttrDesc_t descs =
		{
			{
				{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, pos),
				},
				{
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, color),
				}
			}
		};

		return descs;
	}
};

struct PushConstants
{
	glm::mat4x4 mvp;
};

class Application
{
private:
	static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3ull;
	const std::string_view m_modelPath;
	bool m_isRunning = true;

	SDL_Window* m_pWindow = nullptr;

	VkInstance m_pInstance = VK_NULL_HANDLE;

	// debug
	VkDebugUtilsMessengerEXT m_debugMsger = VK_NULL_HANDLE;

	// device
	uint32_t m_gfxQueueIx = 0;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties m_memoryProperties = {};

	VkQueue m_gfxQueue;

	// surface & swapchain
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	uint32_t m_swapchainImgWidth = 0, m_swapchainImgHeight = 0;
	VkSurfaceFormatKHR m_swapchainImgFormat = {};
	VkFormat m_depthStencilFormat = VK_FORMAT_D32_SFLOAT;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkImage m_depthImage = VK_NULL_HANDLE;
	VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;

	// command stuff
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	// memory
	VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_stagingBufferMemory = VK_NULL_HANDLE;
	void* m_pStagingBuffer = VK_NULL_HANDLE;

	VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

	size_t m_indices = 0;
	VkBuffer m_indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

	glm::mat4 m_mvp;

	// sync
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	uint32_t m_currentFrame = 0;

public:
	Application(const std::string_view modelPath) 
		: m_modelPath(modelPath), 
		m_commandBuffers(MAX_FRAMES_IN_FLIGHT),
		m_imageAvailableSemaphores(MAX_FRAMES_IN_FLIGHT),
		m_renderFinishedSemaphores(MAX_FRAMES_IN_FLIGHT),
		m_inFlightFences(MAX_FRAMES_IN_FLIGHT)
	{}

	~Application()
	{
		cleanup();
	}

	void run()
	{
		initialize();

		while (m_isRunning)
		{
			update();
			render();
		}
	}

private:
	void initialize()
	{
		initWindow();
		initVkInstance();
		selectPhysicalDevice();
		initDevice();
		initSwapchain();
		initDeviceMemory();
		initCmdPoolAndBuffers();
		initRenderpass();
		initPipeline();
		initFramebuffers();
		loadObj(m_modelPath);
		createSyncObjects();
	}

	void initWindow()
	{
		SDL_Init(SDL_INIT_VIDEO);

		SDL_DisplayID primaryId = SDL_GetPrimaryDisplay();
		const char* pdName = SDL_GetDisplayName(primaryId);
		const auto* pdMode = SDL_GetCurrentDisplayMode(primaryId);

		fmt::print("Primary display detected:\n\tName: {}\n\tResolution: {}x{}\n\tRefresh Rate: {}hz\n\n",
			pdName, pdMode->w, pdMode->h, pdMode->refresh_rate
		);

		int windowResW = pdMode->w / 2;
		int windowResH = pdMode->h / 2;

		fmt::print("Creating a {}x{} window on primary display\n", windowResW, windowResH);
		m_pWindow = SDL_CreateWindow("SimpleVulkanRenderer", windowResW, windowResH, SDL_WINDOW_VULKAN /* | SDL_WINDOW_RESIZABLE*/);
		if (m_pWindow == nullptr)
			throw std::runtime_error("Failed to create window");
	}

	void initVkInstance()
	{
		volkInitialize();

		static constexpr VkApplicationInfo appInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "SimpleVulkanRenderer",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = nullptr,
			.engineVersion = 0,
			.apiVersion = VK_API_VERSION_1_3
		};

		static const VkDebugUtilsMessengerCreateInfoEXT dbgMsgCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0,
			.messageSeverity = SVR_DEBUG_UTILS_MESSAGE_SEVERITY,
			.messageType = SVR_DEBUG_UTILS_MESSAGE_TYPES,
			.pfnUserCallback = DebugMessengerCallback,
			.pUserData = this,
		};

		uint32_t sdlRequiredExtCount;
		const char* const* sdlRequirements = SDL_Vulkan_GetInstanceExtensions(&sdlRequiredExtCount);
		std::vector<const char*> requiredExtensions(sdlRequirements, sdlRequirements + sdlRequiredExtCount);

#ifdef SVR_DEBUG
		requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		uint32_t propCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &propCount, nullptr);
		std::vector<VkExtensionProperties> instanceExtensions(propCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &propCount, instanceExtensions.data());

		fmt::print("Found {} instance extension properties\n", propCount);
		for (const auto& requiredExtension : requiredExtensions)
		{
			bool found = false;
			for (const auto& prop : instanceExtensions)
			{
				if (strcmp(requiredExtension, prop.extensionName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				throw std::runtime_error("Failed to create Vulkan Instance. Required extension is missing!");
		}

		const std::vector<const char*> enabledLayers =
		{
			#ifdef SVR_DEBUG
			"VK_LAYER_KHRONOS_validation",
			#endif
		};

		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const auto& enabledLayer : enabledLayers)
		{
			bool found = false;
			for (const auto& layer : availableLayers)
			{
				if (strcmp(layer.layerName, enabledLayer) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				throw std::runtime_error("Failed to create Vulkan Instance. Required layer is missing!");
		}

		const VkInstanceCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = &dbgMsgCreateInfo,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
			.ppEnabledLayerNames = enabledLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data()
		};

		if (vkCreateInstance(&createInfo, nullptr, &m_pInstance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Vulkan Instance!");

		volkLoadInstance(m_pInstance);

#ifdef SVR_DEBUG
		if (vkCreateDebugUtilsMessengerEXT(m_pInstance, &dbgMsgCreateInfo, nullptr, &m_debugMsger) != VK_SUCCESS)
			fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
				"WARN: Failed to create a Vulkan Debug Utils Messenger"
			);
#endif
	}

	VkPhysicalDeviceProperties getPhysicalDeviceProperties(VkPhysicalDevice physDev) const noexcept
	{
		VkPhysicalDeviceProperties devProps;
		vkGetPhysicalDeviceProperties(physDev, &devProps);
		return devProps;
	}

	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice physDev) const noexcept
	{
		VkPhysicalDeviceFeatures devFts;
		vkGetPhysicalDeviceFeatures(physDev, &devFts);
		return devFts;
	}

	std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilies(VkPhysicalDevice physDev) const
	{
		uint32_t qfCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, nullptr);

		if (qfCount == 0)
			throw std::runtime_error("Vulkan API returned 0 queue families");

		std::vector<VkQueueFamilyProperties> qfProps(qfCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfProps.data());

		return qfProps;
	}

	VkFormat findSupportedFormat(VkPhysicalDevice physDev, const std::span<const VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
	{
		assert(physDev != VK_NULL_HANDLE);

		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physDev, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && BITFIELD_TRUE(props.linearTilingFeatures, features))
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && BITFIELD_TRUE(props.optimalTilingFeatures, features))
				return format;
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	VkFormat selectDepthFormat(VkPhysicalDevice physDev) const
	{
		static constexpr std::array<VkFormat, 3> FORMATS =
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		return findSupportedFormat(physDev, FORMATS, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	void selectPhysicalDevice()
	{
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, nullptr);
		if (deviceCount == 0)
			throw std::runtime_error("No GPUs found");

		std::vector<VkPhysicalDevice> m_devices(deviceCount);
		vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, m_devices.data());

		// look for discrete GPU, if not found, select the first one
		for (const auto& physicalDevice : m_devices)
		{
			auto devProps = getPhysicalDeviceProperties(physicalDevice);
			auto qfProps = getPhysicalDeviceQueueFamilies(physicalDevice);

			// we aren't selecting the index yet
			bool hasGraphicsFamily = false;
			for (const auto& props : qfProps)
			{
				if (props.queueCount > 0 && BITFIELD_TRUE(props.queueFlags, (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)))
				{
					hasGraphicsFamily = true;
					break;
				}
			}

			if (devProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && hasGraphicsFamily)
				m_physicalDevice = physicalDevice;
		}

		if (m_physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("No suitable device for graphics rendering found");
	}

	void initDevice()
	{
		assert(m_physicalDevice != VK_NULL_HANDLE);

		auto devProps = getPhysicalDeviceProperties(m_physicalDevice);
		auto devFts = getPhysicalDeviceFeatures(m_physicalDevice);
		auto qfProps = getPhysicalDeviceQueueFamilies(m_physicalDevice);
		m_depthStencilFormat = selectDepthFormat(m_physicalDevice);

		uint32_t extPropsCount;
		vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extPropsCount, nullptr);

		if (extPropsCount == 0)
			throw std::runtime_error("vkEnumerateDeviceExtensionProperties returned 0 prop count");

		static constexpr std::array<const char*, 1> requiredExtensions =
		{
			"VK_KHR_swapchain"
		};

		std::vector<VkExtensionProperties> extProps(extPropsCount);
		if (vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extPropsCount, extProps.data()) != VK_SUCCESS)
			throw std::runtime_error("vkEnumerateDeviceExtensionProperties failed");

		for (const auto& requiredExtension : requiredExtensions)
		{
			bool found = false;
			for (const auto& extProp : extProps)
			{
				if (strcmp(extProp.extensionName, requiredExtension) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				throw std::runtime_error("Required device extension wasn't found");
		}

		for (uint32_t i = 0; i < qfProps.size(); i++)
		{
			auto& props = qfProps[i];

			if (props.queueCount > 0 && BITFIELD_TRUE(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
			{
				m_gfxQueueIx = i;
				break;
			}
		}

		static constexpr float gfxQueuePrio = 1.0f;

		const VkDeviceQueueCreateInfo queueCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = m_gfxQueueIx,
			.queueCount = 1,
			.pQueuePriorities = &gfxQueuePrio
		};

		const VkDeviceCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),
			.pEnabledFeatures = &devFts
		};

		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a logical device.");

		volkLoadDevice(m_device);

		vkGetDeviceQueue(m_device, m_gfxQueueIx, 0, &m_gfxQueue);
	}

	VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface) const
	{
		assert(m_physicalDevice != VK_NULL_HANDLE);

		VkSurfaceCapabilitiesKHR sc;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &sc);

		return sc;
	}

	std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkSurfaceKHR surface) const
	{
		assert(m_physicalDevice != VK_NULL_HANDLE);

		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, nullptr);

		std::vector<VkSurfaceFormatKHR> formats(count);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, formats.data()) != VK_SUCCESS)
			throw std::runtime_error("vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

		return formats;
	}

	std::vector<VkPresentModeKHR> getSurfacePresentModes(VkSurfaceKHR surface) const
	{
		assert(m_physicalDevice != VK_NULL_HANDLE);

		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, nullptr);

		std::vector<VkPresentModeKHR> pms(count);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, pms.data()) != VK_SUCCESS)
			throw std::runtime_error("vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

		return pms;
	}

	void initSwapchain()
	{
		if (!SDL_Vulkan_CreateSurface(m_pWindow, m_pInstance, nullptr, &m_surface))
			throw std::runtime_error("Failed to create a surface");

		auto sfCaps = getSurfaceCapabilities(m_surface);
		auto sfFormats = getSurfaceFormats(m_surface);
		auto sfPms = getSurfacePresentModes(m_surface);

		uint32_t minImageCount = MAX_FRAMES_IN_FLIGHT;
		if (sfCaps.maxImageCount > 0)
			minImageCount = std::clamp(minImageCount, sfCaps.minImageCount, sfCaps.maxImageCount);

		m_swapchainImgFormat = sfFormats[0];
		for (const auto& sfFormat : sfFormats)
		{
			if (sfFormat.format == VK_FORMAT_R8G8B8A8_SRGB
				&& sfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				m_swapchainImgFormat = sfFormat;
				break;
			}
		}

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& pm : sfPms)
		{
			if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
				presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}

		const VkSwapchainCreateInfoKHR scCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = m_surface,
			.minImageCount = minImageCount,
			.imageFormat = m_swapchainImgFormat.format,
			.imageColorSpace = m_swapchainImgFormat.colorSpace,
			.imageExtent = sfCaps.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &m_gfxQueueIx,
			.preTransform = sfCaps.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_FALSE,
			.oldSwapchain = VK_NULL_HANDLE
		};

		if (vkCreateSwapchainKHR(m_device, &scCreateInfo, nullptr, &m_swapchain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a swapchain");

		m_swapchainImgWidth = scCreateInfo.imageExtent.width;
		m_swapchainImgHeight = scCreateInfo.imageExtent.height;

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);

		m_swapchainImages.resize(imageCount);
		m_swapchainImageViews.resize(imageCount);
		if (vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to get swapchain images");

		for (uint32_t i = 0; i < imageCount; i++)
		{
			const VkImageViewCreateInfo ivCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = m_swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_swapchainImgFormat.format,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			if (vkCreateImageView(m_device, &ivCreateInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error(fmt::format("vkCreateImageView failed for swapchain image {}", i));
		}
	}

	bool beginPrimaryCmdBuf(VkCommandBuffer cmdBuf)
	{
		// TODO: Implement for secondary buffers too

		const VkCommandBufferBeginInfo begInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.pInheritanceInfo = nullptr
		};

		if (vkBeginCommandBuffer(cmdBuf, &begInfo) != VK_SUCCESS)
			throw std::runtime_error("vkBeginCommandBuffer failed.");
	}

	void initCmdPoolAndBuffers()
	{
		const VkCommandPoolCreateInfo cmdPoolCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = m_gfxQueueIx
		};

		if (vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a command pool");

		const VkCommandBufferAllocateInfo cmdBufAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
		};

		if (vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, m_commandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate a command buffer");
	}

	void initRenderpass()
	{
		const std::array<VkAttachmentDescription, 2> attachmentDescs =
		{
			{
				{
					.format = m_swapchainImgFormat.format,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				},
				{
					.format = m_depthStencilFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				}
			}
		};

		constexpr std::array<VkAttachmentReference, 2> attachmentRefs =
		{
			{
				{
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				},
				{
					.attachment = 1,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				}
			}
		};

		const VkSubpassDescription subpassDesc =
		{
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentRefs[0],
			.pDepthStencilAttachment = &attachmentRefs[1],
		};

		const VkSubpassDependency subpassDep =
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		};

		const VkRenderPassCreateInfo rpInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = static_cast<uint32_t>(attachmentDescs.size()),
			.pAttachments = attachmentDescs.data(),
			.subpassCount = 1,
			.pSubpasses = &subpassDesc,
			.dependencyCount = 1,
			.pDependencies = &subpassDep
		};

		if (vkCreateRenderPass(m_device, &rpInfo, nullptr, &m_renderPass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a renderpass");
	}

	VkShaderModule createShaderModule(size_t codeSize, const uint32_t* pCode) const
	{
		assert(m_device != VK_NULL_HANDLE);

		const VkShaderModuleCreateInfo createInfo
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = codeSize,
			.pCode = pCode
		};

		VkShaderModule module;
		if (vkCreateShaderModule(m_device, &createInfo, nullptr, &module) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a shader module");

		return module;
	}

	void initPipeline()
	{
		assert(m_device != VK_NULL_HANDLE);

		const VkPushConstantRange pcRange =
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(PushConstants)
		};

		const VkPipelineLayoutCreateInfo layoutCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pcRange,
		};

		if (vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a pipeline layout");

		VkShaderModule vertexShaderModule = createShaderModule(shaders::vert_code_size, (const uint32_t*)shaders::vert_code_start);
		VkShaderModule fragmentShaderModule = createShaderModule(shaders::frag_code_size, (const uint32_t*)shaders::frag_code_start);

		const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages
		{
			{
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = vertexShaderModule,
					.pName = "VSMain",
					.pSpecializationInfo = nullptr
				},
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = fragmentShaderModule,
					.pName = "PSMain",
					.pSpecializationInfo = nullptr
				}
			}
		};

		constexpr auto bindingDesc = Vertex::getBindingDescription();
		constexpr auto attrDescs = Vertex::getAttributeDesc();

		const VkPipelineVertexInputStateCreateInfo viState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDesc,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size()),
			.pVertexAttributeDescriptions = attrDescs.data()
		};

		constexpr VkPipelineInputAssemblyStateCreateInfo iaState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		constexpr VkPipelineTessellationStateCreateInfo tessState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.patchControlPoints = 0
		};

		// dynamic
		constexpr VkPipelineViewportStateCreateInfo viewportState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr
		};

		constexpr VkPipelineRasterizationStateCreateInfo rasterInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};

		constexpr VkPipelineMultisampleStateCreateInfo msInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		constexpr VkPipelineDepthStencilStateCreateInfo dsInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		constexpr VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		const VkPipelineColorBlendStateCreateInfo blendState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachmentState,
		};

		constexpr std::array<VkDynamicState, 2> dynamicStates =
		{
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			}
		};

		const VkPipelineDynamicStateCreateInfo dynState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
		};

		const VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &viState,
			.pInputAssemblyState = &iaState,
			.pTessellationState = &tessState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterInfo,
			.pMultisampleState = &msInfo,
			.pDepthStencilState = &dsInfo,
			.pColorBlendState = &blendState,
			.pDynamicState = &dynState,
			.layout = m_pipelineLayout,
			.renderPass = m_renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};

		if (vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
		{
			vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
			vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
			throw std::runtime_error("Failed to create a graphics pipeline");
		}

		vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) &&
				BITFIELD_TRUE(m_memoryProperties.memoryTypes[i].propertyFlags, properties))
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type");
	}

	void initDeviceMemory()
	{
		assert(m_physicalDevice != VK_NULL_HANDLE);
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

		const VkImageCreateInfo diCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = m_depthStencilFormat,
			.extent =
			{
				.width = m_swapchainImgWidth,
				.height = m_swapchainImgHeight,
				.depth = 1u
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &m_gfxQueueIx,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		if (vkCreateImage(m_device, &diCreateInfo, nullptr, &m_depthImage) != VK_SUCCESS)
			throw std::runtime_error("Failed to create depth image");

		VkMemoryRequirements diMemRequirements;
		vkGetImageMemoryRequirements(m_device, m_depthImage, &diMemRequirements);

		const VkMemoryAllocateInfo allocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = diMemRequirements.size,
			.memoryTypeIndex = findMemoryType(
				diMemRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			),
		};

		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthImageMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate memory for depth image");

		if (vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0) != VK_SUCCESS)
			throw std::runtime_error("Failed to bind memory for depth image");

		const VkImageViewCreateInfo diViewCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = m_depthImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_depthStencilFormat,
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(m_device, &diViewCreateInfo, nullptr, &m_depthImageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view for depth image");
	}

	void initFramebuffers()
	{
		m_swapchainFramebuffers.resize(m_swapchainImages.size());

		for (size_t i = 0; i < m_swapchainFramebuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments = {
				m_swapchainImageViews[i],
				m_depthImageView
			};

			const VkFramebufferCreateInfo fbCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_renderPass,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.width = m_swapchainImgWidth,
				.height = m_swapchainImgHeight,
				.layers = 1
			};

			if (vkCreateFramebuffer(m_device, &fbCreateInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error(fmt::format("Failed to create a framebuffer {}", i));
		}
	}
	
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) const
	{
		assert(m_device != VK_NULL_HANDLE);

		const VkBufferCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &m_gfxQueueIx,
		};

		if (vkCreateBuffer(m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a buffer");

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &requirements);

		const VkMemoryAllocateInfo allocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = requirements.size,
			.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties)
		};

		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate buffer's memory");

		vkBindBufferMemory(m_device, buffer, memory, 0);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
	{
		assert(m_commandPool != VK_NULL_HANDLE);

		const VkCommandBufferAllocateInfo cmdBufAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer cmdBuf;
		vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, &cmdBuf);

		constexpr VkCommandBufferBeginInfo beginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(cmdBuf);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf;

		vkQueueSubmit(m_gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_gfxQueue);

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmdBuf);
	}

	void loadObj(const std::string_view path)
	{
		tinyobj::ObjReaderConfig conf;
		conf.mtl_search_path = "./";

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(path.data(), conf))
		{
			if (!reader.Error().empty())
				throw std::runtime_error(fmt::format("tinyobjloader: {}", reader.Error()));
			else
				throw std::runtime_error("tinyobjloader: Unknown error");
		}

		if (!reader.Warning().empty())
			fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "tinyobjloader: {}\n", reader.Warning());

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (const auto& shape : shapes)
		{
			size_t index_offset = 0;
			for (const auto& fv : shape.mesh.num_face_vertices)
			{
				for (size_t v = 0; v < fv; v++)
				{
					Vertex vtx{};
					tinyobj::index_t ix = shape.mesh.indices[index_offset + v];
					vtx.pos.x = attrib.vertices[3 * ix.vertex_index + 0];
					vtx.pos.y = attrib.vertices[3 * ix.vertex_index + 1];
					vtx.pos.z = attrib.vertices[3 * ix.vertex_index + 2];
					vertices.emplace_back(std::move(vtx));
					indices.emplace_back(ix.vertex_index);
				}

				index_offset += fv;
			}
		}

		// create buffers and copy data
		const VkDeviceSize bufSize = vertices.size() * sizeof(Vertex);
		createBuffer(bufSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_vertexBuffer, m_vertexBufferMemory);

		createBuffer(bufSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_stagingBuffer, m_stagingBufferMemory);

		createBuffer(bufSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_indexBuffer, m_indexBufferMemory);

		vkMapMemory(m_device, m_stagingBufferMemory, 0, bufSize, 0, &m_pStagingBuffer);
		std::memcpy(m_pStagingBuffer, vertices.data(), bufSize);
		copyBuffer(m_stagingBuffer, m_vertexBuffer, bufSize);

		std::memcpy(m_pStagingBuffer, indices.data(), indices.size() * sizeof(uint32_t));
		copyBuffer(m_stagingBuffer, m_indexBuffer, indices.size() * sizeof(uint32_t));
		m_indices = indices.size();
	}

	glm::mat4 calculateMVP(float aspect)
	{
		glm::mat4 model = glm::identity<glm::mat4>();

		glm::mat4 view = glm::lookAt(
			glm::vec3(2.0f, 2.0f, 2.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);

		glm::mat4 proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 10.0f);
		proj[1][1] *= -1;

		return proj * view * model;
	}

	void update()
	{
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_EVENT_QUIT)
				m_isRunning = false;
		}

		m_mvp = calculateMVP(m_swapchainImgWidth / m_swapchainImgHeight);
	}

	void createSyncObjects()
	{
		constexpr VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		constexpr VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
			vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);
		}
	}

	void cleanup()
	{
		vkDeviceWaitIdle(m_device);

		vkUnmapMemory(m_device, m_stagingBufferMemory);
		vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
		vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
		vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
		vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
		vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
		vkFreeMemory(m_device, m_stagingBufferMemory, nullptr);

		for (const auto& framebuffer : m_swapchainFramebuffers)
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);

		vkDestroyImageView(m_device, m_depthImageView, nullptr);
		vkDestroyImage(m_device, m_depthImage, nullptr);
		vkFreeMemory(m_device, m_depthImageMemory, nullptr);
		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		for (const auto& imageView : m_swapchainImageViews)
			vkDestroyImageView(m_device, imageView, nullptr);

		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		SDL_Vulkan_DestroySurface(m_pInstance, m_surface, nullptr);
		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		vkDestroyCommandPool(m_device, m_commandPool, nullptr);
		vkDestroyDevice(m_device, nullptr);
		vkDestroyDebugUtilsMessengerEXT(m_pInstance, m_debugMsger, nullptr);
		vkDestroyInstance(m_pInstance, nullptr);

		SDL_DestroyWindow(m_pWindow);
		SDL_Quit();
	}

	void recordRenderingBuffer(VkCommandBuffer cmdBuf, uint32_t imgIx)
	{
		VkCommandBufferBeginInfo beginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
		};

		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		std::array<VkClearValue, 2> clearVals{};
		clearVals[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
		clearVals[1].depthStencil = { 1.0f, 0 };

		const VkRenderPassBeginInfo renderPassInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_renderPass,
			.framebuffer = m_swapchainFramebuffers[imgIx],
			.renderArea =
			{
				.offset = {0, 0},
				.extent = { m_swapchainImgWidth, m_swapchainImgHeight }
			},
			.clearValueCount = static_cast<uint32_t>(clearVals.size()),
			.pClearValues = clearVals.data()
		};

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		const VkViewport viewport =
		{
			.x = 0,
			.y = 0,
			.width = static_cast<float>(m_swapchainImgWidth),
			.height = static_cast<float>(m_swapchainImgHeight),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		const VkRect2D scissor =
		{
			.offset = { 0u, 0u },
			.extent = { m_swapchainImgWidth, m_swapchainImgHeight}
		};

		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const PushConstants pushData =
		{
			.mvp = m_mvp
		};

		vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushData);
		vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(m_indices), 1, 0, 0, 0);
		vkCmdEndRenderPass(cmdBuf);

		if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS)
			throw std::runtime_error("Failed to record a command buffer");
	}

	void render()
	{
		vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// TODO: recreate swapchain
			throw std::runtime_error("Not implemented");
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to acquire the next image");

		vkResetFences(m_device, 1, &m_inFlightFences[imageIndex]);

		VkCommandBuffer cmdBuf = m_commandBuffers[imageIndex];
		vkResetCommandBuffer(cmdBuf, 0);
		recordRenderingBuffer(cmdBuf, imageIndex);

		const VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		const VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		const VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmdBuf,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		if (vkQueueSubmit(m_gfxQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command!");

		const VkPresentInfoKHR presentInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_renderFinishedSemaphores[imageIndex],
			.swapchainCount = 1,
			.pSwapchains = &m_swapchain,
			.pImageIndices = &imageIndex
		};

		vkQueuePresentKHR(m_gfxQueue, &presentInfo);
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	static VkBool32 DebugMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			fmt::print(fg(fmt::color::red), "Validation Layer Error: {}\n", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			fmt::print(fg(fmt::color::yellow), "Validation Layer Warning: {}\n", pCallbackData->pMessage);
			break;
		default:
			fmt::print("Validation Layer Message: {}\n", pCallbackData->pMessage);
			break;
		}

		return VK_TRUE;
	}
};

int main(int argc, char** argv)
{
	argparse::ArgumentParser parser("SimpleVulkanRenderer", "v1.0");
	parser.add_argument("mesh_obj")
		.required();

	try
	{
		parser.parse_args(argc, argv);
	}
	catch (const std::runtime_error& e)
	{
		fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "Argument Parsing Error: {}\n", e.what());
		return -1;
	}

	std::string modelPath = parser.get<std::string>("mesh_obj");
	Application app(modelPath);

	try
	{
		app.run();
	}
	catch (const std::runtime_error& e)
	{
		fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: {}\n", e.what());
		return -1;
	}

	return 0;
}