// MIT License
// 
// Copyright(c) 2026 Jakub Bączyk
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <shaders.h>

#include "Window.h"
#include "Device.h"
#include "CommandPool.h"
#include "Camera.h"
#include "Mesh.h"
#include "Light.h"

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

struct PushConstants
{
	glm::mat4 model;
};

struct FrameData
{
	glm::mat4 viewProjection;
	glm::vec3 cameraPos;
	uint32_t lightCount;
};

class Application
{
private:
	static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3ull;
	const std::string_view m_modelPath;
	bool m_isRunning = true;

	using clock_t = std::chrono::high_resolution_clock;

	svr::Window::window_ptr m_window;

	VkInstance m_pInstance = VK_NULL_HANDLE;

	// debug
	VkDebugUtilsMessengerEXT m_debugMsger = VK_NULL_HANDLE;

	// device
	uint32_t m_gfxQueueIx = 0;
	std::unique_ptr<svr::Device> m_device;

	VkQueue m_gfxQueue;

	// surface & swapchain
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	uint32_t m_swapchainImageCount = 0;
	VkExtent2D m_swapchainExtent{};
	VkSurfaceFormatKHR m_swapchainImgFormat = {};
	VkFormat m_depthStencilFormat = VK_FORMAT_D32_SFLOAT;
	VkPresentModeKHR m_currentPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;

	// rendering
	svr::CommandPool m_commandPool;
	std::span<VkCommandBuffer> m_inFlightCmdBuffers;
	VkCommandBuffer m_transferCommandBuffer;

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipelineRenderingCreateInfo m_pipelineRenderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	// scene memory
	VkBuffer m_sceneStagingBuffer = VK_NULL_HANDLE;
	VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
	VkBuffer m_indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_sceneDeviceLocalMemory = VK_NULL_HANDLE;
	VkDeviceMemory m_sceneStagingBufferMemory = VK_NULL_HANDLE;
	void* m_pSceneStagingBuffer = nullptr;

	// per-frame memory
	VkDeviceSize m_alignedUboSize = 0;
	VkDeviceSize m_alignedSsboSize = 0;
	VkBuffer m_ubos = VK_NULL_HANDLE; // CHAD BUFFER
	VkBuffer m_ssbos = VK_NULL_HANDLE; // CHAD BUFFER
	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> m_zBuffers;
	std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> m_zBufferViews;
	VkDeviceMemory m_zBuffersMemory = VK_NULL_HANDLE;
	VkDeviceMemory m_ubosMemory = VK_NULL_HANDLE;
	VkDeviceMemory m_ssbosMemory = VK_NULL_HANDLE;
	void* m_pUbos = nullptr;
	void* m_pSsbos = nullptr;

	// per-frame descriptor pool and sets
	VkDescriptorSetLayout m_perFrameDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_perFrameDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet m_perFrameDescriptorSet = VK_NULL_HANDLE;

	// sync
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores{ VK_NULL_HANDLE };
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderingFinishedSemaphores{ VK_NULL_HANDLE };
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences{ VK_NULL_HANDLE };

	svr::Mesh m_mesh;
	svr::Camera m_camera;

	// Imgui
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	uint32_t m_currentFrame = 0;
	float m_deltaTime = 0.0f;

	std::array<svr::Light, 2> m_lights =
	{
		{
			{
				.positionAndRadius = glm::vec4(0.0f, 2.0f, -2.0f, 1.0f),
				.colorAndIntensity = glm::vec4(1.0f, 1.0f, 0.0f, 150.0f)
			},
			{
				.positionAndRadius = glm::vec4(0.0f, 0.0f, -6.0f, 5.0f),
				.colorAndIntensity = glm::vec4(0.0f, 1.0f, 1.0f, 150.0f)
			}
		}
	};

	static constexpr std::array<VkPushConstantRange, 1> m_pushConstantsInfo =
	{
		{
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(PushConstants)
			},
		}
	};

public:
	Application(const std::string_view modelPath) 
		: m_modelPath(modelPath)
	{}

	void run()
	{
		initialize();

		auto lastTime = clock_t::now();
		while (m_isRunning)
		{
			auto currentTime = clock_t::now();
			m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
			lastTime = currentTime;

			update();
			render();
		}

		cleanup();
	}

private:
	void initialize()
	{
		m_window = svr::Window::Get();
		m_window->setKeyboardInputHandler([this](bool down, bool repeat, SDL_Scancode scancode)
			{
				inputHandler(down, repeat, scancode);
			}
		);

		m_mesh = svr::Mesh::loadObjMesh(m_modelPath);

		initVkInstance();
		selectPhysicalDevice();
		initDevice();
		createSwapchain();
		initSync();
		initCmdPoolAndBuffers();
		initSceneResources();
		initPipeline();
		initPerFrameResources();
		initDescPool();
		initImgui();
		initCamera();
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
			.ppEnabledExtensionNames = requiredExtensions.data(),
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
			{
				m_device = std::make_unique<svr::Device>(physicalDevice);
				break;
			}
		}

		if (!m_device)
			throw std::runtime_error("No suitable device for graphics rendering found");
	}

	void initDevice()
	{
		auto& devProps = m_device->getProperties();
		auto& devFts = m_device->getFeatures();
		auto& qfProps = m_device->getQueueFamilyProperties();
		m_depthStencilFormat = selectDepthFormat(m_device->getPhysicalDevice());

		static constexpr std::array<const char*, 1> requiredExtensions =
		{
			"VK_KHR_swapchain"
		};

		for (const auto& requiredExtension : requiredExtensions)
		{
			bool found = false;
			for (const auto& extProp : m_device->getExtensionProperties())
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

		const VkPhysicalDeviceDynamicRenderingFeatures dynRenderingFeatures =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
			.pNext = nullptr,
			.dynamicRendering = VK_TRUE
		};

		const svr::Device::InitInfo initInfo =
		{
			.queueCreateInfosCount = 1,
			.queueCreateInfos = &queueCreateInfo,
			.extensionsCount = static_cast<uint32_t>(requiredExtensions.size()),
			.extensions = requiredExtensions.data(),
			.dynamicRenderingEnabled = true,
			.dynamicRenderingFeatures = &dynRenderingFeatures
		};

		m_device->initialize(initInfo);

		VkDevice device = m_device->getDevice();
		volkLoadDevice(device);
		vkGetDeviceQueue(device, m_gfxQueueIx, 0, &m_gfxQueue);

		// check push constants size
		const uint32_t maxPcSize = m_device->getProperties().properties.limits.maxPushConstantsSize;
		for (const auto& pcInfo : m_pushConstantsInfo)
		{
			if (pcInfo.size >= maxPcSize)
				throw std::runtime_error("CRITCAL: Push constant size exceeded. Please wait until it's fixed");
		}
	}

	void createSwapchain()
	{
		VkDevice device = m_device->getDevice();
		vkDeviceWaitIdle(device);

		if (m_surface == VK_NULL_HANDLE)
			m_surface = m_window->createSurface(m_pInstance);

		if (m_swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, m_swapchain, nullptr);

			for (const auto imageView : m_swapchainImageViews)
				vkDestroyImageView(device, imageView, nullptr);
		}

		auto sfCaps = m_device->getSurfaceCapabilities(m_surface);
		auto sfFormats = m_device->getSurfaceFormats(m_surface);
		auto sfPms = m_device->getPresentModes(m_surface);

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

		for (const auto& pm : sfPms)
		{
			if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
				m_currentPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}

		m_swapchainExtent = sfCaps.currentExtent;

		const svr::Device::SwapchainCreateInfo createInfo =
		{
			.surface = m_surface,
			.minImageCount = minImageCount,
			.imageFormat = m_swapchainImgFormat.format,
			.colorSpace = m_swapchainImgFormat.colorSpace,
			.imageExtent = m_swapchainExtent,
			.queueFamilyIndicesCount = 1,
			.queueFamilyIndices = &m_gfxQueueIx,
			.transform = sfCaps.currentTransform,
			.presentMode = m_currentPresentMode,
			.oldSwapchain = nullptr
		};

		m_swapchain = m_device->createSwapchain(createInfo);

		vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainImageCount, nullptr);

		m_swapchainImages.resize(m_swapchainImageCount);
		m_swapchainImageViews.resize(m_swapchainImageCount);
		if (vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainImageCount, m_swapchainImages.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to get swapchain images");

		for (uint32_t i = 0; i < m_swapchainImageCount; i++)
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

			if (vkCreateImageView(device, &ivCreateInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error(fmt::format("vkCreateImageView failed for swapchain image {}", i));
		}
	}

	void initSync()
	{
		VkDevice device = m_device->getDevice();

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_imageAvailableSemaphores[i] = m_device->createSemaphore();
			m_renderingFinishedSemaphores[i] = m_device->createSemaphore();
			m_inFlightFences[i] = m_device->createFence(true);
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
		m_commandPool = m_device->createCommandPool(m_gfxQueueIx);
		auto tmp = m_commandPool.allocCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT + 1);
		m_inFlightCmdBuffers = std::span<VkCommandBuffer>(tmp.begin(), tmp.end() - 1);
		m_transferCommandBuffer = *tmp.rbegin();
	}

	void initSceneResources()
	{
		// since we're rendering one, static object for now, it's a scene resource
		// it lives until the application exits

		// for now, all of the scene resources live in device memory
		constexpr auto DEVICE_LOCAL_MEMORY_PROPERTIES = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		constexpr auto HOST_VISIBLE_MEMORY_PROPERTIES = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		const auto& vertices = m_mesh.getVertices();
		const auto& indices = m_mesh.getIndices();

		const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(svr::Mesh::Vertex);
		const VkDeviceSize indexBufferSize = indices.size() * sizeof(svr::Mesh::Index_t);
		const VkDeviceSize stagingBufferSize = std::max(vertexBufferSize, indexBufferSize);

		const svr::Device::BufferCreateInfo vertexCreateInfo =
		{
			.size = vertexBufferSize,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharing = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIxCount = 1,
			.queueFamilyIxs = &m_gfxQueueIx
		};

		const svr::Device::BufferCreateInfo indexCreateInfo =
		{
			.size = indexBufferSize,
			.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharing = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIxCount = 1,
			.queueFamilyIxs = &m_gfxQueueIx
		};

		const svr::Device::BufferCreateInfo stagingCreateInfo =
		{
			.size = stagingBufferSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharing = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIxCount = 1,
			.queueFamilyIxs = &m_gfxQueueIx
		};

		m_vertexBuffer = m_device->createBuffer(vertexCreateInfo);
		m_indexBuffer = m_device->createBuffer(indexCreateInfo);
		m_sceneStagingBuffer = m_device->createBuffer(stagingCreateInfo);

		VkDevice device = m_device->getDevice();
		VkMemoryRequirements vertexRequirements;
		VkMemoryRequirements indexRequirements;
		VkMemoryRequirements stagingRequirements;
		vkGetBufferMemoryRequirements(device, m_vertexBuffer, &vertexRequirements);
		vkGetBufferMemoryRequirements(device, m_indexBuffer, &indexRequirements);
		vkGetBufferMemoryRequirements(device, m_sceneStagingBuffer, &stagingRequirements);

		// combine vertex and index as 1 allocation to prevent fragmentation
		const VkDeviceSize totalDeviceLocal = vertexRequirements.size + indexRequirements.size;
		const auto deviceLocalTypeFilters = vertexRequirements.memoryTypeBits | indexRequirements.memoryTypeBits;
		m_sceneDeviceLocalMemory = m_device->allocateMemory(totalDeviceLocal, deviceLocalTypeFilters, DEVICE_LOCAL_MEMORY_PROPERTIES);
		m_device->assignObjectToMemory(m_sceneDeviceLocalMemory, m_vertexBuffer, 0);
		m_device->assignObjectToMemory(m_sceneDeviceLocalMemory, m_indexBuffer, vertexRequirements.size);

		// now staging buffer
		m_sceneStagingBufferMemory = m_device->allocateMemory(stagingRequirements.size, stagingRequirements.memoryTypeBits, HOST_VISIBLE_MEMORY_PROPERTIES);
		m_device->assignObjectToMemory(m_sceneStagingBufferMemory, m_sceneStagingBuffer, 0);

		// map staging buffer
		vkMapMemory(device, m_sceneStagingBufferMemory, 0, stagingBufferSize, 0, &m_pSceneStagingBuffer);

		// copy scene data
		std::memcpy(m_pSceneStagingBuffer, vertices.data(), vertexBufferSize);
		copyBuffer(m_sceneStagingBuffer, m_vertexBuffer, vertexBufferSize, 0, 0);

		std::memcpy(m_pSceneStagingBuffer, indices.data(), indexBufferSize);
		copyBuffer(m_sceneStagingBuffer, m_indexBuffer, indexBufferSize, 0, 0);
	}

	void initPerFrameResources()
	{
		constexpr auto DEVICE_LOCAL_MEMORY_PROPERTIES = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		constexpr auto HOST_VISIBLE_MEMORY_PROPERTIES = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VkDevice device = m_device->getDevice();

		{
			const VkImageCreateInfo zBufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = m_depthStencilFormat,
				.extent = {
					m_swapchainExtent.width,
					m_swapchainExtent.height,
					1u
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

			const svr::Device::ImageViewCreateInfo zBufferViewCreateInfo =
			{
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_depthStencilFormat,
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};

			for (auto& image : m_zBuffers)
				image = m_device->createImage(zBufferCreateInfo);

			VkMemoryRequirements zBufferRequirements;
			vkGetImageMemoryRequirements(device, m_zBuffers[0], &zBufferRequirements);

			const VkDeviceSize totalAllocationSize = zBufferRequirements.size * m_zBuffers.size();
			m_zBuffersMemory = m_device->allocateMemory(totalAllocationSize, zBufferRequirements.memoryTypeBits, DEVICE_LOCAL_MEMORY_PROPERTIES);

			VkDeviceSize offset = 0;
			for (size_t i = 0; i < m_zBuffers.size(); i++)
			{
				auto& image = m_zBuffers[i];
				m_device->assignObjectToMemory(m_zBuffersMemory, image, offset);
				m_zBufferViews[i] = m_device->createImageView(image, zBufferViewCreateInfo);
				offset += zBufferRequirements.size;
			}
		}

		const auto& deviceLimits = m_device->getProperties().properties.limits;

		m_alignedUboSize = sizeof(FrameData);
		if (m_alignedUboSize % deviceLimits.minUniformBufferOffsetAlignment > 0)
			m_alignedUboSize = ((m_alignedUboSize / deviceLimits.minUniformBufferOffsetAlignment) + 1) * deviceLimits.minUniformBufferOffsetAlignment;

		const VkDeviceSize ubosSize = m_alignedUboSize * MAX_FRAMES_IN_FLIGHT;

		const svr::Device::BufferCreateInfo uboCreateInfo =
		{
			.size = ubosSize,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharing = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIxCount = 1,
			.queueFamilyIxs = &m_gfxQueueIx
		};

		m_ubos = m_device->createBuffer(uboCreateInfo);
		VkMemoryRequirements ubosRequirements;
		vkGetBufferMemoryRequirements(device, m_ubos, &ubosRequirements);

		m_ubosMemory = m_device->allocateMemory(ubosRequirements.size, ubosRequirements.memoryTypeBits, HOST_VISIBLE_MEMORY_PROPERTIES);
		m_device->assignObjectToMemory(m_ubosMemory, m_ubos, 0);

		vkMapMemory(device, m_ubosMemory, 0, ubosSize, 0, &m_pUbos);

		m_alignedSsboSize = m_lights.size() * sizeof(svr::Light);
		if (m_alignedSsboSize % deviceLimits.minStorageBufferOffsetAlignment > 0)
			m_alignedSsboSize = ((m_alignedSsboSize / deviceLimits.minStorageBufferOffsetAlignment) + 1) * deviceLimits.minStorageBufferOffsetAlignment;

		const VkDeviceSize ssbosSize = m_alignedSsboSize * MAX_FRAMES_IN_FLIGHT;

		const svr::Device::BufferCreateInfo ssbosCreateInfo =
		{
			.size = ssbosSize,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharing = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIxCount = 1,
			.queueFamilyIxs = &m_gfxQueueIx
		};

		m_ssbos = m_device->createBuffer(ssbosCreateInfo);

		VkMemoryRequirements ssbosRequirements;
		VkMemoryRequirements stagingRequirements;
		vkGetBufferMemoryRequirements(device, m_ssbos, &ssbosRequirements);

		m_ssbosMemory = m_device->allocateMemory(ssbosRequirements.size, ssbosRequirements.memoryTypeBits, HOST_VISIBLE_MEMORY_PROPERTIES);
		m_device->assignObjectToMemory(m_ssbosMemory, m_ssbos, 0);

		// map ssbo
		vkMapMemory(device, m_ssbosMemory, 0, ssbosSize, 0, &m_pSsbos);

		// descriptor pool and sets
		constexpr std::array<VkDescriptorPoolSize, 2> poolSizes =
		{
			{
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					.descriptorCount = 1
				},
				{
					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
					.descriptorCount = 1
				}
			}
		};

		const VkDescriptorPoolCreateInfo poolCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = 1,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};

		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &m_perFrameDescriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create per-frame descriptor pool");

		const VkDescriptorSetAllocateInfo setAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = m_perFrameDescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &m_perFrameDescriptorSetLayout,
		};

		if (vkAllocateDescriptorSets(device, &setAllocInfo, &m_perFrameDescriptorSet) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate per-frame descriptor sets");

		const std::array<VkDescriptorBufferInfo, 2> bufferInfos =
		{
			{
				{
					.buffer = m_ubos,
					.offset = 0,
					.range = m_alignedUboSize
				},
				{
					.buffer = m_ssbos,
					.offset = 0,
					.range = m_alignedSsboSize
				}
			}
		};

		const std::array<VkWriteDescriptorSet, 2> writes =
		{
			{
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = m_perFrameDescriptorSet,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					.pBufferInfo = &bufferInfos[0]
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = m_perFrameDescriptorSet,
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
					.pBufferInfo = &bufferInfos[1]
				}
			}
		};

		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(writes.size()),
			writes.data(),
			0, nullptr
		);
	}

	void initPipeline()
	{
		{
			constexpr std::array<VkDescriptorSetLayoutBinding, 2> bindings =
			{
				{
					// UBO
					{
						.binding = 0,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
						.descriptorCount = 1,
						.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						.pImmutableSamplers = nullptr
					},
					// SSBO
					{
						.binding = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
						.descriptorCount = 1,
						.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
						.pImmutableSamplers = nullptr
					}
				}
			};

			const VkDescriptorSetLayoutCreateInfo layoutInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.bindingCount = static_cast<uint32_t>(bindings.size()),
				.pBindings = bindings.data()
			};

			if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_perFrameDescriptorSetLayout) != VK_SUCCESS)
				throw std::runtime_error("Failed to create descriptor set layout");
		}

		const VkPipelineLayoutCreateInfo layoutCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &m_perFrameDescriptorSetLayout,
			.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantsInfo.size()),
			.pPushConstantRanges = m_pushConstantsInfo.data(),
		};

		if (vkCreatePipelineLayout(m_device->getDevice(), &layoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a pipeline layout");

		VkShaderModule vertexShaderModule = m_device->createShaderModule(shaders::vert_code_size, reinterpret_cast<const uint32_t*>(shaders::vert_code_start));
		VkShaderModule fragmentShaderModule = m_device->createShaderModule(shaders::frag_code_size, reinterpret_cast<const uint32_t*>(shaders::frag_code_start));

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

		constexpr auto bindingDesc = svr::Mesh::Vertex::getBindingDescription();
		constexpr auto attrDescs = svr::Mesh::Vertex::getAttributeDesc();

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
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
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

		m_pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		m_pipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchainImgFormat.format;
		m_pipelineRenderingCreateInfo.depthAttachmentFormat = m_depthStencilFormat;

		const VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &m_pipelineRenderingCreateInfo,
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
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};

		VkDevice device = m_device->getDevice();

		if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
		{
			vkDestroyShaderModule(device, vertexShaderModule, nullptr);
			vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
			throw std::runtime_error("Failed to create a graphics pipeline");
		}

		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
	}

	void initDescPool()
	{
		// imgui
		static constexpr std::array<VkDescriptorPoolSize, 1> poolSizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE } };
		VkDescriptorPoolCreateInfo poolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 0,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data(),
		};

		for (auto& poolSize : poolSizes)
			poolInfo.maxSets = poolSize.descriptorCount;

		if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool for imgui");
	}

	void initImgui()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		auto im_check_result = [](VkResult result)
			{
				if (result != VK_SUCCESS)
					throw std::runtime_error("Failed to initialize imgui");
			};

		ImGui_ImplVulkan_InitInfo initInfo =
		{
			.ApiVersion = VK_API_VERSION_1_3,
			.Instance = m_pInstance,
			.PhysicalDevice = m_device->getPhysicalDevice(),
			.Device = m_device->getDevice(),
			.QueueFamily = m_gfxQueueIx,
			.Queue = m_gfxQueue,
			.DescriptorPool = m_descriptorPool,
			.MinImageCount = m_swapchainImageCount,
			.ImageCount = m_swapchainImageCount,
			.PipelineCache = VK_NULL_HANDLE,
			.PipelineInfoMain = {
				.RenderPass = VK_NULL_HANDLE,
				.Subpass = 0,
				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
				.PipelineRenderingCreateInfo = m_pipelineRenderingCreateInfo
			},
			.UseDynamicRendering = true,
			.CheckVkResultFn = im_check_result,
		};

		auto checkImguiResult = [](bool result)
			{
				if (!result)
					throw std::runtime_error("Failed to initialize imgui");
			};

		checkImguiResult(m_window->initImgui());
		checkImguiResult(ImGui_ImplVulkan_Init(&initInfo));
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		auto& memoryProperties = m_device->getMemoryProperties();

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) &&
				BITFIELD_TRUE(memoryProperties.memoryTypes[i].propertyFlags, properties))
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type");
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) const
	{
		constexpr VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

		vkBeginCommandBuffer(m_transferCommandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		vkCmdCopyBuffer(m_transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(m_transferCommandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_transferCommandBuffer;

		vkQueueSubmit(m_gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_gfxQueue);

		vkResetCommandBuffer(m_transferCommandBuffer, 0);
	}

	void initCamera()
	{
		const float aspect = static_cast<float>(m_swapchainExtent.width) / static_cast<float>(m_swapchainExtent.height);

		m_camera.setPos(0.0f, 3.0f, -5.0f);
		m_camera.setLookingAt(0.0f, 0.0f, 0.0f);
		m_camera.setFov(70u);
		m_camera.setAspectRatio(aspect);
	}
	
	void update()
	{
		m_window->handleEvents();

		if (m_window->shouldClose())
			m_isRunning = false;
	}

	void cleanup()
	{
		VkDevice device = m_device->getDevice();

		vkDeviceWaitIdle(device);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

		vkDestroyBuffer(device, m_vertexBuffer, nullptr);
		vkDestroyBuffer(device, m_indexBuffer, nullptr);
		m_device->freeMemory(m_sceneDeviceLocalMemory);
		vkDestroyBuffer(device, m_sceneStagingBuffer, nullptr);
		vkUnmapMemory(device, m_sceneStagingBufferMemory);
		m_device->freeMemory(m_sceneStagingBufferMemory);

		vkDestroyDescriptorPool(device, m_perFrameDescriptorPool, nullptr);
		vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyImageView(device, m_zBufferViews[i], nullptr);
			vkDestroyImage(device, m_zBuffers[i], nullptr);
		}

		vkFreeMemory(device, m_zBuffersMemory, nullptr);
	
		vkDestroyBuffer(device, m_ssbos, nullptr);
		vkDestroyBuffer(device, m_ubos, nullptr);
		vkUnmapMemory(device, m_ssbosMemory);
		vkUnmapMemory(device, m_ubosMemory);
		vkFreeMemory(device, m_ssbosMemory, nullptr);
		vkFreeMemory(device, m_ubosMemory, nullptr);

		vkDestroyPipeline(device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, m_perFrameDescriptorSetLayout, nullptr);

		for (uint32_t i = 0; i < m_swapchainImageCount; i++)
		{
			vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_renderingFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_inFlightFences[i], nullptr);
			vkDestroyImageView(device, m_swapchainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		SDL_Vulkan_DestroySurface(m_pInstance, m_surface, nullptr);
		m_commandPool.destroy();
		m_device->destroy();
		vkDestroyDebugUtilsMessengerEXT(m_pInstance, m_debugMsger, nullptr);
		vkDestroyInstance(m_pInstance, nullptr);

		SDL_Quit();
	}

	void recordCommandBuffer(VkCommandBuffer cmdBuf, uint32_t imgIx)
	{
		const VkDeviceSize uboOffset = imgIx * m_alignedUboSize;
		const VkDeviceSize ssboOffset = imgIx * m_alignedSsboSize;

		FrameData* frameData = reinterpret_cast<FrameData*>(reinterpret_cast<char*>(m_pUbos) + uboOffset);
		frameData->cameraPos = m_camera.getPos();
		frameData->viewProjection = m_camera.getViewProjection();
		frameData->lightCount = static_cast<uint32_t>(m_lights.size());

		void* lightArrayPtr = reinterpret_cast<char*>(m_pSsbos) + ssboOffset;
		std::memcpy(lightArrayPtr, m_lights.data(), sizeof(svr::Light) * m_lights.size());

		const VkCommandBufferBeginInfo beginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		if (vkBeginCommandBuffer(cmdBuf, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording a command buffer");

		std::array<VkImageMemoryBarrier, 2> imageBarriers =
		{
			{
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = m_swapchainImages[imgIx],
					.subresourceRange =
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				},
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					.image = m_zBuffers[imgIx],
					.subresourceRange =
					{
						.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				}
			}
		};

		vkCmdPipelineBarrier(
			cmdBuf,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarriers[1]
		);

		vkCmdPipelineBarrier(
			cmdBuf,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarriers[0]
		);

		const std::array<VkRenderingAttachmentInfo, 2> attachmentInfos =
		{
			{
				{
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = m_swapchainImageViews[imgIx],
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.clearValue = {.color = {0.0f, 0.0f, 0.0f } }
				},
				{
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = m_zBufferViews[imgIx],
					.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.clearValue = {.depthStencil = { 1.0f, 0u } }
				}
			}
		};

		const VkRenderingInfo renderingInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = { { 0u, 0u }, m_swapchainExtent },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentInfos[0],
			.pDepthAttachment = &attachmentInfos[1]
		};

		vkCmdBeginRendering(cmdBuf, &renderingInfo);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
		
		const VkViewport viewport =
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(m_swapchainExtent.width),
			.height = static_cast<float>(m_swapchainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		const VkRect2D scissor =
		{
			.offset = { 0, 0 },
			.extent = m_swapchainExtent
		};
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		constexpr VkDeviceSize VERTEX_BUFFER_OFFSET = 0ull;
		vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_vertexBuffer, &VERTEX_BUFFER_OFFSET);
		vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const uint32_t offsets[] = {
			static_cast<uint32_t>(uboOffset),
			static_cast<uint32_t>(ssboOffset)
		};

		vkCmdBindDescriptorSets(
			cmdBuf,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			0, 1,
			&m_perFrameDescriptorSet,
			2, offsets
		);

		PushConstants pc = { m_mesh.getModel() };
		vkCmdPushConstants(cmdBuf, m_pipelineLayout, m_pushConstantsInfo[0].stageFlags, m_pushConstantsInfo[0].offset, m_pushConstantsInfo[0].size, &pc);

		vkCmdDrawIndexed(cmdBuf, m_mesh.getIndices().size(), 1, 0, 0, 0);

		auto* drawData = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuf);

		vkCmdEndRendering(cmdBuf);

		imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageBarriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageBarriers[0].dstAccessMask = 0;

		vkCmdPipelineBarrier(
			cmdBuf,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarriers[0]
		);

		if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS)
			throw std::runtime_error("Failed to finish command buffer recording");
	}

	void render()
	{
		if (m_window->isMinimized())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return;
		}

		if (m_window->isResized())
		{
			createSwapchain();
			m_window->setResized(false);
			return;
		}

		VkDevice device = m_device->getDevice();

		static constexpr uint64_t WAIT_TIMEOUT = UINT64_MAX;
		vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, WAIT_TIMEOUT);

		uint32_t imgIx;
		VkResult result = vkAcquireNextImageKHR(device, m_swapchain, WAIT_TIMEOUT, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imgIx);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			createSwapchain();
			return;
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to acquire next image");

		vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);

		VkCommandBuffer currentCmdBuf =	m_inFlightCmdBuffers[m_currentFrame];
		vkResetCommandBuffer(currentCmdBuf, 0);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Controls");
		m_mesh.renderImGuiMenu();
		m_camera.renderImGuiMenu();
		ImGui::End();

		svr::Light::renderLightControlsWindow(m_lights);

		ImGui::Render();

		recordCommandBuffer(currentCmdBuf, imgIx);

		constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		const VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame],
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &currentCmdBuf,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_renderingFinishedSemaphores[imgIx],
		};

		if (vkQueueSubmit(m_gfxQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit a command queue");

		const VkPresentInfoKHR presentInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_renderingFinishedSemaphores[imgIx],
			.swapchainCount = 1,
			.pSwapchains = &m_swapchain,
			.pImageIndices = &imgIx
		};

		result = vkQueuePresentKHR(m_gfxQueue, &presentInfo);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
			throw std::runtime_error("Failed to present an image");

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void inputHandler(bool down, bool repeat, SDL_Scancode scancode)
	{
		if (!down)
			return;

		constexpr float CAMERA_MOVEMENT_SPEED = 0.5f;
		const float currentMovementSpeed = CAMERA_MOVEMENT_SPEED * m_deltaTime;

		switch (scancode)
		{
		case SDL_SCANCODE_A:
			m_camera.move(-CAMERA_MOVEMENT_SPEED, 0.0f, 0.0f);
			break;
		case SDL_SCANCODE_S:
			m_camera.move(0.0f, 0.0f, -CAMERA_MOVEMENT_SPEED);
			break;
		case SDL_SCANCODE_D:
			m_camera.move(CAMERA_MOVEMENT_SPEED, 0.0f, 0.0f);
			break;
		case SDL_SCANCODE_W:
			m_camera.move(0.0f, 0.0f, CAMERA_MOVEMENT_SPEED);
			break;
		}
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
