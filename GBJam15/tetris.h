#include "SDL.h"

extern "C" {

	class Rect {
	public:
		int x, y, w, h;
	};

	class Pt {
	public:
		int x, y;
	};

	void StartGame(uint16_t width, uint16_t height);

	void Tick();
	void Render(uint16_t* pixs, Rect* srcRect);
};