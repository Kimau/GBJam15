#include "tetris.h"
#include <vector>

typedef std::vector<Pt> ListOfPt;
typedef std::vector<Rect> ListOfRect;

struct GameStateData {
  Rect brickArea;
  int width, height;
  uint8_t* sprites;
  int sprPitch;
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

  SDL_Surface* sprSurf = SDL_LoadBMP("sprites.bmp");
  if (sprSurf == 0) {
    SDL_Log(SDL_GetError());
    return;
  }

  SDL_Log("Bytes %d, %d", sprSurf->format->BitsPerPixel, sprSurf->format->format);

  ListOfPt corners;

  SDL_Rect pxSz = sprSurf->clip_rect;
  uint8_t *pixs = (uint8_t *)sprSurf->pixels;
  uint8_t framePix = pixs[0];

  // Fuck it Flip the mofo
  uint8_t *tempLine = new uint8_t[pxSz.w];
  
  for (int y = 0; y < pxSz.h; ++y) {
	  for (int x = 0; x < pxSz.w; ++x) {
		  if ((pixs[x + y * pxSz.w] == framePix) &&
			  (pixs[x + 1 + y * pxSz.w] == framePix) &&
			  (pixs[x + (y + 1) * pxSz.w] == framePix) &&
			  (pixs[(x + 1) + (y + 1) * pxSz.w] == 0)) {
        corners.push_back({x, y});
      }
    }
  }

  for (size_t i = 0; i < corners.size(); i++) {
    const auto &p = corners.at(i);

    // Build Sheet
    ListOfRect currSpr;

    // Get Width

    Rect currRect;
    currRect.x = p.x + 2;
    currRect.y = p.y + 2;
    currRect.w = 0;

	int yoffset = p.y * pxSz.w;
	for (int x = p.x + 2; x < pxSz.w; ++x) {
      if ((pixs[x + yoffset] == framePix) &&
		  (pixs[x + yoffset + pxSz.w] == 0)) {
        if (currRect.w == 0) currRect.x = x;
        ++currRect.w;
      } else if ((pixs[x + yoffset] == 0) &&
		  (pixs[x + yoffset + pxSz.w] == 0)) {
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

	int numSpr = 0;

    int xoffset = p.x;
	for (int y = p.y + 2; y < pxSz.h; ++y) {
		if ((pixs[xoffset + y * pxSz.w] == framePix) &&
			(pixs[xoffset + 1 + y * pxSz.w] == 0)) {
        if (currRect.h == 0) currRect.y = y;
        ++currRect.h;
		}
		else if ((pixs[xoffset + y * pxSz.w] == 0) &&
			(pixs[xoffset + 1 + y * pxSz.w] == 0)) {
        if (currRect.h > 0) {
          for (size_t subI = 0; subI < currSpr.size(); subI++) {
            auto r = currSpr.at(subI);
            r.y = currRect.y;
            r.h = currRect.h;
            gState.sprRect.push_back(r);

			++numSpr;
          }

          currRect.h = 0;
        }
      } else {
        break;
      }
    }

    // All Done
	SDL_Log("[%d:%d] = %d", p.x, p.y, numSpr);
  }

  /**/
  gState.sprPitch = sprSurf->w;
  gState.sprites = new uint8_t[sprSurf->w*sprSurf->h];
  memcpy(gState.sprites, sprSurf->pixels, sprSurf->w*sprSurf->h);
  SDL_FreeSurface(sprSurf);

  SDL_Log("Setup Done");
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

void FillRect(uint16_t *pixs, Rect *tarRect, uint16_t col) {
	for (int x = tarRect->x; x < (tarRect->w + tarRect->x); ++x)
		for (int y = tarRect->y; y < (tarRect->h + tarRect->y); ++y)
      SETPIX(x, y, col);
}

Pt Sprite(uint16_t *pixs, Pt *topLeft, size_t sprID) {
	const Rect& srcRect = gState.sprRect.at(sprID);

	for (int x = 0; x < srcRect.w; ++x) {
		for (int y = 0; y < srcRect.h; ++y) {
			switch (gState.sprites[(srcRect.x + x) + (srcRect.y + y) * gState.sprPitch]) {
			case 0:
				break;
			case 1:
				SETPIX(topLeft->x + x, topLeft->y + y, GBAColours[0]);
				break;
			case 2:
				SETPIX(topLeft->x + x, topLeft->y + y, GBAColours[1]);
				break;
			case 3:
				SETPIX(topLeft->x + x, topLeft->y + y, GBAColours[2]);
				break;
			case 4:
				SETPIX(topLeft->x + x, topLeft->y + y, GBAColours[3]);
				break;
			case 14:
				SETPIX(topLeft->x + x, topLeft->y + y, 0xF0F);
				break;
			default:
				SETPIX(topLeft->x + x, topLeft->y + y, 0);
				SDL_Log("X: %d", gState.sprites[(srcRect.x + x) + (srcRect.y + y) * gState.sprPitch]);
				break;
			}
			
		}
	}

	return Pt{ topLeft->x + srcRect.w, topLeft->y + srcRect.h };
}

void BorderRect(uint16_t *pixs, Rect *tarRect, uint16_t col, int inset,
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

void BezelBoxFilled(uint16_t *pixs, Rect *tarRect, uint16_t loCol,
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

SDL_Rect ShrinkGrow(Rect *srcRect, int amount) {
  SDL_Rect newRect;

  newRect.x = srcRect->x - amount;
  newRect.y = srcRect->y - amount;
  newRect.w = srcRect->w + amount;
  newRect.h = srcRect->h + amount;

  return newRect;
}

void Render(uint16_t *pixs, Rect *srcRect) {
  // Clear Board
  FillRect(pixs, srcRect, GBAColours[1]);

  if (gState.sprites == 0)
    pixs[0] = 0;
  else
    pixs[0] = 0xffff;
  // FillRect(pixs, &(gState.brickArea), GBAColours[0]);

  BezelBoxFilled(pixs, &(gState.brickArea), GBAColours[2], GBAColours[1],
                 GBAColours[0]);

  Pt cur = Pt{ 6, 6 };
  for (int i = 0; i < 10; ++i) {
	  cur = Sprite(pixs, &cur, i);
	  cur.y = 6;
	  cur.x += 1;
  }
    
  /*
  SETPIX(gState.brickArea.x, gState.brickArea.y, 0x0F00);
  SETPIX(gState.brickArea.x-1, gState.brickArea.y-1, 0x000F);
  BorderRect(pixs, &(gState.brickArea), GBAColours[2], -1, 1);
  */
}