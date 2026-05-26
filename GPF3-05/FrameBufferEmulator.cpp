#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "FrameBufferEmulator.h"

// #define	SCREEN_FIX	true	//一度描画して終了（今回はいじらないこと）

FrameBufferEmulator::FrameBufferEmulator()
	: width(DefaultWidth), height(DefaultHeight),sym(0)
{
}

FrameBufferEmulator::FrameBufferEmulator(int w, int h)
	: width(w), height(h), sym(0)
{
}

FrameBufferEmulator::~FrameBufferEmulator()
{
}

int FrameBufferEmulator::init()
{
	//初期化部
	if (SDL_Init(SDL_INIT_VIDEO) && IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG)) {//初期化
		IMG_Quit();		//SDL_IMageを終了する処理
		SDL_Quit();		//SDLを終了する処理
		return 1;		//プログラムにエラーだから終了すると伝える
	}

	//ウィンドウ作る部
	window = SDL_CreateWindow("FrameBuffer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);//ウィンドウ作成
	if (window == NULL) {//ウィンドウが作成できていなかったとき
		SDL_UnlockSurface(image);	//画像データを捨てる処理
		SDL_DestroyWindow(window);	//ウィンドウを捨てる処理
		IMG_Quit();					//SDL_IMageを終了する処理
		SDL_Quit();					//SDLを終了する処理
		return 1;					//プログラムにエラーだから終了すると伝える
	}

	//ウィンドウを使う為のデータを登録部
	//renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {				//レンダーが作成できていなかったとき
		SDL_UnlockSurface(image);		//画像データを捨てる処理
		SDL_DestroyRenderer(renderer);	//ウィンドウの描画するための情報を削除する
		SDL_DestroyWindow(window);		//ウィンドウを捨てる処理
		IMG_Quit();						//SDL_IMageを終了する処理
		SDL_Quit();						//SDLを終了する処理
		return 1;						//プログラムにエラーだから終了すると伝える
	}

	int r = Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG);
	Mix_AllocateChannels(-1);	// 最大チャンネル数

	return 0;
}

int FrameBufferEmulator::loadImage(const char* filename)
{
	//画像読み込み部
	imageSrc = SDL_LoadBMP(filename);
	if (imageSrc == NULL) {	//画像が読み込めなかった時
		IMG_Quit();			//SDL_IMageを終了する処理
		SDL_Quit();			//SDLを終了する処理
		return 1;			//プログラムにエラーだから終了すると伝える
	}
	image = SDL_LoadBMP(filename);

	//画像を使う為のデータ入れる部
	image_texture = SDL_CreateTextureFromSurface(renderer, image);
	if (image_texture == NULL) {			//テクスチャが作成できていなかったとき
		SDL_UnlockSurface(image);			//画像データを捨てる処理
		SDL_DestroyTexture(image_texture);	//画像を使う為のデータを削除する
		SDL_DestroyRenderer(renderer);		//ウィンドウの描画するための情報を削除する
		SDL_DestroyWindow(window);			//ウィンドウを捨てる処理
		IMG_Quit();							//SDL_IMageを終了する処理
		SDL_Quit();							//SDLを終了する処理
		return 1;							//プログラムにエラーだから終了すると伝える
	}

	//画像加工部
	SDL_QueryTexture(image_texture, NULL, NULL, &image_w, &image_h);		//テクスチャ(画像)の大きさを知る関数
	imageRect = { 0,0,image_w,image_h };	//画像の標準の大きさの指定
	drawRect = { 0,0,image_w,image_h };		//画像を描画する時の場所の指定

	return 0;
}

// 文字表示設定
int	FrameBufferEmulator::loadFONT(const char* fontname) {
	if (TTF_Init() == -1)//初期化
	{
		printf("TTF_Init失敗: %s\n", TTF_GetError());  return -1;
	}
	if ((font = TTF_OpenFont(fontname, 24)) == NULL)//フォント取得
	{
		printf("TTF_OpenFont失敗: %s\n", TTF_GetError());  return -1;
	}

	return 0;
}

// 音声ファイルの読み込み
Mix_Chunk* FrameBufferEmulator::loadSound(const char* fname) {
	Mix_Chunk* p;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);	// signed 16bit
	//Mix_OpenAudio(44100 / 2, AUDIO_S8, 2, 4096);			// 
	if ((p = Mix_LoadWAV(fname)) == NULL) {
		fprintf(stderr, "Mix_LoadWAV: %s\n", Mix_GetError());
	}
	return p;
}

void FrameBufferEmulator::playSound(Mix_Chunk* chunk, int loops)
{
	Mix_PlayChannel(-1, chunk, loops);
}


int FrameBufferEmulator::drawLoop()
{
	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0, deltaTimeUser = 0;
	double	maxDeltaTime = 0, maxDeltaTimeUser = 0, deltaTotal = 0, deltaTotalUser = 0, deltaAvg = 0;
	int ct = 0, maxCt = 0, maxCtUser = 0, frameCt = 0;

	SDL_Color clr = { 255,255,0 };	// 文字の色
	char	tmp[256] = "";

	SDL_Rect dst; dst.x = 6; dst.y = 4;	// 文字列を表示し始める位置
	while (!isQuit()) {

		//フレームバッファのクリア
		if (!dontClear) {
			if (bg_texture) {
				//テクスチャ有りなら最初にフレームバッファを画像で埋める
				memcpy(image->pixels, imageSrc->pixels, (width * height * ColorDepth));
			}
			else {
				//テクスチャ無しなら黒塗り
				memset(image->pixels, 0, (width * height * ColorDepth));
			}
		}
		int keyLevel = sym;
		int keyTrigger = 0;
		if (key_old == false) {
			keyTrigger = keyLevel;
		}
		for (auto& k : inputKeys) {
			if (k.second.old == false) {
				k.second.trigger = k.second.level;
			}
		}

		if (keyTrigger == SDLK_DELETE) {	//背景グラフィックのON/OFF
			bg_texture = 1 - bg_texture;
		}
		if (keyTrigger == SDLK_INSERT) {	//ワークメーターのON/OFF
			workmeter = 1 - workmeter;
		}
		key_old = keyLevel;
		for (auto& k : inputKeys) {
			k.second.old = k.second.level;
		}

		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();

		drawUser((unsigned char*)image->pixels, 0, keyLevel, keyTrigger);
		//SDL_Delay(10);

		// １フレーム時間計算
		deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
		deltaTimeUser = (double)((SDL_GetPerformanceCounter() - NOW) * 1000 / (double)SDL_GetPerformanceFrequency());
#if WORKMETER
		if (workmeter)	dispWorkMeter(deltaTime, deltaTimeUser);	// ワークメータを描画する
#endif // WORKMETER

		if (ct > 10) {	// 実行直後は安定しないので、計測開始までちょっとあける
			deltaTotal += deltaTime;
			deltaTotalUser += deltaTimeUser;
			frameCt++;
			if (deltaTime > maxDeltaTime) {
				maxDeltaTime = deltaTime;
				maxCt = ct - 1;
			}
			if (deltaTimeUser > maxDeltaTimeUser) {
				maxDeltaTimeUser = deltaTimeUser;
				maxCtUser = ct - 1;
			}


			if (workmeter) {
				// テキスト表示のために画面上部を消しておく
				unsigned char* buff = (unsigned char*)image->pixels;
#if 0
				for (int i = 0; i < (width * 24 * 2); i++) {
					//半透明
					buff[0] = buff[0] / 2;	buff[1] = buff[1] / 2; buff[2] = buff[2] / 2;
					buff += ColorDepth;
				}
#else
				changeRGB(buff, 0.5f, 0.5f, 0.5f, 10, 0, width - 1, 48);
#endif
				//文字描画処理
#if WORKMETER_ALL

				sprintf(tmp, "Frame %4d : All %3fms (max %4fms %4d) User %4fms (max %4fms %4d)  : Avg %fms %ffps",
					ct, deltaTime, maxDeltaTime, maxCt, deltaTimeUser, maxDeltaTimeUser, maxCtUser, deltaTotalUser / frameCt, 1000 / (deltaTotal / frameCt));
#else



				sprintf_s(tmp, "  Frame %4d CPU Time: %5.2fms(Avg %5.2fms) CPU Load:%5.2f%%(Avg %5.2f%%) ",
					ct, deltaTimeUser, deltaTotalUser / frameCt,
					100. * deltaTimeUser / MS_PER_FRAME,
					100. * deltaTotalUser / ((double)frameCt * MS_PER_FRAME));
#endif
			}

		}
		ct++;

		if (workmeter) {
			clr = { 255,255,0 };	// 文字の色
			dst.x = 6; dst.y = 0;	// 文字列を表示し始める位置
			txtSurface = TTF_RenderUTF8_Solid(font, tmp, clr);						// 文字列書き込み用のサーフェスを作る
			SDL_BlitSurface(txtSurface, NULL, image, &dst);							// 文字列書き込み用のサーフェスから書き込み
			SDL_FreeSurface(txtSurface);											// 文字列書き込み用のサーフェスを破棄
		}


		draw(image);	// １フレームの表示処理実行
	}
	return ct;
}

void FrameBufferEmulator::changeRGB(unsigned char* buff, float r_rate, float g_rate, float b_rate, int x0, int y0, int x1, int y1)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if ((x >= x0) && (x <= x1) && (y >= y0) && (y <= y1)) {	//x,yが指定された矩形の範囲内であれば色に倍率を掛けて描き戻す
				unsigned char* p = buff + (y * width + x) * ColorDepth;	//画面上のx,y座標を該当ピクセルのフレームバッファのアドレスに変換
				*p = (int)((*p++) * b_rate);
				*p = (int)((*p++) * g_rate);
				*p = (int)((*p++) * r_rate);
			}
		}
	}
}

void FrameBufferEmulator::destroy()
{
	//終了処理
	Mix_HaltChannel(-1);
	Mix_CloseAudio();
	Mix_Quit();

	SDL_UnlockSurface(image);//画像データを捨てる処理
	SDL_DestroyTexture(image_texture);//画像を使う為のデータを削除する
	SDL_DestroyRenderer(renderer);//ウィンドウの描画するための情報を削除する
	SDL_DestroyWindow(window);	//ウィンドウを削除する
	//IMG_Quit();//SDL_IMageを終了する処理
	SDL_Quit();					//SDLを終了する処理
}

void FrameBufferEmulator::addInputKey(SDL_Keycode key)
{
	// 入力判定をするキーを登録する
	inputKeys.emplace(key, InputKey());
}

bool FrameBufferEmulator::getKeyTrigger(SDL_Keycode key)
{
	// 登録されたキーではない場合は常にfalse
	bool trigger = false;
	auto k = inputKeys.find(key);
	if (k != inputKeys.end()) { // 登録されたキーの場合のみ判定する
		trigger = k->second.trigger;
		k->second.trigger = false;
	}
	return trigger;
}

bool FrameBufferEmulator::getKeyLevel(SDL_Keycode key)
{
	// 登録されたキーではない場合は常にfalse
	bool level = false;
	auto k = inputKeys.find(key);
	if (k != inputKeys.end()) { // 登録されたキーの場合のみ判定する
		level = k->second.level;
	}
	return level;
}

// keydown to quit
int	FrameBufferEmulator::isQuit()
{
	// イベントがなくなるまでループする
	while (SDL_PollEvent(&event) == 1) {
		if (event.type == SDL_QUIT) {
			// 終了
			return -1;
		}

		// ここにキー入力の処理等を書く
		if (event.type == SDL_KEYDOWN) {
			sym = event.key.keysym.sym;
			auto k = inputKeys.find(event.key.keysym.sym);
			if (k != inputKeys.end()) {
				k->second.level = true;
			}
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				return -1;
			}
		} else if(event.type == SDL_KEYUP) {
			sym = 0;
			auto k = inputKeys.find(event.key.keysym.sym);
			if (k != inputKeys.end()) {
				k->second.level = false;
			}
		}
		else {
			event.key.keysym.sym = 0;
		}
	}
	return 0;
}

void FrameBufferEmulator::draw(SDL_Surface* img) {
	if (image_texture != NULL) {
		SDL_DestroyTexture(image_texture);//画像を使う為のデータを削除する
	}
	image_texture = SDL_CreateTextureFromSurface(renderer, img);

	//描画部
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//指定のカラーで画面を塗る設定をする
	SDL_RenderClear(renderer);										//実際にぬる
	SDL_RenderCopy(renderer, image_texture, &imageRect, &drawRect);//画像をウィンドウに描画するための情報の描画部分に送る
	SDL_GL_SwapWindow(window);										// Vsync wait
	SDL_RenderPresent(renderer);									//ここで描画！
}

