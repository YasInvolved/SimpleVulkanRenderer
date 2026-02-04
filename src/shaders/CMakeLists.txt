message(STATUS "Spir-V compiler: ${Vulkan_dxc_EXECUTABLE}")

set(VERT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.vert.hlsl")
set(VERT_BIN "${CMAKE_BINARY_DIR}/shaders/main.vert.spv")

set(FRAG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.frag.hlsl")
set(FRAG_BIN "${CMAKE_BINARY_DIR}/shaders/main.frag.spv")

set(EMBED_ASM "${CMAKE_BINARY_DIR}/generated/embed.asm")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/embed.asm.in"
	"${EMBED_ASM}"
	@ONLY
)

add_library(SimpleVulkanRenderer_shaders OBJECT "${EMBED_ASM}")
target_include_directories(SimpleVulkanRenderer_shaders PUBLIC include)

add_custom_command(
	OUTPUT ${VERT_BIN}
	DEPENDS ${VERT_SRC}
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe> -spirv -fvk-use-dx-layout -T vs_6_0 -E VSMain "${VERT_SRC}" -Fo "${VERT_BIN}"
	COMMENT "Compiling vertex shader"
	VERBATIM
)

add_custom_command(
	OUTPUT ${FRAG_BIN}
	DEPENDS ${VERT_SRC}
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe> -spirv -fvk-use-dx-layout -T ps_6_0 -E PSMain "${FRAG_SRC}" -Fo "${FRAG_BIN}"
	COMMENT "Compiling fragment shader"
	VERBATIM
)

add_custom_target(SimpleVulkanRenderer_compileShaders DEPENDS ${VERT_BIN} ${FRAG_BIN})

add_dependencies(SimpleVulkanRenderer_shaders SimpleVulkanRenderer_compileShaders)