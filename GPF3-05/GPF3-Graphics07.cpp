#include "FrameBufferEmulator.h"
#include <iostream>
#include <ctime>


//  buff フレームバッファの先頭アドレス
//  width, height フレームバッファの高さと横幅
//  radius 円の半径
//  centerX, centerY 中心座標(X, Y)
//  color 描画色（B,G,Rの配列）
void drawFilledCircle(unsigned char* buff, int width, int height, int radius, int centerX, int centerY, unsigned char color[3])
{
	const int squaredRadius = radius * radius; // 半径の二乗
	for (int y = 0; y < height; y++) { // 縦方向のループ
		int squaredY = y - centerY;	// Y軸の中心からの距離
		squaredY *= squaredY;  // 二乗しておく
		for (int x = 0; x < width; x++) { // 横方向のループ
			int squaredX = x - centerX;	// X軸の中心からの距離
			squaredX *= squaredX;  // 二乗しておく
			if (squaredY + squaredX <= squaredRadius) { // 距離が半径以下ならば（二乗どうしで比較）
				// 現在のX,Yで示す位置は円の内側として色を置く
				*buff++ = color[0];  // B
				*buff++ = color[1];  // G
				*buff++ = color[2];  // R
			}
			else {
				buff += 3;	// 現在のX,Yで示す位置は円の外側（色は置かず次のピクセルに移る）
			}
		}
	}
}

int centerX; // 円の中心座標X
int centerY; // 円の中心座標Y
int radius;  // 円の半径
unsigned char color[3] = { 10, 200, 0 }; // B, G, R
Uint32 lastUpdateTime = 0; // 押され始めた時間
bool isWaitingLongPress = false; // 長押しの待ち時間かどうかの判定

// 初期化処理（最初に1回だけ呼び出される）
void FrameBufferEmulator::initUser()
{
	// フレームバッファの中心座標を求める
	centerX = width / 2; // 横幅の半分を代入
	centerY = height / 2; // 縦幅の半分を代入
	radius = 100; // 初期の半径
}

// 描画処理（毎フレーム呼び出される）
void FrameBufferEmulator::drawUser(unsigned char* buff, int mode, int keyLevel, int keyTrigger)
{

	if (keyTrigger == SDLK_UP) { // 上矢印キーが押されたら(押したとき1度だけ呼び出される)
		radius++;  // 最初に一回だけ半径を大きくする
		lastUpdateTime = SDL_GetTicks(); // 押され始めた時間を取得
		isWaitingLongPress = true; // 待機モードに突入
	}
	if (keyLevel == SDLK_UP) { // 上矢印キーが押され続けたら(押し続けている限り何度でも呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 押され続けた場合の処理
		if (isWaitingLongPress) { // 待機モード
			if (now - lastUpdateTime > 500) { // 500ミリ秒(0.5秒)経過したら
				radius++; // 半径を大きくし始める
				lastUpdateTime = now; // 連続モードに移行し始めた時刻を記録
				isWaitingLongPress = false; // 待機モードを解除し連続モードに移行
			}
		}
		else { // 連続モード
			if (now - lastUpdateTime > 50) { // 50ミリ秒経過したら
				radius++; // 半径を大きくし続ける
				lastUpdateTime = now; // 半径を大きくしたら時間をリセット
			}
		}
	}
	
	if (keyTrigger == SDLK_DOWN) { // 下矢印キーが押されたら
		radius--; // 半径を小さくする
	}
	if (keyTrigger == SDLK_LEFT) { // 左矢印キーが押されたら
		color[0] += 10; // 青を増やす
	}
	if (keyTrigger == SDLK_RIGHT) { // 右矢印キーが押されたら
		color[0] -= 10; // 青を減らす
	}
	drawFilledCircle(buff, width, height, radius, centerX, centerY, color); // 円を描画する
}
