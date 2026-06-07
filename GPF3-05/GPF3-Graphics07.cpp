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

// キー入力のクラス化
class KeyHandler {
private:
	Uint32 lastKeyPressTime = 0;
	Uint32 lastUpdateTime = 0;
	bool isWaitingLongPress = false;

public:
	// ボタンの状態を表す「列挙型(enum)」を作っておく
	enum class State {
		NONE,           // 何もなし
		SINGLE_TAP,     // 単発押し
		DOUBLE_TAP,     // ダブルタップ
		REPEAT          // 長押し中の連続実行
	};

	// 毎フレーム呼び出して、現在のボタンの状態を返す関数
	State update(bool isTriggered, bool isPressed, Uint32 now) {
		if (isTriggered) {
			if (now - lastKeyPressTime < 300) {
				lastUpdateTime = now;
				isWaitingLongPress = true;
				lastKeyPressTime = 0; // 誤爆防止
				return State::DOUBLE_TAP;
			}
			else {
				lastUpdateTime = now;
				isWaitingLongPress = true;
				lastKeyPressTime = now;
				return State::SINGLE_TAP;
			}
		}

		if (isPressed) {
			if (isWaitingLongPress) {
				if (now - lastUpdateTime > 300) {
					lastUpdateTime = now;
					isWaitingLongPress = false;
					return State::REPEAT;
				}
			}
			else {
				if (now - lastUpdateTime > 50) {
					lastUpdateTime = now;
					return State::REPEAT;
				}
			}
		}

		return State::NONE; // 何も起きていない
	}
};

int centerX; // 円の中心座標X
int centerY; // 円の中心座標Y
int radius;  // 円の半径
int hue = 120; // 色相環の角度
unsigned char color[3] = { 10, 200, 0 }; // B, G, R
KeyHandler upKey;
KeyHandler downKey;
KeyHandler leftKey;
KeyHandler rightKey;

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

	// 上キーの処理
	switch (upKey.update(keyTrigger == SDLK_UP, keyLevel == SDLK_UP, now)) {
	case KeyHandler::State::SINGLE_TAP: radius++; break;
	case KeyHandler::State::DOUBLE_TAP: radius += 50; break;
	case KeyHandler::State::REPEAT:     radius += 4; break;
	default: break;
	}

	// 下キーの処理
	switch (downKey.update(keyTrigger == SDLK_DOWN, keyLevel == SDLK_DOWN, now)) {
	case KeyHandler::State::SINGLE_TAP: radius--; break;
	case KeyHandler::State::DOUBLE_TAP: radius = 100; break;
	case KeyHandler::State::REPEAT:     radius -= 4; break;
	default: break;
	}

	// 左キーの処理
	switch (leftKey.update(keyTrigger == SDLK_LEFT, keyLevel == SDLK_LEFT, now)) {
	case KeyHandler::State::SINGLE_TAP: hue++; break;
	case KeyHandler::State::DOUBLE_TAP: hue += 60; break;
	case KeyHandler::State::REPEAT:     hue += 4; break;
	default: break;
	}

	// 右キーの処理
	switch (rightKey.update(keyTrigger == SDLK_RIGHT, keyLevel == SDLK_RIGHT, now)) {
	case KeyHandler::State::SINGLE_TAP: hue--; break;
	case KeyHandler::State::DOUBLE_TAP: hue = 120; break;
	case KeyHandler::State::REPEAT:     hue -= 4; break;
	default: break;
	}

	// 境界値のチェック
	// 半径が0より小さくなったら、強制的に0で止める
	if (radius < 0) {
		radius = 0;
	}

	updateColorFromHue(hue, color); // 色を決める
	drawFilledCircle(buff, width, height, radius, centerX, centerY, color); // 円を描画する
}
