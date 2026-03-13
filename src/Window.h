#pragma once

namespace svr
{
	class Window
	{
	public:
		using window_ptr = std::shared_ptr<Window>;
		using keyboard_input_handler_t = std::function<void(bool, bool, SDL_Scancode)>;

	private:
		// instance
		static window_ptr s_instance;

		Window();

		SDL_Window* m_ptr = nullptr;

		bool m_shouldClose = false;
		bool m_resized = false;
		bool m_minimized = false;

		keyboard_input_handler_t m_inputHandler;

	public:
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		static window_ptr Get();

		inline bool shouldClose() const { return m_shouldClose; }
		inline bool isResized() const { return m_resized; }
		inline bool isMinimized() const { return m_minimized; }

		inline void setResized(bool value) { m_resized = value; }
		inline void setKeyboardInputHandler(const keyboard_input_handler_t& value) { m_inputHandler = value; }

		VkSurfaceKHR createSurface(VkInstance instance) const;
		inline bool initImgui() const { return ImGui_ImplSDL3_InitForVulkan(m_ptr); }

		void handleEvents();
	};
}
