set(SVR_IMGUI_BACKENDS_DIR "imgui/backends" CACHE STRING "" FORCE)

set(SVR_IMGUI_BACKENDS
	${SVR_IMGUI_BACKENDS_DIR}/imgui_impl_vulkan.cpp
	${SVR_IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.cpp
)

add_library(Imgui STATIC imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp ${SVR_IMGUI_BACKENDS})
target_link_libraries(Imgui PRIVATE volk::volk_headers SDL3::Headers)
target_compile_definitions(Imgui PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)
target_include_directories(Imgui PUBLIC
	imgui
)