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

// どこかヘッダなどに

enum class LaunchPadSendMode {
    Button,
    Toggle,
    EaseShot,
    EaseLoop,
    EasePingPong,   // ★追加
    EaseAccum       // ★追加
};

struct LaunchPad {
    std::string ParName;
    float val = 0.0f;
    float lastSentVal = 0.0f;
    std::string oscAddress;

    LaunchPadSendMode sendMode = LaunchPadSendMode::Button;

    // MIDI 関連
    int ccNumber = 0;
    int lastCcValue = 0;

    // イージング用
    bool  isAnimating = false;
    float animStartTime = 0.0f;

    // 汎用イージング (EaseShot / PingPong / Accum 用)
    float animStartVal = 0.0f;
    float animEndVal = 0.0f;

    // PingPong 用 : true なら次は 0→1, false なら 1→0
    bool pingUp = true;

    // Accumulate 用 : 今何ステップ目か、最大ステップ数
    int   accStep = 0;
    int   accMaxStep = 4;  // 0→1→2→3→4 まで積んだら 4→0 へ

    bool sendOsc = true;   // false にすると sendLaunchPadOsc で無視される
    int  effectIndex = -1;    // 0〜14 = allPasses のインデックス, -1 = 非エフェクト
};
