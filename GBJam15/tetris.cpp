#include "tetris.h"
#include <vector>

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

struct GameStateData {
  Rect brickArea;
  int width, height;
  CatData cat;
  SpriteData sprites;
  BackgroundData floor;
} gState;

/////// MACRO

#ifdef _DEBUG
#define SETPIX(__x, __y, __c)                                            \
  {                                                                      \
    SDL_assert(((__x) >= 0) && ((__y) >= 0) && ((__x) < gState.width) && \
               ((__y) < gState.height));                                 \
    pixs[(__x) + (__y)*gState.width] = (__c);                            \
  }
#else
#define SETPIX(__x, __y, __c) \
  { pixs[(__x) + (__y)*gState.width] = (__c); }
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
  gState.brickArea.x = 4;
  gState.brickArea.y = 4;
  gState.brickArea.w = width / 2;
  gState.brickArea.h = height - 8;

  gState.width = width;
  gState.height = height;

  SetupSprites(gState.sprites, "sprites.bmp");
  SetupBackground(gState.floor, "floor.bmp");

  gState.cat.pos = {30, gState.height - FLOOR_HEIGHT -
                            gState.sprites.sprRect.at(SPR_CAT_IDLE[0]).h};
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
      if ((ticker % 3) > 0) gState.cat.pos.x -= 1;
      break;
    case CatData::Right:
      if ((ticker % 3) > 0) gState.cat.pos.x += 1;
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

  // Clear Press
  buttons->up &= 1;
  buttons->down &= 1;
  buttons->left &= 1;
  buttons->right &= 1;

  ++ticker;
}

void FillRect(uint16_t *pixs, Rect *tarRect, uint16_t col) {
  for (int x = tarRect->x; x < (tarRect->w + tarRect->x); ++x)
    for (int y = tarRect->y; y < (tarRect->h + tarRect->y); ++y)
      SETPIX(x, y, col);
}

Pt SpriteHorFlip(uint16_t *pixs, const Pt &topLeft, const SpriteData &sheet,
                 size_t sprID) {
  const Rect &srcRect = sheet.sprRect.at(sprID);

  for (int x = 0; x < srcRect.w; ++x) {
    for (int y = 0; y < srcRect.h; ++y) {
      switch (sheet.pixs[srcRect.w + srcRect.x - x - 1 +
                         (srcRect.y + y) * sheet.sprPitch]) {
        case 0:
          break;
        case 1:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[0]);
          break;
        case 2:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[1]);
          break;
        case 3:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[2]);
          break;
        case 4:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[3]);
          break;
        case 14:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF0F);
          break;
        default:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF00);
          break;
      }
    }
  }

  return Pt{topLeft.x + srcRect.w, topLeft.y + srcRect.h};
}

Pt Sprite(uint16_t *pixs, const Pt &topLeft, const SpriteData &sheet,
          size_t sprID) {
  const Rect &srcRect = sheet.sprRect.at(sprID);

  for (int x = 0; x < srcRect.w; ++x) {
    for (int y = 0; y < srcRect.h; ++y) {
      switch (sheet.pixs[(srcRect.x + x) + (srcRect.y + y) * sheet.sprPitch]) {
        case 0:
          break;
        case 1:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[0]);
          break;
        case 2:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[1]);
          break;
        case 3:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[2]);
          break;
        case 4:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[3]);
          break;
        case 14:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF0F);
          break;
        default:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF00);
          break;
      }
    }
  }

  return Pt{topLeft.x + srcRect.w, topLeft.y + srcRect.h};
}

void Background(uint16_t *pixs, Pt topLeft, const BackgroundData &bg) {
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

  if ((topLeft.x + srcRect.w - srcRect.x) >= gState.width) {
    srcRect.w -= (topLeft.x + srcRect.w - srcRect.x) - gState.width;
  }

  if ((topLeft.y + srcRect.h - srcRect.y) >= gState.height) {
    srcRect.h -= (topLeft.y + srcRect.h - srcRect.y) - gState.height;
  }

  for (int x = srcRect.x; x < srcRect.w; ++x) {
    for (int y = srcRect.y; y < srcRect.h; ++y) {
      switch (bg.pixs[(srcRect.x + x) + (srcRect.y + y) * bg.size.w]) {
        case 0:
          break;
        case 1:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[0]);
          break;
        case 2:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[1]);
          break;
        case 3:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[2]);
          break;
        case 4:
          SETPIX(topLeft.x + x, topLeft.y + y, GBAColours[3]);
          break;
        case 14:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF0F);
          break;
        default:
          SETPIX(topLeft.x + x, topLeft.y + y, 0xF00);
          break;
      }
    }
  }
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

static int animCount = 0;

void RenderCat(uint16_t *pixs, Rect *srcRect, CatData &cat) {
  int l = 1;
  switch (cat.state) {
    case CatData::Idle:
      l = sizeof(SPR_CAT_IDLE) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_IDLE[animCount / 10 % l]);
      break;
    case CatData::Left:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
      SpriteHorFlip(pixs, cat.pos, gState.sprites,
                    SPR_CAT_WALK[animCount / 7 % l]);
      break;
    case CatData::Right:
      l = sizeof(SPR_CAT_WALK) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_WALK[animCount / 7 % l]);
      break;
    case CatData::Up:
      l = sizeof(SPR_CAT_UP) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_UP[animCount / 10 % l]);
      break;
    case CatData::Down:
      l = sizeof(SPR_CAT_DOWN) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_DOWN[animCount / 10 % l]);
      break;
    case CatData::PounceLeft:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_POUNCE[animCount / 10 % l]);
      break;
    case CatData::PounceRight:
      l = sizeof(SPR_CAT_POUNCE) / sizeof(int);
      SpriteHorFlip(pixs, cat.pos, gState.sprites,
                    SPR_CAT_POUNCE[animCount / 10 % l]);
      break;
    case CatData::Hold:
      l = sizeof(SPR_CAT_HOLD) / sizeof(int);
      Sprite(pixs, cat.pos, gState.sprites, SPR_CAT_HOLD[animCount / 10 % l]);
      break;
  }
}

void Render(uint16_t *pixs, Rect *srcRect) {
  // Clear Board
  FillRect(pixs, srcRect, GBAColours[1]);

  BezelBoxFilled(pixs, &(gState.brickArea), GBAColours[2], GBAColours[1],
                 GBAColours[0]);

  Background(pixs, Pt{0, gState.height - gState.floor.size.h}, gState.floor);

  Pt cur = Pt{6, 6};
  for (int i = 0; i < 10; ++i) {
    cur = Sprite(pixs, cur, gState.sprites, i);
    cur.y = 6;
    cur.x += 1;
  }

  int l = sizeof(SPR_WINDOW_CAT) / sizeof(int);
  Sprite(pixs, Pt{6, 20}, gState.sprites, SPR_WINDOW_CAT[animCount / 5 % l]);
  ++animCount;

  RenderCat(pixs, srcRect, gState.cat);

  /*
  SETPIX(gState.brickArea.x, gState.brickArea.y, 0x0F00);
  SETPIX(gState.brickArea.x-1, gState.brickArea.y-1, 0x000F);
  BorderRect(pixs, &(gState.brickArea), GBAColours[2], -1, 1);
  */
}