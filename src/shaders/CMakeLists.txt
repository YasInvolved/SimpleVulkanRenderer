message(STATUS "Spir-V compiler: ${Vulkan_dxc_EXECUTABLE}")

set(VERT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.vert.hlsl" CACHE STRING "" FORCE)
set(VERT_BIN "${CMAKE_BINARY_DIR}/shaders/main.vert.spv" CACHE STRING "" FORCE)

set(FRAG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.frag.hlsl" CACHE STRING "" FORCE)
set(FRAG_BIN "${CMAKE_BINARY_DIR}/shaders/main.frag.spv" CACHE STRING "" FORCE)

set(EMBED_ASM "${CMAKE_BINARY_DIR}/generated/embed.asm" CACHE STRING "" FORCE)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/embed.asm.in"
	"${EMBED_ASM}"
	@ONLY
)

add_library(SimpleVulkanRenderer_shaders OBJECT "${EMBED_ASM}")
target_include_directories(SimpleVulkanRenderer_shaders PUBLIC include)

add_custom_target(SimpleVulkanRenderer_compileShaders
	DEPENDS ${VERT_SRC} ${FRAG_SRC}
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe> -spirv -T vs_6_0 -E VSMain ${VERT_SRC} -Fo ${VERT_BIN}
	COMMAND $<TARGET_FILE:Vulkan::dxc_exe> -spirv -T ps_6_0 -E PSMain ${FRAG_SRC} -Fo ${FRAG_BIN}
	COMMENT "Compiling and embedding shaders"
	VERBATIM
)

add_dependencies(SimpleVulkanRenderer_shaders SimpleVulkanRenderer_compileShaders)