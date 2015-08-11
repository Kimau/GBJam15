#include "tetris.h"
#include <vector>
#include <algorithm>

typedef std::vector<Pt> ListOfPt;
typedef std::vector<Rect> ListOfRect;

////// CONSTS
uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};
int FLOOR_HEIGHT = 5;

///////// SPRITE SHEET DATA
int SPR_NUM[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
int SPR_MOUSE_IDLE[] = {10};
int SPR_MOUSE_RUN[] = {11, 12};
int SPR_CAT_IDLE[] = {28, 29, 30, 31, 32, 33, 34, 35};
int SPR_CAT_WALK[] = {36, 37, 38, 39};
int SPR_CAT_UP[] = {40};
int SPR_CAT_HOLD[] = {41};
int SPR_CAT_DOWN[] = {42};
int SPR_CAT_POUNCE[] = {44};
int SPR_DOG[] = {47, 48};
int SPR_FIGHT[] = {49, 50, 51};
int SPR_FOOD[] = {13, 14};
int SPR_SQUEEK[] = {27};
int SPR_WINDOW_CAT[] = {15, 17, 19, 21, 23, 25, 25, 25, 23, 21, 19, 17, 15};
int SPR_WINDOW_EMPTY[] = {16, 18, 20, 22, 24, 26, 26,
                          26, 24, 22, 22, 20, 18, 16};

//// STATE

struct CatData {
  enum { Idle, Left, Right, Up, Down, PounceLeft, PounceRight, Hold } state;
  Pt pos;
};

struct SpriteData {
  uint8_t *pixs;
  int sprPitch;
  ListOfRect sprRect;
};

struct BackgroundData {
  uint8_t *pixs;
  Rect size;
};

struct PixData {
	uint16_t *pixs;
	Rect size;
};


struct GameStateData {
  int screen_width, screen_height;
  CatData cat;
  SpriteData sprites;
  BackgroundData floor;
  Pt scrollPoint;
} gState;

/////// FUNCTIONS - RECT / PT

Pt operator+(const Pt& a, const Pt& b) { return Pt{ a.x + b.x, a.y + b.y }; }
Pt operator-(const Pt& a, const Pt& b) { return Pt{ a.x - b.x, a.y - b.y }; }
Rect operator+(const Rect& r, const Pt& p) { return Rect{ r.x + p.x, r.y + p.y, r.w, r.h }; }
Rect operator-(const Rect& r, const Pt& p) { return Rect{ r.x - p.x, r.y - p.y, r.w, r.h }; }
bool in(const Rect& r, const Pt& p) { return (p.x >= r.x) && (p.y >= r.y) && (p.x < (r.x+r.w)) && (p.y < (r.y+r.h)); }
Rect ShrinkGrow(const Rect& r, int s) {	return Rect{ r.x - s, r.y - s, r.w + s, r.h + s }; }

Rect operator&(const Rect& a, const Rect& b) { 
	return Rect{ 
	std::max(a.x, b.x), 
	std::max(a.y, b.y), 
	std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x), 
	std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y) }; 
}

Rect operator|(const Rect& a, const Rect& b) {
	return Rect{
		std::min(a.x, b.x),
		std::min(a.y, b.y),
		std::max(a.x + a.w, b.x + b.w) - std::min(a.x, b.x),
		std::max(a.y + a.h, b.y + b.h) - std::min(a.y, b.y) };
}

/////// MACRO

#ifdef _DEBUG
#define SETPIX(__scrn, __x, __y, __c)                                            \
  {                                                                      \
    SDL_assert(((__x) >= 0) && ((__y) >= 0) && ((__x) < (__scrn).size.w ) && \
               ((__y) < (__scrn).size.h));                                 \
    (__scrn).pixs[(__x) + (__y)*(__scrn).size.w] = (__c);                            \
  }
#else
#define SETPIX(__x, __y, __c) \
  { pixs[(__x) + (__y)*(__scrn).size.w] = (__c); }
#endif  // _DEBUG

void SetupBackground(BackgroundData &bgData, const char *filename) {
  SDL_Surface *bgSurf = SDL_LoadBMP(filename);
  if (bgSurf == 0) {
    SDL_Log(SDL_GetBasePath());
    SDL_Log(SDL_GetError());
    return;
  }

  /**/
  bgData.size = {0, 0, bgSurf->w, bgSurf->h};
  bgData.pixs = new uint8_t[bgSurf->w * bgSurf->h];
  memcpy(bgData.pixs, bgSurf->pixels, bgSurf->w * bgSurf->h);
  SDL_FreeSurface(bgSurf);
}

void SetupSprites(SpriteData &sprData, const char *filename) {
  SDL_Surface *sprSurf = SDL_LoadBMP(filename);
  if (sprSurf == 0) {
    SDL_Log(SDL_GetBasePath());
    SDL_Log(SDL_GetError());
    return;
  }

  SDL_Log("Bytes %d, %d", sprSurf->format->BitsPerPixel,
          sprSurf->format->format);

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
      } else if ((pixs[xoffset + y * pxSz.w] == 0) &&
                 (pixs[xoffset + 1 + y * pxSz.w] == 0)) {
        if (currRect.h > 0) {
          for (size_t subI = 0; subI < currSpr.size(); subI++) {
            auto r = currSpr.at(subI);
            r.y = currRect.y;
            r.h = currRect.h;
            sprData.sprRect.push_back(r);

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
  sprData.sprPitch = sprSurf->w;
  sprData.pixs = new uint8_t[sprSurf->w * sprSurf->h];
  memcpy(sprData.pixs, sprSurf->pixels, sprSurf->w * sprSurf->h);
  SDL_FreeSurface(sprSurf);

  SDL_Log("Setup Done");
}

void StartGame(uint16_t width, uint16_t height) {

  gState.screen_width = width;
  gState.screen_height = height;

  gState.scrollPoint = { 0, 0 };

  SetupSprites(gState.sprites, "sprites.bmp");
  SetupBackground(gState.floor, "floor.bmp");

  gState.cat.pos = {30, FLOOR_HEIGHT + 10};
  gState.cat.state = CatData::Idle;
  return;
}

static int ticker = 0;
void Tick(ButState *buttons) {
  if (buttons->left > 0)
    gState.cat.state = CatData::Left;
  else if (buttons->right > 0)
    gState.cat.state = CatData::Right;
  else
    gState.cat.state = CatData::Idle;

  switch (gState.cat.state) {
    case CatData::Idle:
      break;
    case CatData::Left:
		gState.cat.pos.x -= ((ticker % 3) == 0) ? 1:2;
      break;
    case CatData::Right:
		gState.cat.pos.x += ((ticker % 3) == 0) ? 1:2;
      break;
    case CatData::Up:
      break;
    case CatData::Down:
      break;
    case CatData::PounceLeft:
      break;
    case CatData::PounceRight:
      break;
    case CatData::Hold:
      break;
  }

  // Scroll Screen
  gState.scrollPoint.x += (ticker % 10 == 0) ? 1 : 0;

  // Clear Press
  buttons->up &= 1;
  buttons->down &= 1;
  buttons->left &= 1;
  buttons->right &= 1;

  ++ticker;
}

////////////////////////////////////////////////////////// RENDER FUNCTIONS
static int animCount = 0;

// UNSAFE
void RenderFillRect(PixData& scrn, Rect *tarRect, uint16_t col) {
	for (int x = tarRect->x; x < (tarRect->w + tarRect->x); ++x)
		for (int y = tarRect->y; y < (tarRect->h + tarRect->y); ++y)
			SETPIX(scrn, x, y, col);
}

Pt RenderSpriteHorFlip(PixData& scrn, const Pt &topLeft, 
	const SpriteData &sheet, size_t sprID) {
	const Rect &srcRect = sheet.sprRect.at(sprID);

	for (int x = 0; x < srcRect.w; ++x) {
		for (int y = 0; y < srcRect.h; ++y) {
			switch (sheet.pixs[srcRect.w + srcRect.x - x - 1 +
				(srcRect.y + y) * sheet.sprPitch]) {
			case 0:
				break;
			case 1:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[0]);
				break;
			case 2:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[1]);
				break;
			case 3:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[2]);
				break;
			case 4:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[3]);
				break;
			case 14:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF0F);
				break;
			default:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF00);
				break;
			}
		}
	}

	return Pt{ topLeft.x + srcRect.w, topLeft.y + srcRect.h };
}

Pt RenderSprite(PixData& scrn, const Pt &topLeft, const SpriteData &sheet,
	size_t sprID) {
	const Rect &srcRect = sheet.sprRect.at(sprID);

	for (int x = 0; x < srcRect.w; ++x) {
		for (int y = 0; y < srcRect.h; ++y) {
			switch (sheet.pixs[(srcRect.x + x) + (srcRect.y + y) * sheet.sprPitch]) {
			case 0:
				break;
			case 1:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[0]);
				break;
			case 2:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[1]);
				break;
			case 3:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[2]);
				break;
			case 4:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[3]);
				break;
			case 14:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF0F);
				break;
			default:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF00);
				break;
			}
		}
	}

	return Pt{ topLeft.x + srcRect.w, topLeft.y + srcRect.h };
}

void RenderBackground(PixData& scrn, Pt topLeft, const BackgroundData &bg) {
	Rect srcRect = bg.size;

	if (topLeft.x < 0) {
		srcRect.x += topLeft.x;
		srcRect.x = topLeft.x;
		topLeft.x = -topLeft.x;
	}

	if (topLeft.y < 0) {
		srcRect.y += topLeft.y;
		srcRect.y = topLeft.y;
		topLeft.y = -topLeft.y;
	}

	if ((topLeft.x + srcRect.w - srcRect.x) >= scrn.size.w) {
		srcRect.w -= (topLeft.x + srcRect.w - srcRect.x) - scrn.size.w;
	}

	if ((topLeft.y + srcRect.h - srcRect.y) >= scrn.size.h) {
		srcRect.h -= (topLeft.y + srcRect.h - srcRect.y) - scrn.size.h;
	}

	for (int x = srcRect.x; x < srcRect.w; ++x) {
		for (int y = srcRect.y; y < srcRect.h; ++y) {
			switch (bg.pixs[(srcRect.x + x) + (srcRect.y + y) * bg.size.w]) {
			case 0:
				break;
			case 1:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[0]);
				break;
			case 2:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[1]);
				break;
			case 3:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[2]);
				break;
			case 4:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, GBAColours[3]);
				break;
			case 14:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF0F);
				break;
			default:
				SETPIX(scrn, topLeft.x + x, topLeft.y + y, 0xF00);
				break;
			}
		}
	}
}

void RenderBorderRect(PixData& scrn, const Rect& tarRect, uint16_t col, int inset,
	int outset) {

	Rect topLeft = scrn.size & Rect{ tarRect.x - outset, tarRect.y - outset, outset + inset, outset + inset };
	Rect botRight = scrn.size & Rect{ tarRect.x + tarRect.w - inset - 1, tarRect.y + tarRect.h - inset - 1, outset + inset, outset + inset };
	
	for (int x = topLeft.x; x < (botRight.x + botRight.w); ++x) {
		// TOP
		for (int y = topLeft.y; y < (topLeft.y + topLeft.h); ++y) SETPIX(scrn, x, y, col);

		// BOTTOM
		for (int y = botRight.y; y < (botRight.y + botRight.h); ++y) SETPIX(scrn, x, y, col);
	}

	for (int y = topLeft.y; y < (botRight.y + botRight.h); ++y) {
		// LEFT
		for (int x = topLeft.x; x < (topLeft.x + topLeft.w); ++x) SETPIX(scrn, x, y, col);

		// RIGHT
		for (int x = botRight.x; x < (botRight.x + botRight.w); ++x) SETPIX(scrn, x, y, col);
	}
}

void RenderBezelBoxFilled(PixData &scrn, Rect *tarRect, uint16_t loCol,
                          uint16_t midCol, uint16_t hiCol) {
  Rect actualRect = scrn.size & *tarRect;
  if ((actualRect.w == 0) || (actualRect.h == 0))
	  return;

  RenderFillRect(scrn, &actualRect, midCol);

  int tlx, tly, brx, bry;
  tlx = actualRect.x;
  tly = actualRect.y;
  brx = actualRect.x + actualRect.w - 1;
  bry = actualRect.y + actualRect.h - 1;

  if ((tarRect->y >= tly) && (tarRect->y <= bry))
    for (int x = tlx; x < brx; ++x) {
      SETPIX(scrn, x, tarRect->y, hiCol);
    }

  if (((tarRect->x + tarRect->w) >= tlx) && ((tarRect->x + tarRect->w-1) <= brx))
    for (int y = tly; y < bry; ++y) {
      SETPIX(scrn, tarRect->x + tarRect->w-1, y, hiCol);
    }

  if (((tarRect->y + tarRect->h) >= tly) && ((tarRect->y + tarRect->h-1) <= bry))
    for (int x = tlx; x < brx; ++x) {
      SETPIX(scrn, x, tarRect->y + tarRect->h-1, loCol);
    }

  if ((tarRect->x >= tlx) && (tarRect->x <= brx))
    for (int y = tly; y < bry; ++y) {
      SETPIX(scrn, tarRect->x, y, loCol);
    }
}

void RenderCat(PixData& scrn, Rect *srcRect, CatData &cat) {
  int l = 1;

  Pt topLeft = Pt{ cat.pos.x, scrn.size.h - cat.pos.y };

  switch (cat.state) {
    case CatData::Idle:
      l = sizeof(SPR_CAT_IDLE) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_IDLE[animCount / 10 % l]);
      break;

    case CatData::Left:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
	  RenderSpriteHorFlip(scrn, topLeft, gState.sprites,
                    SPR_CAT_WALK[animCount / 7 % l]);
      break;

    case CatData::Right:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_WALK[animCount / 7 % l]);
      break;

    case CatData::Up:
      l = sizeof(SPR_CAT_UP) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_UP[animCount / 10 % l]);
      break;

    case CatData::Down:
      l = sizeof(SPR_CAT_DOWN) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_DOWN[animCount / 10 % l]);
      break;

    case CatData::PounceLeft:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_POUNCE[animCount / 10 % l]);
      break;

    case CatData::PounceRight:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
	  RenderSpriteHorFlip(scrn, topLeft, gState.sprites,
                    SPR_CAT_POUNCE[animCount / 10 % l]);
      break;

    case CatData::Hold:
      l = sizeof(SPR_CAT_HOLD) / sizeof(int);
	  RenderSprite(scrn, topLeft, gState.sprites, SPR_CAT_HOLD[animCount / 10 % l]);
      break;
  }
}

void Render(uint16_t *pixs, Rect *srcRect) {

	PixData screen = PixData{ pixs, *srcRect };
  // Clear Board
	RenderFillRect(screen, srcRect, GBAColours[1]);

	RenderBezelBoxFilled(screen, &(Rect{ 5, 5, srcRect->w / 2, srcRect->h - 10 } -gState.scrollPoint), GBAColours[2], GBAColours[1],
                 GBAColours[0]);

	RenderBackground(screen, Pt{ 0, srcRect->h - gState.floor.size.h }, gState.floor );

  Pt cur = Pt{6, 6};
  for (int i = 0; i < 10; ++i) {
	  cur = RenderSprite(screen, cur, gState.sprites, i);
    cur.y = 6;
    cur.x += 1;
  }

  int l = sizeof(SPR_WINDOW_CAT) / sizeof(int);
  RenderSprite(screen, Pt{ 6, 20 }, gState.sprites, SPR_WINDOW_CAT[animCount / 5 % l]);
  ++animCount;

  RenderCat(screen, srcRect, gState.cat);

  if (((animCount / 30) % 2) == 0) {
	  RenderBorderRect(screen, ShrinkGrow(screen.size, -1), 0xF00, 1, 0);
	  RenderBorderRect(screen, ShrinkGrow(screen.size, -1), 0x00F, 0, 1);
  }

  /*
  SETPIX(gState.brickArea.x, gState.brickArea.y, 0x0F00);
  SETPIX(gState.brickArea.x-1, gState.brickArea.y-1, 0x000F);
  BorderRect(pixs, &(gState.brickArea), GBAColours[2], -1, 1);
  */
}