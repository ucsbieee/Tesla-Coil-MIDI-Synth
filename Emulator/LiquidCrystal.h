#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <unordered_map>
#include <vector>
#include <tuple>
#include <inttypes.h>

// Emulate Arduino LiquidCrystal library LCD screen with SDL

class Coil;

class LiquidCrystal {
public:
	LiquidCrystal(Coil *parent, const char *windowName);
	~LiquidCrystal();

	void begin(int w, int h);

	void clear();
	void setCursor(int x, int y);

	void print(const char *str);
	void print(char c);
	void print(int i);

	void createChar(uint8_t value, uint8_t *bitmap);
	void write(uint8_t value);

	void present();

	static void initialize();
	static void destroy();
	static bool pollSDL();

private:
	Coil *coil;
	const char *windowName;

	SDL_Window *window;
	SDL_Renderer *renderer;

	float scaleX, scaleY;

	static constexpr SDL_Color bgColor{0, 0, 0, 255};
	static constexpr SDL_Color fgColor{255, 255, 255, 255};

	std::unordered_map<char, std::tuple<SDL_Texture*, int, int>> renderCache;

	int w = 0, h = 0;
	int cursorX = 0, cursorY = 0;

	std::vector<char> lcdContent;

	static TTF_Font *font;
	static int charW, charH;

	// Map between window IDs and Coil objects
	static std::unordered_map<uint32_t, Coil*> windowMap;

	// Knob actions
	static void buttonClick(uint32_t windowID);
	static void scrollCW(uint32_t windowID);
	static void scrollCCW(uint32_t windowID);
};
