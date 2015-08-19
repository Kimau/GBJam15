#include "SDL.h"

extern "C" {

struct GameStateData;

class Rect {
 public:
  int x, y, w, h;
};

class Pt {
 public:
  int x, y;
};

class ButState {	
 public:
  int up, down, left, right;
};

GameStateData* GameSetup(uint16_t width, uint16_t height);

void Tick(GameStateData* pGameState, ButState* buttons);
void Render(GameStateData* pGameState, uint16_t* pixs, Rect* srcRect);

// DEBUG
void DebugPt(GameStateData* pGameState, Pt m);
};