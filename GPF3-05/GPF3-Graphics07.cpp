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

// 色相環の角度(0 〜 359)からRGBを計算してcolor配列に入れる関数
void updateColorFromHue(int hue, unsigned char color[3]) {
	// 色相を 0 〜 359 の範囲に安全に収める（マイナスになった時などの対策）
	// hue % 360 で範囲を -359 ～ 359 にする
	// + 360 によって範囲を 0 ～ 719 にする
	// さらにその結果を % 360 することで 0 ～359 の範囲になる
	hue = (hue % 360 + 360) % 360;

	// 360度を60度ごとの6つのフェーズに分ける
	int phase = hue / 60;          // 結果は 0, 1, 2, 3, 4, 5 のどれかになる
	int remainder = hue % 60;      // そのフェーズの中でどれくらい進んだか（0〜59）

	// 進み具合(0〜59)を、RGBの強さ(0〜255)に変換する
	int up = (remainder * 255) / 60;   // 0から255へ上がる間隔
	int down = 255 - up;               // 255から0へ下がる間隔

	// RGBの変数を定義(わかりやすいように）
	int r = 0, g = 0, b = 0;

	// フェーズごとに、どれがMAXで、どれが上がって、どれ下がるかを割り振る
	switch (phase) {
	case 0: r = 255;  g = up;   b = 0;    break; // フェーズ0 (0 ～ 59度) : 赤→黄
	case 1: r = down; g = 255;  b = 0;    break; // フェーズ1 (60 ～ 119度): 黄→緑
	case 2: r = 0;    g = 255;  b = up;   break; // フェーズ2 (120 ～ 179度): 緑→水
	case 3: r = 0;    g = down; b = 255;  break; // フェーズ3 (180 ～ 239度): 水→青
	case 4: r = up;   g = 0;    b = 255;  break; // フェーズ4 (240 ～ 299度): 青→紫
	case 5: r = 255;  g = 0;    b = down; break; // フェーズ5 (300 ～ 359度): 紫→赤
	}

	// 元のcolor配列にセットする
	color[0] = b; // 青
	color[1] = g; // 緑
	color[2] = r; // 赤
}

int centerX; // 円の中心座標X
int centerY; // 円の中心座標Y
int radius;  // 円の半径
int hue = 120; // 色相環の角度
unsigned char color[3] = { 10, 200, 0 }; // B, G, R
Uint32 lastUpdateTime = 0; // 押され始めた時間
Uint32 lastUpKeyPressTime = 0;
Uint32 lastDownKeyPressTime = 0;
Uint32 lastLeftKeyPressTime = 0;
Uint32 lastRightKeyPressTime = 0;
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
	// 現在のミリ秒を取得
	Uint32 now = SDL_GetTicks();


	// 単発押し＆ダブルタップの処理 
	switch (keyTrigger) {
	case SDLK_UP: // 上矢印キーが押されたら
		if (now - lastUpKeyPressTime < 300) {
			radius += 50; // 半径を一気に大きくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastUpKeyPressTime = 0; // 誤爆防止
		}
		else {
			radius++; // 最初に一度だけ半径を大きくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastUpKeyPressTime = now;
		}
		break;

	case SDLK_DOWN: // 下矢印キーが押されたら
		if (now - lastDownKeyPressTime < 300) {
			radius = 100; // 半径を初期値にリセット
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastDownKeyPressTime = 0; // 誤爆防止
		}
		else {
			radius--; // 最初に一度だけ半径を小さくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastDownKeyPressTime = now;
		}
		break;

	case SDLK_LEFT: // 左矢印キーが押されたら
		if (now - lastLeftKeyPressTime < 300) {
			hue += 60; // 色相環の角度を一気に大きくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastLeftKeyPressTime = 0; // 誤爆防止
		}
		else {
			hue++; // 最初に一度だけ色相環の角度を大きくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastLeftKeyPressTime = now;
		}
		break;

	case SDLK_RIGHT: // 右矢印キーが押されたら
		if (now - lastRightKeyPressTime < 300) {
			hue = 120; // 色相環の角度を初期値にリセット
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastRightKeyPressTime = 0; // 誤爆防止
		}
		else {
			hue--; // 最初に一度だけ色相環の角度を小さくする
			lastUpdateTime = now;
			isWaitingLongPress = true;
			lastRightKeyPressTime = now;
		}
		break;

	default:
		break;
	}


	// 押しっぱなしの処理
	switch (keyLevel) {
	case SDLK_UP: // 上矢印キーが押され続けたら半径を大きくし続ける
		if (isWaitingLongPress) {
			if (now - lastUpdateTime > 300) {
				radius += 4;
				lastUpdateTime = now;
				isWaitingLongPress = false;
			}
		}
		else {
			if (now - lastUpdateTime > 50) {
				radius += 4;
				lastUpdateTime = now;
			}
		}
		break;

	case SDLK_DOWN: // 下矢印キーが押され続けたら半径を小さくし続ける
		if (isWaitingLongPress) {
			if (now - lastUpdateTime > 300) {
				radius -= 4;
				lastUpdateTime = now;
				isWaitingLongPress = false;
			}
		}
		else {
			if (now - lastUpdateTime > 50) {
				radius -= 4;
				lastUpdateTime = now;
			}
		}
		break;

	case SDLK_LEFT: // 左矢印キーが押され続けたら色相環の角度を大きくし続ける
		if (isWaitingLongPress) {
			if (now - lastUpdateTime > 300) {
				hue += 4;
				lastUpdateTime = now;
				isWaitingLongPress = false;
			}
		}
		else {
			if (now - lastUpdateTime > 50) {
				hue += 4;
				lastUpdateTime = now;
			}
		}
		break;

	case SDLK_RIGHT: // 右矢印キーが押され続けたら色相環の角度を小さくし続ける
		if (isWaitingLongPress) {
			if (now - lastUpdateTime > 300) {
				hue -= 4;
				lastUpdateTime = now;
				isWaitingLongPress = false;
			}
		}
		else {
			if (now - lastUpdateTime > 50) {
				hue -= 4;
				lastUpdateTime = now;
			}
		}
		break;

	default:
		break;
	}


	// 境界値のチェック
	// 半径が0より小さくなったら、強制的に0で止める
	if (radius < 0) {
		radius = 0;
	}


	updateColorFromHue(hue, color); // 色を決める
	drawFilledCircle(buff, width, height, radius, centerX, centerY, color); // 円を描画する
}
