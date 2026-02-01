#include <stdexcept>
#include <span>
#include <iostream>

class Application
{
private:
	bool m_isRunning = true;
	
	SDL_Window* m_pWindow = nullptr;

	VkInstance m_pInstance = nullptr;

public:
	Application() {}
	~Application() 
	{
		cleanup();
	}

	void run()
	{
		initialize();

		while (m_isRunning)
		{
			update();
			render();
		}
	}

private:
	void initialize()
	{
		initWindow();
	}

	void initWindow()
	{
		SDL_Init(SDL_INIT_VIDEO);

		SDL_DisplayID primaryId = SDL_GetPrimaryDisplay();
		const char* pdName = SDL_GetDisplayName(primaryId);
		const auto* pdMode = SDL_GetCurrentDisplayMode(primaryId);

		std::cout << "Primary display detected: \n\tName: " << pdName
			<< "\n\tResolution: " << pdMode->w << 'x' << pdMode->h
			<< "\n\tRefresh Rate: " << pdMode->refresh_rate << "hz\n\n";

		int windowResW = pdMode->w / 2;
		int windowResH = pdMode->h / 2;

		std::cout << "Creating a " << windowResW << 'x' << windowResH << " window\n";
		m_pWindow = SDL_CreateWindow("SimpleVulkanRenderer", windowResW, windowResH, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_pWindow == nullptr)
			throw std::runtime_error("Failed to create window");
	}

	void initVkInstance()
	{
		volkInitialize();

		static constexpr VkApplicationInfo appInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "SimpleVulkanRenderer",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = nullptr,
			.engineVersion = 0,
			.apiVersion = VK_API_VERSION_1_3
		};

		const VkInstanceCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = 0,
			.ppEnabledExtensionNames = nullptr
		};

		if (vkCreateInstance(&createInfo, nullptr, &m_pInstance))
			throw std::runtime_error("Failed to create Vulkan Instance!");

		volkLoadInstance(m_pInstance);
	}

	void cleanup()
	{
		if (m_pInstance != nullptr)
			vkDestroyInstance(m_pInstance, nullptr);

		if (m_pWindow != nullptr)
			SDL_DestroyWindow(m_pWindow);

		SDL_Quit();
	}

	void update()
	{
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_EVENT_QUIT)
				m_isRunning = false;
		}
	}

	void render()
	{

	}
};

int main(int argc, char** argv)
{
	Application app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "Error: " << e.what() << "\n";
		return -1;
	}

	return 0;
}