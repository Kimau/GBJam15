#include "tetris.h"

struct  
{
	SDL_Rect brickArea;
	uint16_t width, height;
} gState;

uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

void StartGame(uint16_t width, uint16_t height) {
	gState.brickArea.x = 4;
	gState.brickArea.y = 4;
	gState.brickArea.w = width / 2;
	gState.brickArea.h = height - 8;

	gState.width = width;
	gState.height = height;
}

void Tick(){

}

void FillRect(uint16_t* pixs, SDL_Rect* tarRect, uint16_t col) {
	for (uint16_t x=tarRect->x; x < (tarRect->w + tarRect->x); ++x)
		for (uint16_t y=tarRect->y; y < (tarRect->h + tarRect->y); ++y)
			pixs[x+y*gState.width] = col;
}

void BorderRect(uint16_t* pixs, SDL_Rect* tarRect, uint16_t col, int inset, int outset) {
	for (uint16_t x=(tarRect->x - outset); x < (tarRect->w + tarRect->x + outset); ++x) {
		for (uint16_t y=(tarRect->y-outset); y < (tarRect->y + inset); ++y)
			pixs[x+y*gState.width] = col;

		for (uint16_t y=(tarRect->y+tarRect->h-tarRect->y-inset); y < (tarRect->y + tarRect->h + tarRect->y + outset); ++y)
			pixs[x+y*gState.width] = col;
	}

	for (uint16_t y=(tarRect->y + inset); y < (tarRect->y+tarRect->h + tarRect->y-inset); ++y) {
		for (uint16_t x=(tarRect->x - outset); x < (tarRect->x + inset); ++x)
			pixs[x+y*gState.width] = col;

		for (uint16_t x=(tarRect->x + tarRect->h - inset); x < (tarRect->x + tarRect->w +tarRect->x+ outset); ++x)
			pixs[x+y*gState.width] = col;
	}
}

void Render(uint16_t* pixs, SDL_Rect* srcRect) {
	// Clear Board
	FillRect(pixs, srcRect, GBAColours[1]);
	FillRect(pixs, &(gState.brickArea), GBAColours[0]);
}