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

		struct BufferCreateInfo
		{
			VkDeviceSize size;
			VkBufferUsageFlags usage;
			VkSharingMode sharing;
			uint32_t queueFamilyIxCount;
			const uint32_t* queueFamilyIxs;
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
		const std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkSurfaceKHR surface) const;
		const std::vector<VkPresentModeKHR> getPresentModes(VkSurfaceKHR surface) const;

		VkSwapchainKHR createSwapchain(const SwapchainCreateInfo& createInfo) const;
		VkSemaphore createSemaphore() const;
		VkFence createFence(bool signaled = false) const;

		// memory
		VkDeviceMemory allocateMemory(VkDeviceSize size, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) const;
		bool assignObjectToMemory(VkDeviceMemory memory, VkBuffer buffer, VkDeviceSize offset) const;
		bool assignObjectToMemory(VkDeviceMemory memory, VkImage image, VkDeviceSize offset) const;
		void freeMemory(VkDeviceMemory memory) const;
		VkMemoryRequirements getMemoryRequirements(VkBuffer buffer) const;
		VkMemoryRequirements getMemoryRequirements(VkImage image) const;

		VkImage createImage(const VkImageCreateInfo& createInfo) const;
		VkImageView createImageView(VkImage image, const ImageViewCreateInfo& createInfo) const;
		VkBuffer createBuffer(const BufferCreateInfo& createInfo) const;		

		CommandPool createCommandPool(uint32_t queueFamilyindex) const;

		VkShaderModule createShaderModule(size_t codeSize, const uint32_t* pCode) const;
		VkDescriptorSetLayout createDescriptorSetLayout(size_t bindingCount, const VkDescriptorSetLayoutBinding* pBindings) const;

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
