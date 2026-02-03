#include <stdexcept>
#include <span>
#include <vector>
#include <array>
#include <algorithm>

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

class Application
{
private:
	bool m_isRunning = true;
	
	SDL_Window* m_pWindow = nullptr;

	VkInstance m_pInstance = VK_NULL_HANDLE;

	// debug
	VkDebugUtilsMessengerEXT m_debugMsger = VK_NULL_HANDLE;

	// device
	uint32_t m_gfxQueueIx = 0;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	// surface & swapchain
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;

public:
	Application() {}
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
		m_pWindow = SDL_CreateWindow("SimpleVulkanRenderer", windowResW, windowResH, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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
				if (props.queueCount > 0 && BITFIELD_TRUE(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
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

		uint32_t minImageCount = 2;
		if (sfCaps.maxImageCount > 0)
			minImageCount = std::clamp(minImageCount, sfCaps.minImageCount, sfCaps.maxImageCount);

		VkSurfaceFormatKHR& format = sfFormats[0];
		for (const auto& sfFormat : sfFormats)
		{
			if (sfFormat.format == VK_FORMAT_R8G8B8A8_SRGB
				&& sfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				format = sfFormat;
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
			.imageFormat = format.format,
			.imageColorSpace = format.colorSpace,
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
				.format = format.format,
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

	void cleanup()
	{
		for (const auto& imageView : m_swapchainImageViews)
			vkDestroyImageView(m_device, imageView, nullptr);

		if (m_swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

		if (m_surface != VK_NULL_HANDLE)
			SDL_Vulkan_DestroySurface(m_pInstance, m_surface, nullptr);

		if (m_device != VK_NULL_HANDLE)
			vkDestroyDevice(m_device, nullptr);

		if (m_debugMsger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(m_pInstance, m_debugMsger, nullptr);

		if (m_pInstance != VK_NULL_HANDLE)
			vkDestroyInstance(m_pInstance, nullptr);

		if (m_pWindow != nullptr)
			SDL_DestroyWindow(m_pWindow);

		SDL_Quit();
	}

	void update()
	{
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_EVENT_QUIT)
				m_isRunning = false;
		}
	}

	void render()
	{

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
	Application app;

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