#include "SDL.h"

extern "C" {
	void StartGame(uint16_t width, uint16_t height);

	void Tick();
	void Render(uint16_t* pixs, SDL_Rect* srcRect);
};