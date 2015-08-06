#include "SDL.h"
#include <iostream>

#define SCREEN_TITLE "GBJam #15 - Kimau"
#define SCREEN_WIDTH  480 // 160
#define SCREEN_HEIGHT 432 // 144
#define GB_WIDTH 160
#define GB_HEIGHT 144

struct SDLAPP
{
	SDL_Window* m_window;
	SDL_Renderer* m_renderer;
};

SDLAPP* CreateApp() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return nullptr;
	}

	SDLAPP* pApp = new SDLAPP();
	pApp->m_window = SDL_CreateWindow(SCREEN_TITLE, 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (pApp->m_window == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		delete pApp;
		return nullptr;
	}

	pApp->m_renderer = SDL_CreateRenderer(pApp->m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (pApp->m_renderer == nullptr){
		SDL_DestroyWindow(pApp->m_window);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		delete pApp;
		return nullptr;
	}	

	return pApp;
}

void CleanQuit(SDLAPP* pApp) {
	SDL_DestroyRenderer(pApp->m_renderer);
	SDL_DestroyWindow(pApp->m_window);
	SDL_Quit();
}

// Colours
uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

void RenderScene(uint16_t* pixs, SDL_Texture* pBackBuffTex, SDL_Rect srcRect)
{
	for (int x = 0; x < GB_WIDTH; x++)
	{
		for (int y = 0; y < GB_HEIGHT; y++)
		{
			pixs[x+y*GB_WIDTH] = GBAColours[y%4];
		}

		pixs[x] = 0x0FFF;
		pixs[x+(GB_HEIGHT/2)*GB_WIDTH] = 0x0F00;
	}


	SDL_UpdateTexture(pBackBuffTex, &srcRect, pixs, GB_WIDTH*2);

}

int main( int argc, char* argv[] )
{
	SDLAPP* pApp = CreateApp();
	if(pApp == nullptr) { return -1; }

	SDL_Rect srcRect = { 0,0, GB_WIDTH, GB_HEIGHT};
	SDL_Rect tarRect = { 0,0, SCREEN_WIDTH, SCREEN_HEIGHT};
	uint16_t* pixs = new uint16_t[GB_WIDTH*GB_HEIGHT];
	SDL_Texture* pBackBuffTex = SDL_CreateTexture(pApp->m_renderer, SDL_PIXELFORMAT_RGB444, SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);

	RenderScene(pixs, pBackBuffTex, srcRect);

	do 
	{
		SDL_RenderClear(pApp->m_renderer);
		SDL_RenderCopy(pApp->m_renderer, pBackBuffTex, &srcRect, &tarRect);
		SDL_RenderPresent(pApp->m_renderer);
		SDL_Delay(1000);

	} while (pApp);
	

	CleanQuit(pApp);
	return 0;
}

