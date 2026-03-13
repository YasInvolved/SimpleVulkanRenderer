#include "Window.h"

using namespace svr;

std::shared_ptr<Window> Window::s_instance;

Window::Window()
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_DisplayID primaryId = SDL_GetPrimaryDisplay();
	const auto* displayMode = SDL_GetCurrentDisplayMode(primaryId);

	static constexpr auto INIT_FLAGS =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY |
		SDL_WINDOW_RESIZABLE;

	m_ptr = SDL_CreateWindow("SimpleVulkanRenderer", displayMode->w * 0.75f, displayMode->h * 0.75f, INIT_FLAGS);
	if (!m_ptr)
		throw std::runtime_error("Failed to create a window");
}

Window::window_ptr Window::Get()
{
	if (!s_instance)
		s_instance = window_ptr(new Window());

	return s_instance;
}

VkSurfaceKHR Window::createSurface(VkInstance instance) const
{
	assert(m_ptr != nullptr);

	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(m_ptr, instance, nullptr, &surface))
		throw std::runtime_error("Failed to create a surface");

	return surface;
}

void Window::handleEvents()
{
	assert(m_ptr != nullptr);

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL3_ProcessEvent(&event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			m_shouldClose = true;
			break;
		case SDL_EVENT_WINDOW_MINIMIZED:
			m_minimized = true;
			break;
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			m_resized = true;
			break;
		case SDL_EVENT_WINDOW_RESTORED:
			m_minimized = false;
			break;
		}
	}
}
