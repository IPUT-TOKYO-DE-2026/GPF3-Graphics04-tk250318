#include "FrameBufferEmulator.h"


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

// 初期化処理（最初に1回だけ呼び出される）
void FrameBufferEmulator::initUser()
{
	// フレームバッファの中心座標を求める
	centerX = width / 2;
	centerY = height / 2;
	radius = 100; // 初期の半径
}

// 描画処理（毎フレーム呼び出される）
void FrameBufferEmulator::drawUser(unsigned char* buff, int mode, int keyLevel, int keyTrigger)
{

	if (keyTrigger == SDLK_UP) { // 上矢印キーが押されたら
		radius++;  // 半径を大きくする
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
