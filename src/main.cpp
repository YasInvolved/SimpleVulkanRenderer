#include <stdexcept>
#include <span>
#include <iostream>

class Application
{
private:
	bool m_isRunning = true;
	
	SDL_Window* m_window = nullptr;

public:
	Application() {}
	~Application() {}

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
		m_window = SDL_CreateWindow("SimpleVulkanRenderer", windowResW, windowResH, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
			throw std::runtime_error("Failed to create window");
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