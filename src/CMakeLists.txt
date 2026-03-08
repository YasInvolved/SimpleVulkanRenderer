# MIT License
# 
# Copyright(c) 2026 Jakub Bączyk
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(OUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(shaders EXCLUDE_FROM_ALL)

add_executable(SimpleVulkanRenderer main.cpp "pch.h" "Camera.h" "Mesh.h" "mesh.cpp" "Device.h" "Device.cpp" "ValueCache.h" "CommandPool.h" "CommandPool.cpp")

set_target_properties(SimpleVulkanRenderer PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED True
	RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
)

# also set out dir for libs
set_target_properties(SDL3-shared PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
)

target_link_libraries(SimpleVulkanRenderer 
	PRIVATE SDL3::SDL3 fmt::fmt glm::glm SimpleVulkanRenderer_shaders tinyobjloader argparse::argparse volk::volk Imgui
)

target_precompile_headers(SimpleVulkanRenderer
	PRIVATE pch.h
)

target_compile_definitions(SimpleVulkanRenderer
	PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK
	PRIVATE $<$<CONFIG:Debug>:SVR_DEBUG>
)
