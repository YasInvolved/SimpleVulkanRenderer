cmake_minimum_required(VERSION 4.1)

project(SimpleVulkanRenderer)

enable_language(ASM_NASM)

find_package(Vulkan 
	REQUIRED
	COMPONENTS dxc
)

add_subdirectory(3rdparty)
add_subdirectory(src)
