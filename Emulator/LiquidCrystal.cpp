#include "LiquidCrystal.h"
#include "Coil.h"
#include "BarChars.h"

#include <string>
#include <cstring>
#include <stdexcept>

using namespace std;

extern const char lcdFont[], lcdFontEnd[];

TTF_Font *LiquidCrystal::font;
int LiquidCrystal::charW, LiquidCrystal::charH;

unordered_map<uint32_t, Coil*> LiquidCrystal::windowMap;

void LiquidCrystal::initialize() {
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		throw runtime_error(string("SDL_Init: ") + SDL_GetError());
	
	if(TTF_Init() < 0)
		throw runtime_error(string("TTF_Init: ") + TTF_GetError());

	SDL_RWops *fontDataRWops = SDL_RWFromConstMem(lcdFont, lcdFontEnd - lcdFont);
	if(!fontDataRWops)
		throw runtime_error(string("SDL_RWFromConstMem: ") + SDL_GetError());

	font = TTF_OpenFontRW(fontDataRWops, 1, 24);
	if(!font)
		throw runtime_error(string("TTF_OpenFontRW: ") + TTF_GetError());
	
	if(TTF_SizeUTF8(font, "E", &charW, &charH) < 0)
		throw runtime_error(string("TTF_SizeUTF8: ") + TTF_GetError());
}

void LiquidCrystal::destroy() {
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
}

bool LiquidCrystal::pollSDL() {
	SDL_Event event;
	if(!SDL_WaitEventTimeout(&event, 10))
		return true;

	switch(event.type) {
		case SDL_QUIT:
			return false;
		
		case SDL_KEYDOWN:
			if(event.key.repeat)
				return true;
			
			switch(event.key.keysym.sym) {
				case SDLK_LEFT:
				case SDLK_DOWN:
					scrollCCW(event.key.windowID);
					return true;
				
				case SDLK_RIGHT:
				case SDLK_UP:
					scrollCW(event.key.windowID);
					return true;
				
				case SDLK_RETURN:
				case SDLK_SPACE:
					buttonClick(event.key.windowID);
					return true;
				
				default:
					return true;
			}
		
		case SDL_MOUSEBUTTONDOWN:
			if(event.button.button == SDL_BUTTON_LEFT)
				buttonClick(event.button.windowID);
			return true;
		
		case SDL_MOUSEWHEEL:
			if(event.wheel.y > 0)
				scrollCCW(event.wheel.windowID);
			else if(event.wheel.y < 0)
				scrollCW(event.wheel.windowID);
			return true;
		
		default:
			return true;
	}
}

void LiquidCrystal::buttonClick(uint32_t windowID) {
	auto it = windowMap.find(windowID);
	if(it == windowMap.end())
		return;
	
	it->second->lcdObj.editing ^= 1;
}

void LiquidCrystal::scrollCW(uint32_t windowID) {
	auto it = windowMap.find(windowID);
	if(it == windowMap.end())
		return;
	
	it->second->knob.movement += 4;
	it->second->knob.enc();
}

void LiquidCrystal::scrollCCW(uint32_t windowID) {
	auto it = windowMap.find(windowID);
	if(it == windowMap.end())
		return;
	
	it->second->knob.movement -= 4;
	it->second->knob.enc();
}

LiquidCrystal::LiquidCrystal(Coil *parent, const char *windowName): coil(parent), windowName(windowName) {}

LiquidCrystal::~LiquidCrystal() {
	for(auto c:renderCache)
		SDL_DestroyTexture(std::get<0>(c.second));
	SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void LiquidCrystal::begin(int w, int h) {
	this->w = w;
	this->h = h;

	const int requestedW = (w+1)*charW;
	const int requestedH = (h+1)*charH;
	window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, requestedW, requestedH, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
	if(!window)
		throw runtime_error(string("SDL_CreateWindow: ") + SDL_GetError());
	
	windowMap.emplace(SDL_GetWindowID(window), coil);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!renderer)
		throw runtime_error(string("SDL_CreateRenderer: ") + SDL_GetError());
	
	int renderW = 0, renderH = 0;
	if(SDL_GetRendererOutputSize(renderer, &renderW, &renderH) < 0)
		throw runtime_error(string("SDL_GetRendererOutputSize: ") + SDL_GetError());
	scaleX = (float)renderW/requestedW;
	scaleY = (float)renderH/requestedH;

	lcdContent.resize(w*h);

	clear();
}

void LiquidCrystal::clear() {
	for(char &c:lcdContent)
		c = ' ';
}

void LiquidCrystal::setCursor(int x, int y) {
	cursorX = x;
	cursorY = y;
}

void LiquidCrystal::print(const char *str) {
	while(*str && cursorX < w && cursorY < h)
		lcdContent.at(cursorX++ + cursorY*w) = *str++;
}

void LiquidCrystal::print(char c) {
	if(cursorX < w && cursorY < h)
		lcdContent.at(cursorX++ + cursorY*w) = c;
}

void LiquidCrystal::print(int i) {
	char str[w+1];
	snprintf(str, w+1, "%d", i);
	print(str);
}

void LiquidCrystal::createChar(uint8_t value, uint8_t *bitmap) {
	if(renderCache.find(value) != renderCache.end())
		return;
	
	// Create a new texture for this character
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, charW, charH);
	if(!texture)
		throw runtime_error(string("SDL_CreateTexture: ") + SDL_GetError());
	
	// Render to the new texture
	if(SDL_SetRenderTarget(renderer, texture) < 0)
		throw runtime_error(string("SDL_SetRenderTarget: ") + SDL_GetError());
	
	// Background
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 0);
    SDL_RenderClear(renderer);

	// Draw a bunch of squares
	SDL_SetRenderDrawColor(renderer, fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	const int pixelSize = charW / 6;
	const int margin = pixelSize / 3;
	for(int rowInd = 0; rowInd < 8; rowInd++) {
		for(int colInd = 0; colInd < 5; colInd++)
			if(bitmap[rowInd] & (1 << (4 - colInd))) {
				const SDL_Rect rect = {margin + pixelSize * colInd,
				                       pixelSize + margin + pixelSize * rowInd,
				                       pixelSize - margin,
				                       pixelSize - margin};
				SDL_RenderFillRect(renderer, &rect);
			}
	}

	renderCache.emplace(value, tuple<SDL_Texture*, int, int>{texture, charW, charH});

	// Set renderer back to the main window
	if(SDL_SetRenderTarget(renderer, nullptr) < 0)
		throw runtime_error(string("SDL_SetRenderTarget: ") + SDL_GetError());
}

void LiquidCrystal::write(uint8_t value) {
	if(value == BAR_EMPTY)
		print(' ');
	else
		print((char)value);
}

void LiquidCrystal::present() {
	// Clear with black
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderClear(renderer);

	// Render each character
	int x = 0, y = 0;
	for(const char c:lcdContent) {
		auto it = renderCache.find(c);

		if(it == renderCache.end()) {
			char str[2] = {c, 0};
			SDL_Surface *surface = TTF_RenderUTF8(font, str, fgColor, bgColor);
			if(!surface)
				throw runtime_error(string("TTF_RenderText_Solid: ") + TTF_GetError());
			
			SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
			if(!texture)
				throw runtime_error(string("SDL_CreateTextureFromSurface: ") + SDL_GetError());
			
			it = renderCache.emplace(c, tuple<SDL_Texture*, int, int>{texture, surface->w, surface->h}).first;

			SDL_FreeSurface(surface);
		}

		const auto &info = it->second;
		SDL_Texture *texture = std::get<0>(info);
		const int texW = std::get<1>(info), texH = std::get<2>(info);
		SDL_Rect dstRect = {static_cast<int>((charW/2 + x*charW) * scaleX),
		                    static_cast<int>((charH/2 + y*charH) * scaleY),
		                    static_cast<int>(texW * scaleX),
		                    static_cast<int>(texH * scaleY)};
		SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
		x++;
		if(x >= w) {
			x = 0;
			y++;
		}
	}

	SDL_RenderPresent(renderer);
}
