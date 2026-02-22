set(OUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(shaders)

add_executable(SimpleVulkanRenderer main.cpp "pch.h" "Camera.h" "Mesh.h" "mesh.cpp")

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