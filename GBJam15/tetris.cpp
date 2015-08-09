#include "tetris.h"

struct  
{
	SDL_Rect brickArea;
	uint16_t width, height;
	SDL_Surface* sprites;
} gState;

uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

#ifdef _DEBUG
#define SETPIX(__x,__y,__c) { SDL_assert(((__x)+(__y)*gState.width >= 0) && ((__x)<gState.width) && ((__y)<gState.height)); pixs[(__x)+(__y)*gState.width]=(__c); }
#else
#define SETPIX(__x,__y,__c) { pixs[(__x)+(__y)*gState.width]=(__c); }
#endif // _DEBUG

void StartGame(uint16_t width, uint16_t height) {
	gState.brickArea.x = 4;
	gState.brickArea.y = 4;
	gState.brickArea.w = width / 2;
	gState.brickArea.h = height - 8;

	gState.width = width;
	gState.height = height;

	SDL_Log(SDL_GetBasePath());
	gState.sprites = SDL_LoadBMP("sprites.bmp");

	if(gState.sprites != NULL) {
		Uint8 bytesPerPix = gState.sprites->format->BitsPerPixel;
		SDL_Log("Bytes %d", bytesPerPix);
	} else {
		SDL_Log(SDL_GetError());
	}
}

void Tick(){

}

void FillRect(uint16_t* pixs, SDL_Rect* tarRect, uint16_t col) {
	for (uint16_t x=tarRect->x; x < (tarRect->w + tarRect->x); ++x)
		for (uint16_t y=tarRect->y; y < (tarRect->h + tarRect->y); ++y)
			SETPIX(x,y,col);
}

void BorderRect(uint16_t* pixs, SDL_Rect* tarRect, uint16_t col, int inset, int outset) {
	int tlx, tly, brx,bry;
	tlx = tarRect->x;
	tly = tarRect->y;
	brx = tarRect->x + tarRect->w;
	bry = tarRect->y + tarRect->h;
	
	for (int x=(tlx - outset); x < (brx + outset); ++x) {
		for (int y=(tly -outset); y < (tly + inset); ++y)
			SETPIX(x,y,col);

		for (int y=(bry -inset); y < (bry + outset); ++y)
			SETPIX(x,y,col);
	}

	for (int y=(tly - outset); y < (bry + outset); ++y) {
		for (int x=(tlx -outset); x < (tlx + inset); ++x)
			SETPIX(x,y,col);

		for (int x=(brx -inset); x < (brx + outset); ++x)
			SETPIX(x,y,col);
	}
}

void BezelBoxFilled(uint16_t* pixs, SDL_Rect* tarRect, uint16_t loCol, uint16_t midCol, uint16_t hiCol) {
	FillRect(pixs, tarRect, midCol);

	int tlx, tly, brx,bry;
	tlx = tarRect->x;
	tly = tarRect->y;
	brx = tarRect->x + tarRect->w;
	bry = tarRect->y + tarRect->h;

	for(int x=tlx; x < brx; ++x) {
		SETPIX(x,tarRect->y,hiCol);
	}

	for(int y=tly; y < bry; ++y) {
		SETPIX(tarRect->x+tarRect->w,y,hiCol);
	}

	for(int x=tlx; x < brx; ++x) {
		SETPIX(x,tarRect->y+tarRect->h,loCol);
	}

	for(int y=tly; y < bry; ++y) {
		SETPIX(tarRect->x,y,loCol);
	}
}

SDL_Rect ShrinkGrow(SDL_Rect* srcRect, int amount) {
	SDL_Rect newRect;

	newRect.x = srcRect->x - amount;
	newRect.y = srcRect->y - amount;
	newRect.w = srcRect->w + amount;
	newRect.h = srcRect->h + amount;

	return newRect;
}

void Render(uint16_t* pixs, SDL_Rect* srcRect) {
	// Clear Board
	FillRect(pixs, srcRect, GBAColours[1]);

	if(gState.sprites == 0)
		pixs[0] = 0;
	else
		pixs[0] =0xffff;
	//FillRect(pixs, &(gState.brickArea), GBAColours[0]);

	BezelBoxFilled(pixs, &(gState.brickArea), GBAColours[2], GBAColours[1], GBAColours[0]);

	/*
	SETPIX(gState.brickArea.x, gState.brickArea.y, 0x0F00);
	SETPIX(gState.brickArea.x-1, gState.brickArea.y-1, 0x000F);
	BorderRect(pixs, &(gState.brickArea), GBAColours[2], -1, 1);
	*/
}