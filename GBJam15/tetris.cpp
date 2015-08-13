#include "tetris.h"
#include <vector>
#include <algorithm>

typedef std::vector<Pt> ListOfPt;
typedef std::vector<Rect> ListOfRect;

////// CONSTS
uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};
int FLOOR_HEIGHT = 5;

///////// SPRITE SHEET DATA
int SPR_COUNT = 0;
int SPR_NUM[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
int SPR_MOUSE_IDLE[] = {0};
int SPR_MOUSE_RUN[] = {0, 1};
int SPR_FOOD[] = {0, 1};
int SPR_WINDOW_CAT[] = {0, 2, 4, 6, 8, 10, 10, 10, 8, 6, 4, 2, 0};
int SPR_WINDOW_EMPTY[] = {1, 3, 5, 7, 9, 11, 11, 11, 9, 7, 5, 3, 1};
int SPR_SQUEEK[] = {0};
int SPR_BIN_TOP[] = {0};
int SPR_CAT_IDLE[] = {0, 1, 2, 3, 4, 5, 6, 7};
int SPR_CAT_WALK[] = {0, 1, 2, 3};
int SPR_CAT_UP[] = {0};
int SPR_CAT_HOLD[] = {0};
int SPR_CAT_DOWN[] = {0};
int SPR_CAT_1[] = {0};
int SPR_CAT_POUNCE[] = {0};
int SPR_CAT_2[] = {0, 1};
int SPR_DOG[] = {0, 1};
int SPR_FIGHT[] = {0, 1, 2};

//// STATE

struct CatData {
  enum { Idle, Left, Right, Up, Down, PounceLeft, PounceRight, Hold } state;
  Pt pos;
};

struct SpriteData {
  uint8_t* pixs;
  int sprPitch;
  ListOfRect sprRect;
};

struct BackgroundData {
  uint8_t* pixs;
  Rect size;
};

struct PixData {
  uint16_t* pixs;
  Rect size;

  int GetI(const Pt& p) const;
};

struct GameStateData {
  int screen_width, screen_height;
  CatData cat;
  SpriteData sprites;
  BackgroundData floor;
  Pt scrollPoint;
} gState;

/////// FUNCTIONS - RECT / PT

Pt operator+(const Pt& a, const Pt& b) { return Pt{a.x + b.x, a.y + b.y}; }
Pt operator-(const Pt& a, const Pt& b) { return Pt{a.x - b.x, a.y - b.y}; }
Rect operator+(const Rect& r, const Pt& p) {
  return Rect{r.x + p.x, r.y + p.y, r.w, r.h};
}
Rect operator-(const Rect& r, const Pt& p) {
  return Rect{r.x - p.x, r.y - p.y, r.w, r.h};
}
bool in(const Rect& r, const Pt& p) {
  return (p.x >= r.x) && (p.y >= r.y) && (p.x < (r.x + r.w)) &&
         (p.y < (r.y + r.h));
}
Rect ShrinkGrow(const Rect& r, int s) {
  return Rect{r.x - s, r.y - s, r.w + s, r.h + s};
}

Rect operator&(const Rect& a, const Rect& b) {
  return Rect{std::max(a.x, b.x), std::max(a.y, b.y),
              std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x),
              std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y)};
}

Rect operator|(const Rect& a, const Rect& b) {
  return Rect{std::min(a.x, b.x), std::min(a.y, b.y),
              std::max(a.x + a.w, b.x + b.w) - std::min(a.x, b.x),
              std::max(a.y + a.h, b.y + b.h) - std::min(a.y, b.y)};
}

int PixData::GetI(const Pt& p) const {
  SDL_assert(in(size, p));
  return (p.x - size.x) + (p.y - size.y) * size.w;
}

/////// MACRO

void SetupBackground(BackgroundData& bgData, const char* filename) {
  SDL_Surface* bgSurf = SDL_LoadBMP(filename);
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

void SetupSprites(SpriteData& sprData, const char* filename) {
  SDL_Surface* sprSurf = SDL_LoadBMP(filename);
  if (sprSurf == 0) {
    SDL_Log(SDL_GetBasePath());
    SDL_Log(SDL_GetError());
    return;
  }

  SDL_Log("Bytes %d, %d", sprSurf->format->BitsPerPixel,
          sprSurf->format->format);

  ListOfPt corners;
  int numSpritesTotal = 0;

  SDL_Rect pxSz = sprSurf->clip_rect;
  uint8_t* pixs = (uint8_t*)sprSurf->pixels;
  uint8_t framePix = pixs[0];

  // Fuck it Flip the mofo
  uint8_t* tempLine = new uint8_t[pxSz.w];

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
    const auto& p = corners.at(i);

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
            ++numSpritesTotal;
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

  SDL_Log("Setup Done found %d sprites in %s", numSpritesTotal, filename);
}

int SetupSpriteArray(int* arr, size_t numElem) {
  int maxNum = 0;
  int c = 0;
  for (size_t i = 0; i < numElem; i++) {
    c = arr[i];
    if (maxNum < c) maxNum = c;
    arr[i] = c + SPR_COUNT;
  }

  return maxNum + 1;
}

void SetupMySheet() {
  SPR_COUNT += SetupSpriteArray(SPR_NUM, sizeof(SPR_NUM) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_MOUSE_IDLE, sizeof(SPR_MOUSE_IDLE) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_MOUSE_RUN, sizeof(SPR_MOUSE_RUN) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_FOOD, sizeof(SPR_FOOD) / sizeof(int));

  SetupSpriteArray(SPR_WINDOW_CAT, sizeof(SPR_WINDOW_CAT) / sizeof(int));
  SetupSpriteArray(SPR_WINDOW_EMPTY, sizeof(SPR_WINDOW_EMPTY) / sizeof(int));
  SPR_COUNT += 12;

  SPR_COUNT += SetupSpriteArray(SPR_SQUEEK, sizeof(SPR_SQUEEK) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_BIN_TOP, sizeof(SPR_BIN_TOP) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_CAT_IDLE, sizeof(SPR_CAT_IDLE) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_CAT_WALK, sizeof(SPR_CAT_WALK) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_CAT_UP, sizeof(SPR_CAT_UP) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_CAT_HOLD, sizeof(SPR_CAT_HOLD) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_CAT_DOWN, sizeof(SPR_CAT_DOWN) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_CAT_1, sizeof(SPR_CAT_1) / sizeof(int));
  SPR_COUNT +=
      SetupSpriteArray(SPR_CAT_POUNCE, sizeof(SPR_CAT_POUNCE) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_CAT_2, sizeof(SPR_CAT_2) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_DOG, sizeof(SPR_DOG) / sizeof(int));
  SPR_COUNT += SetupSpriteArray(SPR_FIGHT, sizeof(SPR_FIGHT) / sizeof(int));

  SDL_Log("Setup %d sprites", SPR_COUNT);
}

void StartGame(uint16_t width, uint16_t height) {
  gState.screen_width = width;
  gState.screen_height = height;

  gState.scrollPoint = {0, 0};

  SetupSprites(gState.sprites, "sprites.bmp");
  SetupBackground(gState.floor, "floor.bmp");
  SetupMySheet();

  gState.cat.pos = {30, FLOOR_HEIGHT + 10};
  gState.cat.state = CatData::Idle;
  return;
}

static int ticker = 0;
void Tick(ButState* buttons) {
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
      gState.cat.pos.x -= ((ticker % 3) == 0) ? 1 : 2;
      break;
    case CatData::Right:
      gState.cat.pos.x += ((ticker % 3) == 0) ? 1 : 2;
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

void RenderFillRect(PixData& scrn, const Rect& origTarRect, uint16_t col) {
  Rect tarRect = scrn.size & origTarRect;

  int c = scrn.GetI(Pt{tarRect.x, tarRect.y});
  for (int y = 0; y < tarRect.h; ++y) {
    for (int x = 0; x < tarRect.w; ++x) scrn.pixs[c++] = col;
    c = c - tarRect.w + scrn.size.w;
  }
}

Rect RenderSpriteHorFlip(PixData& scrn, const Pt& topLeft,
                         const SpriteData& sheet, size_t sprID) {
  Rect sprRect = sheet.sprRect.at(sprID);
  Rect srcRect = sprRect;
  Rect tarRect = {topLeft.x, topLeft.y, srcRect.w, srcRect.h};
  tarRect = tarRect & scrn.size;

  if ((tarRect.w <= 0) || (tarRect.h <= 0)) return sprRect;

  srcRect.x = srcRect.x + (srcRect.w - 1 - (tarRect.x - topLeft.x));
  srcRect.y += tarRect.y - topLeft.y;
  srcRect.w = tarRect.w;
  srcRect.h = tarRect.h;

  int c = scrn.GetI(Pt{tarRect.x, tarRect.y});
  for (int y = 0; y < srcRect.h; ++y) {
    for (int x = 0; x < srcRect.w; ++x) {
      switch (sheet.pixs[(srcRect.x - x) + (srcRect.y + y) * sheet.sprPitch]) {
        case 0:
          break;
        case 1:
          scrn.pixs[c] = GBAColours[0];
          break;
        case 2:
          scrn.pixs[c] = GBAColours[1];
          break;
        case 3:
          scrn.pixs[c] = GBAColours[2];
          break;
        case 4:
          scrn.pixs[c] = GBAColours[3];
          break;
        case 14:
          scrn.pixs[c] = 0xF0F;
          break;
        default:
          scrn.pixs[c] = 0xF00;
          break;
      }
      ++c;
    }
    c = c - srcRect.w + scrn.size.w;
  }

  return sprRect;
}

Rect RenderSprite(PixData& scrn, const Pt& topLeft, const SpriteData& sheet,
                  size_t sprID) {
  Rect sprRect = sheet.sprRect.at(sprID);
  Rect srcRect = sprRect;
  Rect tarRect = {topLeft.x, topLeft.y, srcRect.w, srcRect.h};
  tarRect = tarRect & scrn.size;

  if ((tarRect.w <= 0) || (tarRect.h <= 0)) return sprRect;

  srcRect.x += tarRect.x - topLeft.x;
  srcRect.y += tarRect.y - topLeft.y;
  srcRect.w = tarRect.w;
  srcRect.h = tarRect.h;

  int c = scrn.GetI(Pt{tarRect.x, tarRect.y});
  for (int y = 0; y < srcRect.h; ++y) {
    for (int x = 0; x < srcRect.w; ++x) {
      switch (sheet.pixs[(srcRect.x + x) + (srcRect.y + y) * sheet.sprPitch]) {
        case 0:
          break;
        case 1:
          scrn.pixs[c] = GBAColours[0];
          break;
        case 2:
          scrn.pixs[c] = GBAColours[1];
          break;
        case 3:
          scrn.pixs[c] = GBAColours[2];
          break;
        case 4:
          scrn.pixs[c] = GBAColours[3];
          break;
        case 14:
          scrn.pixs[c] = 0xF0F;
          break;
        default:
          scrn.pixs[c] = 0xF00;
          break;
      }
      ++c;
    }
    c = c - srcRect.w + scrn.size.w;
  }

  return sprRect;
}

void RenderBackground(PixData& scrn, Pt topLeft, const BackgroundData& bg) {
  Rect srcRect = bg.size;
  Rect tarRect = {topLeft.x, topLeft.y, srcRect.w, srcRect.h};
  tarRect = tarRect & scrn.size;

  if ((tarRect.w < 0) || (tarRect.h < 0)) return;

  srcRect.x += tarRect.x - topLeft.x;
  srcRect.y += tarRect.y - topLeft.y;
  srcRect.w = tarRect.w;
  srcRect.h = tarRect.h;

  int c = scrn.GetI(Pt{tarRect.x, tarRect.y});
  for (int y = 0; y < srcRect.h; ++y) {
    for (int x = 0; x < srcRect.w; ++x) {
      switch (bg.pixs[(srcRect.x + x) + (srcRect.y + y) * bg.size.w]) {
        case 0:
          break;
        case 1:
          scrn.pixs[c] = GBAColours[0];
          break;
        case 2:
          scrn.pixs[c] = GBAColours[1];
          break;
        case 3:
          scrn.pixs[c] = GBAColours[2];
          break;
        case 4:
          scrn.pixs[c] = GBAColours[3];
          break;
        case 14:
          scrn.pixs[c] = 0xF0F;
          break;
        default:
          scrn.pixs[c] = 0xF00;
          break;
      }
      ++c;
    }
    c = c - srcRect.w + scrn.size.w;
  }

  return;
}

void RenderBorderRect(PixData& scrn, const Rect& tarRect, uint16_t col,
                      int inset, int outset) {
  int c = 0;
  Rect topLeft = scrn.size & Rect{tarRect.x - outset, tarRect.y - outset,
                                  outset + inset, outset + inset};
  Rect botRight = scrn.size & Rect{tarRect.x + tarRect.w - inset - 1,
                                   tarRect.y + tarRect.h - inset - 1,
                                   outset + inset, outset + inset};

  // TOP
  for (int y = topLeft.y; y < (topLeft.y + topLeft.h); ++y) {
    c = scrn.GetI(Pt{topLeft.x, y});
    for (int x = topLeft.x; x < (botRight.x + botRight.w); ++x)
      scrn.pixs[c++] = col;
  }

  // BOTTOM
  for (int y = botRight.y; y < (botRight.y + botRight.h); ++y) {
    c = scrn.GetI(Pt{topLeft.x, y});
    for (int x = topLeft.x; x < (botRight.x + botRight.w); ++x)
      scrn.pixs[c++] = col;
  }

  for (int y = topLeft.y; y < (botRight.y + botRight.h); ++y) {
    // LEFT
    c = scrn.GetI(Pt{topLeft.x, y});
    for (int x = topLeft.x; x < (topLeft.x + topLeft.w); ++x)
      scrn.pixs[c++] = col;

    // RIGHT
    c = scrn.GetI(Pt{botRight.x, y});
    for (int x = botRight.x; x < (botRight.x + botRight.w); ++x)
      scrn.pixs[c++] = col;
  }
}

void RenderBezelBoxFilled(PixData& scrn, const Rect& tarRect, uint16_t loCol,
                          uint16_t midCol, uint16_t hiCol) {
  Rect actualRect = scrn.size & tarRect;
  if ((actualRect.w == 0) || (actualRect.h == 0)) return;

  RenderFillRect(scrn, actualRect, midCol);

  int c;
  int tlx, tly, brx, bry;
  tlx = actualRect.x;
  tly = actualRect.y;
  brx = actualRect.x + actualRect.w - 1;
  bry = actualRect.y + actualRect.h - 1;

  if ((tarRect.y >= tly) && (tarRect.y <= bry)) {
    c = scrn.GetI(Pt{tlx, tarRect.y});
    for (int x = tlx; x < brx; ++x) {
      scrn.pixs[c++] = hiCol;
    }
  }

  if (((tarRect.x + tarRect.w) >= tlx) &&
      ((tarRect.x + tarRect.w - 1) <= brx)) {
    c = scrn.GetI(Pt{tarRect.x + tarRect.w - 1, tly});
    for (int y = tly; y < bry; ++y) {
      scrn.pixs[c] = hiCol;
      c += scrn.size.w;
    }
  }

  if (((tarRect.y + tarRect.h) >= tly) &&
      ((tarRect.y + tarRect.h - 1) <= bry)) {
    c = scrn.GetI(Pt{tlx, tarRect.y + tarRect.h - 1});
    for (int x = tlx; x < brx; ++x) {
      scrn.pixs[c++] = loCol;
    }
  }

  if ((tarRect.x >= tlx) && (tarRect.x <= brx)) {
    c = scrn.GetI(Pt{tarRect.x, tly});
    for (int y = tly; y < bry; ++y) {
      scrn.pixs[c] = loCol;
      c += scrn.size.w;
    }
  }
}

void RenderCat(PixData& scrn, Rect* srcRect, CatData& cat) {
  int l = 1;

  Pt topLeft = Pt{cat.pos.x, scrn.size.h - cat.pos.y};

  switch (cat.state) {
    case CatData::Idle:
      l = sizeof(SPR_CAT_IDLE) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_IDLE[animCount / 10 % l]);
      break;

    case CatData::Left:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
      RenderSpriteHorFlip(scrn, topLeft, gState.sprites,
                          SPR_CAT_WALK[animCount / 7 % l]);
      break;

    case CatData::Right:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_WALK[animCount / 7 % l]);
      break;

    case CatData::Up:
      l = sizeof(SPR_CAT_UP) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_UP[animCount / 10 % l]);
      break;

    case CatData::Down:
      l = sizeof(SPR_CAT_DOWN) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_DOWN[animCount / 10 % l]);
      break;

    case CatData::PounceLeft:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_POUNCE[animCount / 10 % l]);
      break;

    case CatData::PounceRight:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
      RenderSpriteHorFlip(scrn, topLeft, gState.sprites,
                          SPR_CAT_POUNCE[animCount / 10 % l]);
      break;

    case CatData::Hold:
      l = sizeof(SPR_CAT_HOLD) / sizeof(int);
      RenderSprite(scrn, topLeft, gState.sprites,
                   SPR_CAT_HOLD[animCount / 10 % l]);
      break;
  }
}

void Render(uint16_t* pixs, Rect* srcRect) {
  PixData screen = PixData{
      pixs,
      Rect{gState.scrollPoint.x, gState.scrollPoint.y, srcRect->w, srcRect->h}};

  // Clear Board
  RenderFillRect(screen, screen.size, GBAColours[1]); //  + ((animCount / 10) % 5)

  // Windows
  int l = sizeof(SPR_WINDOW_CAT) / sizeof(int);
  for (int y = 0; y < 4; ++y) {
	  // Clothes Line
	  if (y > 0) {
		  Pt startPt = Pt{ screen.size.x, 30 - 32 * y + 17 };
		  if (in(screen.size, startPt)) {
			  int c = screen.GetI(startPt);
			  int tog = startPt.x % 2;
			  for (int x = 0; x < screen.size.w; ++x) {
				  screen.pixs[c++] = GBAColours[tog*2];
				  tog ^= 1;
			  }
		  }
	  }

	  for (int x = 0; x < 4; ++x) {
		  RenderSprite(screen, Pt{ 20 + 80 * x, 30 - 32 * y }, gState.sprites, SPR_WINDOW_CAT[animCount / 5 % l]);
	  }
  }
  ++animCount;

  // Fence
  RenderBackground(screen, Pt{ 0, srcRect->h - gState.floor.size.h },
	  gState.floor);

  // Score
  Pt cur = Pt{ 24 + 8, 66 };
  for (int i = 0; i < 10; ++i) {
	  // screen.pixs[screen.GetI(cur)] = 0xF00;
	  int padLeft = (8 - gState.sprites.sprRect.at(i).w) / 2;
	  cur.x += padLeft;
	  Rect res = RenderSprite(screen, cur, gState.sprites, i);
	  cur.x += 8 - padLeft;  // res.w + 1;
  }

  // Cat
  RenderCat(screen, srcRect, gState.cat);

  // Flashing Border
  if ((0) && (((animCount / 30) % 2) == 0)) {
    RenderBorderRect(screen, ShrinkGrow(screen.size, -1), 0xF00, 1, 0);
    RenderBorderRect(screen, ShrinkGrow(screen.size, -1), 0x00F, 0, 1);
  }
}

// DEBUG
void DebugPt(Pt m) {
  SDL_Log("Mouse [%d,%d] -> [%d,%d]", m.x, m.y, m.x + gState.scrollPoint.x,
          m.y + gState.scrollPoint.y);
}