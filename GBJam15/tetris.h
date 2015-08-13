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

	class ButState {
	public:
		int up, down, left, right;
	};

	void StartGame(uint16_t width, uint16_t height);

	void Tick(ButState* buttons);
	void Render(uint16_t* pixs, Rect* srcRect);

	// DEBUG
	void DebugPt(Pt m);
};