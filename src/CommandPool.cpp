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

#include "CommandPool.h"

using namespace svr;

std::span<VkCommandBuffer> CommandPool::allocCommandBuffers(VkCommandBufferLevel level, uint32_t count) const
{
	const VkCommandBufferAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_commandPool,
		.level = level,
		.commandBufferCount = count
	};

	const size_t lastSize = m_commandBuffers.size();
	m_commandBuffers.resize(lastSize + count);

	VkCommandBuffer* const offsetPtr = m_commandBuffers.data() + lastSize;
	if (vkAllocateCommandBuffers(m_device, &allocInfo, offsetPtr) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");

	return std::span<VkCommandBuffer>(offsetPtr, count);
}

void CommandPool::freeCommandBuffers(const std::span<VkCommandBuffer> buffers) const
{
	vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(buffers.size()), buffers.data());
}

void CommandPool::destroy()
{
	vkFreeCommandBuffers(m_device, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}
