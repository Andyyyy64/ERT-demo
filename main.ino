#include <Arduino.h>

// 電流源制御ピンの定義
const int currentSourcePins[8] = {15, 16, 17, 18, 19, 21, 22, 23}; // 8つの電極に対応

void setup() {
    Serial.begin(115200);
    // 電流源制御ピンを出力として設定
    for (int i = 0; i < 8; i++) {
        pinMode(currentSourcePins[i], OUTPUT);
        // 初期は電流を流さない
        digitalWrite(currentSourcePins[i], LOW);
    }
}

void loop() {
    for (int i = 0; i < 8; i++) {
        // 現在の電極ペアを選択
        int nextChannel = (i + 1) % 8;

        // 電流を流す電極ペアを設定
        digitalWrite(currentSourcePins[i], HIGH);
        digitalWrite(currentSourcePins[nextChannel], HIGH);

        // 電流が安定するのを待機
        delay(10);

        // 電流を停止
        digitalWrite(currentSourcePins[i], LOW);
        digitalWrite(currentSourcePins[nextChannel], LOW);

        // 次のペア設定までの待機時間
        delay(500);
    }
}

