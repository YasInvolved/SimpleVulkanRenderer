message(STATUS "Spir-V compiler: ${Vulkan_dxc_EXECUTABLE}")

set(VERT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.vert.hlsl")
set(VERT_BIN "${CMAKE_BINARY_DIR}/shaders/main.vert.spv")

set(FRAG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.frag.hlsl")
set(FRAG_BIN "${CMAKE_BINARY_DIR}/shaders/main.frag.spv")

set_source_files_properties("${VERT_SRC}" PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties("${VERT_SRC}" PROPERTIES HEADER_FILE_ONLY "None")
set_source_files_properties("${FRAG_SRC}" PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties("${FRAG_SRC}" PROPERTIES HEADER_FILE_ONLY "None")

set(EMBED_ASM "${CMAKE_BINARY_DIR}/generated/embed.asm")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/embed.asm.in"
	"${EMBED_ASM}"
	@ONLY
)

add_custom_command(
	OUTPUT ${VERT_BIN}
	DEPENDS ${VERT_SRC}
	COMMENT "Compiling vertex shader"
	VERBATIM
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe>
	"-spirv"
	"-fvk-use-dx-layout"
	"-T" "vs_6_1"
	"-E" "VSMain"
	"-Fo" "${VERT_BIN}"
	"${VERT_SRC}"
)

add_custom_command(
	OUTPUT ${FRAG_BIN}
	DEPENDS ${FRAG_SRC}
	COMMENT "Compiling fragment shader"
	VERBATIM
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe>
		"-spirv"
		"-fvk-use-dx-layout"
		"-T" "ps_6_1"
		"-E" "PSMain"
		"-Fo" "${FRAG_BIN}"
		"${FRAG_SRC}"
)

add_custom_target(shaders_target DEPENDS "${VERT_BIN}" "${FRAG_BIN}")

add_library(SimpleVulkanRenderer_shaders OBJECT "${EMBED_ASM}" "${VERT_SRC}" "${FRAG_SRC}")
set_target_properties(SimpleVulkanRenderer_shaders PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shaders"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shaders"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shaders"
)
target_include_directories(SimpleVulkanRenderer_shaders PUBLIC cpp_include)
add_dependencies(SimpleVulkanRenderer_shaders shaders_target)
