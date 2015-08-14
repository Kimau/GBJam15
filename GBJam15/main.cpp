#include "SDL.h"
#include <iostream>

#include "s_animCounter.h"

#define SCREEN_TITLE "GBJam #15 - Kimau"

#if 0  // Big Res
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define GB_WIDTH 320
#define GB_HEIGHT 240
#else
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 432
#define GB_WIDTH 160
#define GB_HEIGHT 144
#endif

void Log(const char *msg) {
  std::cout << "LOG:" << msg << std::endl;  // Bleh
}

void Error(const char *msg) {
  std::cout << "ERROR:" << msg << " :: " << SDL_GetError()
            << std::endl;  // Bleh
}

struct SDLAPP {
  SDL_Window *m_window;
  SDL_Renderer *m_renderer;
};

SDLAPP *CreateApp() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    Error("SDL_Init");
    return nullptr;
  }

  SDLAPP *pApp = new SDLAPP();
  pApp->m_window = SDL_CreateWindow(SCREEN_TITLE, 100, 100, SCREEN_WIDTH,
                                    SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (pApp->m_window == nullptr) {
    Error("SDL_CreateWindow");
    delete pApp;
    return nullptr;
  }

  pApp->m_renderer = SDL_CreateRenderer(
      pApp->m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (pApp->m_renderer == nullptr) {
    Error("SDL_CreateRenderer");
    SDL_DestroyWindow(pApp->m_window);
    delete pApp;
    return nullptr;
  }

  return pApp;
}

void CleanQuit(SDLAPP *pApp) {
  SDL_DestroyRenderer(pApp->m_renderer);
  SDL_DestroyWindow(pApp->m_window);
  SDL_Quit();
}

void RenderTestScene(uint16_t *pixs, SDL_Rect srcRect) {
  uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

  for (int x = 0; x < GB_WIDTH; x++) {
    for (int y = 0; y < GB_HEIGHT; y++) {
      pixs[x + y * GB_WIDTH] = GBAColours[y % 4];
    }

    pixs[x] = 0x0FFF;
    pixs[x + (GB_HEIGHT / 2) * GB_WIDTH] = 0x0F00;
  }
}

const Uint32 s_FrameRate = 1000 / 30;
static SDL_Event s_event;
static Uint32 start_time = SDL_GetTicks();
static ButState buttons;
static GameStateData *pGameState = NULL;

int GameStep() {
  start_time = SDL_GetTicks();

  // Events
  while (SDL_PollEvent(&s_event)) {
    switch (s_event.type) {
      case SDL_WINDOWEVENT:
        // if (event.window.windowID == windowID)
        switch (s_event.window.event) {
          case SDL_WINDOWEVENT_CLOSE: {
            Log("Event: Close Window");
            break;
          }
        }
        break;

      case SDL_MOUSEBUTTONUP:
        DebugPt(pGameState, Pt{s_event.button.x / 3, s_event.button.y / 3});

      case SDL_KEYDOWN:
        switch (s_event.key.keysym.scancode) {
          case SDL_SCANCODE_UP:
            buttons.up = 3;
            break;
          case SDL_SCANCODE_DOWN:
            buttons.down = 3;
            break;
          case SDL_SCANCODE_LEFT:
            buttons.left = 3;
            break;
          case SDL_SCANCODE_RIGHT:
            buttons.right = 3;
            break;
        }
        break;

      case SDL_KEYUP:
        switch (s_event.key.keysym.scancode) {
          case SDL_SCANCODE_UP:
            buttons.up = 0;
            break;
          case SDL_SCANCODE_DOWN:
            buttons.down = 0;
            break;
          case SDL_SCANCODE_LEFT:
            buttons.left = 0;
            break;
          case SDL_SCANCODE_RIGHT:
            buttons.right = 0;
            break;
        }
        break;

      case SDL_QUIT:
        Log("Event: QUIT");
        return 0;

      default:
        Log("Event: ");  // %d", s_event.type);  // Event Spam
    }

    // TODO :: Handle Event
  }

  // Update
  Tick(pGameState, &buttons);

  return 1;
}

int main(int argc, char *argv[]) {
  SDLAPP *pApp = CreateApp();
  if (pApp == nullptr) {
    return -1;
  }

  SDL_Rect srcRect = {0, 0, GB_WIDTH, GB_HEIGHT};
  SDL_Rect tarRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
  uint16_t *pixs = new uint16_t[GB_WIDTH * GB_HEIGHT];
  SDL_Texture *pBackBuffTex =
      SDL_CreateTexture(pApp->m_renderer, SDL_PIXELFORMAT_RGB444,
                        SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);

  buttons = {0, 0, 0, 0};
  pGameState = GameSetup(GB_WIDTH, GB_HEIGHT);

  if (GameStep() == 0) {
    CleanQuit(pApp);
    return 0;
  }

  // RenderTestScene(pixs, srcRect);

  do {
    Render(pGameState, pixs, &Rect{srcRect.x, srcRect.y, srcRect.w, srcRect.h});
    SDL_UpdateTexture(pBackBuffTex, &srcRect, pixs, GB_WIDTH * 2);

    SDL_RenderClear(pApp->m_renderer);
    SDL_RenderCopy(pApp->m_renderer, pBackBuffTex, &srcRect, &tarRect);
    SDL_RenderPresent(pApp->m_renderer);

    // Sleep
    if ((s_FrameRate) > (SDL_GetTicks() - start_time)) {
      SDL_Delay((s_FrameRate) -
                (SDL_GetTicks() - start_time));  // Yay stable framerate!
    }

  } while (GameStep());

  CleanQuit(pApp);
  return 0;
}
