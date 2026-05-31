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
Uint32 lastDownKeyPressTime = 0; // 下矢印キーが最後に押された時間
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
	// 半径を大きくする処理
	if (keyTrigger == SDLK_UP) { // 上矢印キーが押されたら(押したとき一度だけ呼び出される)
		radius++;  // 最初に一度だけ半径を大きくする
		lastUpdateTime = SDL_GetTicks(); // 押され始めた時間を取得
		isWaitingLongPress = true; // 待機モードに突入
	}
	if (keyLevel == SDLK_UP) { // 上矢印キーが押され続けたら(押し続けている限り何度でも呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 押され続けた場合の処理
		if (isWaitingLongPress) { // 待機モード
			if (now - lastUpdateTime > 500) { // 500ミリ秒(0.5秒)経過したら
				radius += 4; // 半径を大きくし始める
				lastUpdateTime = now; // 連続モードに移行し始めた時刻を記録
				isWaitingLongPress = false; // 待機モードを解除し連続モードに移行
			}
		}
		else { // 連続モード
			if (now - lastUpdateTime > 50) { // 50ミリ秒経過したら
				radius += 4; // 半径を大きくし続ける
				lastUpdateTime = now; // 半径を大きくするたびに時間をリセット
			}
		}
	}
	
	// 半径を小さくする処理（＆ダブルタップで最初の状態にリセット）
	if (keyTrigger == SDLK_DOWN) { // 下矢印キーが押されたら(押したとき一度だけ呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 前回押された時から0.3秒以内にもう一度押された場合
		if (now - lastDownKeyPressTime < 300) {
			radius = 100;
			hue = 120;

			lastUpdateTime = now; // 押され始めた時間を取得（長押し用）
			isWaitingLongPress = true; // 待機モードに突入
			lastDownKeyPressTime = 0; // 3回連続で押したときの誤爆を防ぐために一回目に押した時間をリセット
		}
		// 普通に押された場合
		else {
			radius--; // 最初に一度だけ半径を小さくする
			lastUpdateTime = now; // 押され始めた時間を取得（長押し用）
			isWaitingLongPress = true; // 待機モードに突入

			lastDownKeyPressTime = now; // 一回目に押した時間を取得（ダブルクリック用）
		}
	}
	if (keyLevel == SDLK_DOWN) { // 下矢印キーが押され続けたら(押し続けている限り何度でも呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 押され続けた場合の処理
		if (isWaitingLongPress) { // 待機モード
			if (now - lastUpdateTime > 500) { // 500ミリ秒(0.5秒)経過したら
				radius -= 4; // 半径を小さくし始める
				lastUpdateTime = now; // 連続モードに移行し始めた時刻を記録
				isWaitingLongPress = false; // 待機モードを解除し連続モードに移行
			}
		}
		else { // 連続モード
			if (now - lastUpdateTime > 50) { // 50ミリ秒経過したら
				radius -= 4; // 半径を小さくし続ける
				lastUpdateTime = now; // 半径を小さくするたびに時間をリセット
			}
		}
	}

	// 色相環の角度を上げる処理
	if (keyTrigger == SDLK_LEFT) { // 左矢印キーが押されたら(押したとき一度だけ呼び出される)
		hue++; // 最初に一度だけ角度を上げる
		lastUpdateTime = SDL_GetTicks(); // 押され始めた時間を取得
		isWaitingLongPress = true; // 待機モードに突入
	}
	if (keyLevel == SDLK_LEFT) { // 左矢印キーが押され続けたら(押し続けている限り何度でも呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 押され続けた場合の処理
		if (isWaitingLongPress) { // 待機モード
			if (now - lastUpdateTime > 500) { // 500ミリ秒(0.5秒)経過したら
				hue += 4; // 角度を上げ始める
				lastUpdateTime = now; // 連続モードに移行し始めた時刻を記録
				isWaitingLongPress = false; // 待機モードを解除し連続モードに移行
			}
		}
		else { // 連続モード
			if (now - lastUpdateTime > 50) { // 50ミリ秒経過したら
				hue += 4; // 角度を上げ続ける
				lastUpdateTime = now; // 角度を上げるたびに時間をリセット
			}
		}
	}

	// 半径が0より小さくなったら、強制的に0で止める
	if (radius < 0) {
		radius = 0;
	}

	// 色相環の角度を下げる処理
	if (keyTrigger == SDLK_RIGHT) { // 右矢印キーが押されたら(押したとき一度だけ呼び出される)
		hue--; // 最初に一度だけ角度を下げる
		lastUpdateTime = SDL_GetTicks(); // 押され始めた時間を取得
		isWaitingLongPress = true; // 待機モードに突入
	}
	if (keyLevel == SDLK_RIGHT) { // 右矢印キーが押され続けたら(押し続けている限り何度でも呼び出される)
		Uint32 now = SDL_GetTicks(); // 現在のミリ秒を取得
		// 押され続けた場合の処理
		if (isWaitingLongPress) { // 待機モード
			if (now - lastUpdateTime > 500) { // 500ミリ秒(0.5秒)経過したら
				hue -= 4; // 角度を下げ始める
				lastUpdateTime = now; // 連続モードに移行し始めた時刻を記録
				isWaitingLongPress = false; // 待機モードを解除し連続モードに移行
			}
		}
		else { // 連続モード
			if (now - lastUpdateTime > 50) { // 50ミリ秒経過したら
				hue -= 4; // 角度を下げ続ける
				lastUpdateTime = now; // 角度を下げるたびに時間をリセット
			}
		}
	}

	updateColorFromHue(hue, color); // 色を決める
	drawFilledCircle(buff, width, height, radius, centerX, centerY, color); // 円を描画する
}
