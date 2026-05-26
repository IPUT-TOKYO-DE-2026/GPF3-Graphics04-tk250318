#pragma once

#define	_SOUND_

#include <unordered_map>

#include	<SDL.h>			//SDLを使う為にインクルードする
#include	<sdl_image.h>	//SDL_IMageを使う為にインクルードする
#include	<SDL_ttf.h>
#ifdef	_SOUND_
#include	<SDL_mixer.h>
#endif	//_SOUND_

class InputKey
{
public:
	InputKey() : level(false), trigger(false), old(false) {};

	bool level;
	bool trigger;
	bool old;
};

class FrameBufferEmulator
{
public:
	FrameBufferEmulator();
	FrameBufferEmulator(int w, int h);
	~FrameBufferEmulator();

	int init();
	void initUser();
	int loadImage(const char* filename);
	int	loadFONT(const char* fontname);
	Mix_Chunk* loadSound(const char* fname);
	void playSound(Mix_Chunk* chunk, int loops = 0); // loops ... ループ回数（省略時は0、-1で永久に繰り返す）

	void addInputKey(SDL_Keycode key);
	bool getKeyTrigger(SDL_Keycode key);
	bool getKeyLevel(SDL_Keycode key);

	int drawLoop();
	void draw(SDL_Surface* img);
	void drawUser(unsigned char* buff, int mode, int keyLevel, int keyTrigger);
	void changeRGB(unsigned char* buff, float r_rate, float g_rate, float b_rate, int x0, int y0, int x1, int y1);
	void destroy();

	void getSize(int& w, int& h) { w = width; h = height; }

	static const int DefaultWidth = 960;
	static const int DefaultHeight = 540;

private:
	int width;
	int height;
	const int ColorDepth = 3;		//１ピクセルは３バイト(=24bit)（の色深度）
	const float MS_PER_FRAME = 16.7f;		// １フレームの時間(ms)

	SDL_Surface* image;				//画像を入れる用
	SDL_Texture* image_texture;		//画像を使うためのデータ用
	SDL_Window* window;				//ウィンドウの情報入れる用
	SDL_Renderer* renderer;			//ウィンドウを使う為のデータ用
	int image_w, image_h;			//画像の大きさを保存するための変数
	SDL_Rect imageRect = { 0,0,image_w,image_h };	//画像の標準の大きさの指定
	SDL_Rect drawRect = { 0,0,image_w,image_h };	//画像を描画する時の場所の指定
	TTF_Font* font;
	SDL_Surface* txtSurface;			// 文字を描画するテクスチャ
	SDL_Surface* imageSrc;	//元テクスチャ

	SDL_Event event;
	SDL_Keycode sym;
	int key_old;

	int	bg_texture = 1;	//BGをテクスチャにするか黒塗りか
	int	workmeter = 1;
	bool dontClear = false;

	std::unordered_map<SDL_Keycode, InputKey> inputKeys;

	int isQuit();
};

