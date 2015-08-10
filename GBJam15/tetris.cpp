#include "tetris.h"
#include <vector>

class Rect {
public:
	int x, y, w, h;
};

class Pt {
public:
  int x, y;

  Pt(Pt&& p) = default;
};

typedef std::vector<Pt> ListOfPt;
typedef std::vector<Rect> ListOfRect;

struct GameStateData {
	SDL_Rect brickArea;
	uint16_t width, height;
	SDL_Surface *sprites;
	ListOfRect sprRect;
} gState;



uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

#ifdef _DEBUG
#define SETPIX(__x, __y, __c)                                                 \
  {                                                                           \
    SDL_assert(((__x) + (__y)*gState.width >= 0) && ((__x) < gState.width) && \
               ((__y) < gState.height));                                      \
    pixs[(__x) + (__y)*gState.width] = (__c);                                 \
  }
#else
#define SETPIX(__x, __y, __c) \
  { pixs[(__x) + (__y)*gState.width] = (__c); }
#endif  // _DEBUG

void SetupSprites() {
  SDL_Log(SDL_GetBasePath());
  gState.sprites = SDL_LoadBMP("sprites.bmp");
  if (gState.sprites == 0) {
    SDL_Log(SDL_GetError());
    return;
  }

  ListOfPt corners;

  uint16_t *pixs = (uint16_t *)gState.sprites->pixels;
  uint16_t framePix = pixs[0];
  for (int y = 0; y < gState.height; ++y) {
    for (int x = 0; x < gState.width; ++x) {
      if ((pixs[x + y * gState.width] == framePix) &&
          (pixs[x + 1 + y * gState.width] == framePix) &&
          (pixs[x + (y + 1) * gState.width] == framePix) &&
          (pixs[(x + 1) + (y + 1) * gState.width] == 0)) {
		  corners.push_back({ x, y });
      }
    }
  }
  
  for (const auto &p : corners) {

  }/*
    // Build Sheet
	ListOfRect currSpr;

    // Get Width
    
    Rect currRect;
    currRect.x = p->x + 2;
    currRect.y = p->y + 2;
	currRect.w = 0;

    int yoffset = p->y * gState.width;
    for (int x = p->x + 2; x < gState.sprites->clip_rect.w; ++x) {
      if ((pixs[x + yoffset] == framePix) &&
          (pixs[x + yoffset + gState.width] == 0)) {
		  if (currRect.w == 0)
			  currRect.x = x;
		  ++currRect.w;
      } else if ((pixs[x + yoffset] == 0) &&
                 (pixs[x + yoffset + gState.width] == 0)) {
		  if (currRect.w > 0) {
		  currSpr.push_back(currRect);
		  currRect.w = 0;
        }
      } else {
        break;
      }
    }

    // Get Height
	currRect.h = 0;

    int xoffset = p->x;
    for (int y = p->y + 2; y < gState.sprites->clip_rect.h; ++y) {
      if ((pixs[xoffset + y * gState.width] == framePix) &&
          (pixs[xoffset + 1 + y * gState.width] == 0)) {
		  if (currRect.h == 0)
			  currRect.y = y;
		  ++currRect.h;
      } else if ((pixs[xoffset + y * gState.width] == 0) &&
                 (pixs[xoffset + 1 + y * gState.width] == 0)) {
		  if (currRect.h > 0) {
			for (auto r = currSpr.begin(); r != currSpr.end(); ++r)
			{
				r->y = currRect.y;
				r->h = currRect.h;

				gState.sprRect.push_back(*r);
			}
			currRect.h = 0;          
        }
      } else {
        break;
      }
    }

	// All Done
	SDL_Log("[%d:%d] spr count now %d", p->x, p->y, gState.sprRect.size());
  }
  /**/

  Uint8 bytesPerPix = gState.sprites->format->BitsPerPixel;
  SDL_Log("Bytes %d", bytesPerPix);
}

void StartGame(uint16_t width, uint16_t height) {
  gState.brickArea.x = 4;
  gState.brickArea.y = 4;
  gState.brickArea.w = width / 2;
  gState.brickArea.h = height - 8;

  gState.width = width;
  gState.height = height;

  SetupSprites();
  return;
}

void Tick() {}

void FillRect(uint16_t *pixs, SDL_Rect *tarRect, uint16_t col) {
  for (uint16_t x = tarRect->x; x < (tarRect->w + tarRect->x); ++x)
    for (uint16_t y = tarRect->y; y < (tarRect->h + tarRect->y); ++y)
      SETPIX(x, y, col);
}

void BorderRect(uint16_t *pixs, SDL_Rect *tarRect, uint16_t col, int inset,
                int outset) {
  int tlx, tly, brx, bry;
  tlx = tarRect->x;
  tly = tarRect->y;
  brx = tarRect->x + tarRect->w;
  bry = tarRect->y + tarRect->h;

  for (int x = (tlx - outset); x < (brx + outset); ++x) {
    for (int y = (tly - outset); y < (tly + inset); ++y) SETPIX(x, y, col);

    for (int y = (bry - inset); y < (bry + outset); ++y) SETPIX(x, y, col);
  }

  for (int y = (tly - outset); y < (bry + outset); ++y) {
    for (int x = (tlx - outset); x < (tlx + inset); ++x) SETPIX(x, y, col);

    for (int x = (brx - inset); x < (brx + outset); ++x) SETPIX(x, y, col);
  }
}

void BezelBoxFilled(uint16_t *pixs, SDL_Rect *tarRect, uint16_t loCol,
                    uint16_t midCol, uint16_t hiCol) {
  FillRect(pixs, tarRect, midCol);

  int tlx, tly, brx, bry;
  tlx = tarRect->x;
  tly = tarRect->y;
  brx = tarRect->x + tarRect->w;
  bry = tarRect->y + tarRect->h;

  for (int x = tlx; x < brx; ++x) {
    SETPIX(x, tarRect->y, hiCol);
  }

  for (int y = tly; y < bry; ++y) {
    SETPIX(tarRect->x + tarRect->w, y, hiCol);
  }

  for (int x = tlx; x < brx; ++x) {
    SETPIX(x, tarRect->y + tarRect->h, loCol);
  }

  for (int y = tly; y < bry; ++y) {
    SETPIX(tarRect->x, y, loCol);
  }
}

SDL_Rect ShrinkGrow(SDL_Rect *srcRect, int amount) {
  SDL_Rect newRect;

  newRect.x = srcRect->x - amount;
  newRect.y = srcRect->y - amount;
  newRect.w = srcRect->w + amount;
  newRect.h = srcRect->h + amount;

  return newRect;
}

void Render(uint16_t *pixs, SDL_Rect *srcRect) {
  // Clear Board
  FillRect(pixs, srcRect, GBAColours[1]);

  if (gState.sprites == 0)
    pixs[0] = 0;
  else
    pixs[0] = 0xffff;
  // FillRect(pixs, &(gState.brickArea), GBAColours[0]);

  BezelBoxFilled(pixs, &(gState.brickArea), GBAColours[2], GBAColours[1],
                 GBAColours[0]);

  /*
  SETPIX(gState.brickArea.x, gState.brickArea.y, 0x0F00);
  SETPIX(gState.brickArea.x-1, gState.brickArea.y-1, 0x000F);
  BorderRect(pixs, &(gState.brickArea), GBAColours[2], -1, 1);
  */
}