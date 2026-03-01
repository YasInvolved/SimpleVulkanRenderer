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
