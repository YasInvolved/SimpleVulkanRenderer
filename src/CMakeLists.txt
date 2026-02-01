set(OUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_executable(SimpleVulkanRenderer main.cpp "pch.h")

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
	PRIVATE SDL3::SDL3 volk::volk fmt::fmt
)

target_precompile_headers(SimpleVulkanRenderer
	PRIVATE pch.h
)

target_compile_definitions(SimpleVulkanRenderer
	PRIVATE $<$<CONFIG:Debug>:SVR_DEBUG>
)