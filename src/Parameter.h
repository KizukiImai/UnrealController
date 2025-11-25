#pragma once
float colorR = 0.0f;
float colorG = 0.0f;
float colorB = 0.0f;

struct MidiMix {
    std::string ParName;
    float val = 0.0f;          // 0〜1 に正規化した値（OSC でもこれを送る）
    float lastSentVal = 0.0f;  // 最後に OSC 送った値
    std::string oscAddress;

    int ccNumber = 0;          // このパラメータに対応する CC 番号 (1〜…)
    int lastCcValue = 0;       // 前フレームの CC 生値 (0〜127)
};

enum class LaunchPadSendMode {
    Button,     // 押してる間 1, 離したら 0
    Toggle,     // 押すたびに 0/1 トグル
    EaseShot,   // 一拍かけて 0→1 へイージング（ワンショット）
    EaseLoop    // 一拍かけて 0→1→0 へイージング（ワンショット）
};

struct LaunchPad {
    std::string ParName;
    float val = 0.0f;   // 現在の 0?1 値（OSC で送る）
    float lastSentVal = 0.0f;   // 最後に OSC 送った値
    std::string oscAddress;

    LaunchPadSendMode sendMode = LaunchPadSendMode::Button;

    // イージング状態
    bool  isAnimating = false;
    float animStartTime = 0.0f;

    // MIDI CC 関連
    int   ccNumber = 0;   // この Pad の CC 番号 (0?63)
    int   lastCcValue = 0;   // 前フレームの CC 値 (0?127)
};