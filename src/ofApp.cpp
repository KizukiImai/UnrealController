#include "ofApp.h"

//--------------------------------------------------------------

namespace {
    // 0〜1 → 0〜1 のイージング（イーズインアウト）
    float easeInOutQuad(float t) {
        t = ofClamp(t, 0.0f, 1.0f);
        return (t < 0.5f)
            ? 2.0f * t * t
            : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    }
}


void ofApp::drawImGui(){

	gui.begin();

    if (ImGui::Begin("Global")) {
        ImGui::Text("Tap BPM with SPACE");
        ImGui::SliderFloat("BPM", &bpm, 40.0f, 200.0f);
    }
    ImGui::End();

    drawSequencerUI();
    drawMidiMix();
	drawLaunchPad();
	drawDebugTexture();
	gui.end();

}

void ofApp::setupMidi() {
	midimix.listPorts();
	//launchpad.listPorts();
    midimix.setup(3);
	launchpad.setup(1);

}

void ofApp::updateMidi() {
	midimix.update();
	launchpad.update();
	updateMidiMixFromMidi();   // ★ Midimix のノブ/フェーダーの状態を反映
    updateLaunchPadFromMidi();   // ★ まず実機の状態を反映
    updateLaunchPadEasing();     // ★ EaseShot/EaseLoop のアニメを進める
}

void ofApp::updateMidiMixFromMidi() {
    for (int i = 0; i < NUM_PARAMS; ++i) { // NUM_PARAMS = NUM_KNOBS + NUM_FADERS
        auto& p = midimixarray[i];

        int cc = midimix.getCCValue(2,p.ccNumber); // 0〜127 が返ってくる想定

        // 値が変わっていなければ何もしない（好みで省略してもOK）
        if (cc == p.lastCcValue) continue;

        // 0〜127 → 0.0〜1.0 に正規化
        p.val = ofMap(cc, 0, 127, 0.0f, 1.0f, true);

        p.lastCcValue = cc;
    }
}


void ofApp::initMidiMixArray() {
    int ccBase = 1; // Knob1 → CC1

    // ノブ 24 個 (3x8)
    for (int i = 0; i < NUM_KNOBS; ++i) {
        midimixarray[i].ParName = "Knob" + std::to_string(i + 1);
        midimixarray[i].val = 0.0f;
        midimixarray[i].lastSentVal = midimixarray[i].val;
        midimixarray[i].oscAddress = "/midimix/knob/" + std::to_string(i + 1);

        midimixarray[i].ccNumber = ccBase + i; // CC1〜24
        midimixarray[i].lastCcValue = 0;
    }

    // フェーダー 8 本
    for (int i = 0; i < NUM_FADERS; ++i) {
        int idx = NUM_KNOBS + i;
        midimixarray[idx].ParName = "Fader" + std::to_string(i + 1);
        midimixarray[idx].val = 0.0f;
        midimixarray[idx].lastSentVal = midimixarray[idx].val;
        midimixarray[idx].oscAddress = "/midimix/fader/" + std::to_string(i + 1);

        midimixarray[idx].ccNumber = ccBase + NUM_KNOBS + i; // CC25〜32
        midimixarray[idx].lastCcValue = 0;
    }
}

void ofApp::setupPostEffects() {
    bleachPass = postprocess.createPass<BleachBypassPass>();   // コントラスト強＋色味変化
    bloomPass = postprocess.createPass<BloomPass>();          // 発光
    contrastPass = postprocess.createPass<ContrastPass>();       // コントラスト強調
    dofAltPass = postprocess.createPass<DofAltPass>();         // 被写界深度（別実装）
    dofPass = postprocess.createPass<DofPass>();            // 被写界深度
    edgePass = postprocess.createPass<EdgePass>();           // エッジ抽出
    fakeSSSPass = postprocess.createPass<FakeSSSPass>();        // SSS っぽいにじみ
    godRaysPass = postprocess.createPass<GodRaysPass>();        // ゴッドレイ
    kaleidoPass = postprocess.createPass<KaleidoscopePass>();   // 万華鏡
    //limbDarkPass = postprocess.createPass<LimbDarkeningPass>();  // 周辺減光
    noiseWarpPass = postprocess.createPass<NoiseWarpPass>();      // ノイズ歪み
    pixelatePass = postprocess.createPass<PixelatePass>();       // ピクセレート
    rgbShiftPass = postprocess.createPass<RGBShiftPass>();       // RGB シフト
    ssaoPass = postprocess.createPass<SSAOPass>();           // SSAO
    toonPass = postprocess.createPass<ToonPass>();           // トーンシェーディング
    zoomBlurPass = postprocess.createPass<ZoomBlurPass>();       // ズームブラー

    //// ─────────────────────────────
    //// LaunchPad 右上に割り当てるための配列
    //// （順番は好きに変えてOK）
    //// ─────────────────────────────
    allPasses = {
        std::static_pointer_cast<RenderPass>(bloomPass),
        std::static_pointer_cast<RenderPass>(pixelatePass),
        std::static_pointer_cast<RenderPass>(rgbShiftPass),
        std::static_pointer_cast<RenderPass>(kaleidoPass),
        std::static_pointer_cast<RenderPass>(zoomBlurPass),

        std::static_pointer_cast<RenderPass>(godRaysPass),
        std::static_pointer_cast<RenderPass>(edgePass),
        std::static_pointer_cast<RenderPass>(toonPass),
        std::static_pointer_cast<RenderPass>(noiseWarpPass),
        std::static_pointer_cast<RenderPass>(fakeSSSPass),

        std::static_pointer_cast<RenderPass>(bleachPass),
        std::static_pointer_cast<RenderPass>(contrastPass),
        std::static_pointer_cast<RenderPass>(dofPass),
        std::static_pointer_cast<RenderPass>(dofAltPass),
        std::static_pointer_cast<RenderPass>(ssaoPass)
    };

    //// 全部 OFF からスタート
    for (auto& p : allPasses) {
        if (p) p->setEnabled(false);
    }
}

void ofApp::updateLaunchPadFromMidi() {
    float now = ofGetElapsedTimef();

    for (int i = 0; i < LP_PADS; ++i) {
        LaunchPad& p = launchPads[i];

        int  cc = launchpad.getCCValue(1, p.ccNumber); // 0〜127
        bool down = (cc > 0);
        bool wasDown = (p.lastCcValue > 0);
        bool pressed = down && !wasDown;
        bool released = !down && wasDown;

        if (p.effectIndex >= 0) {
            // 見た目用：押している間だけ赤くしたいので val を 0/1 に
            p.val = down ? 1.0f : 0.0f;

            // 対応するポストプロセスを ON/OFF
            if (p.effectIndex < (int)allPasses.size()) {
                auto& pass = allPasses[p.effectIndex];
                if (pass) {
                    pass->setEnabled(down);  // 押している間だけ有効
                }
            }

            p.lastCcValue = cc;
            continue;   // 他のモード処理はスキップ
        }

        switch (p.sendMode) {
        case LaunchPadSendMode::Button:
            // 押している間だけ 1, 離したら 0
            p.isAnimating = false;
            p.val = down ? 1.0f : 0.0f;
            break;

        case LaunchPadSendMode::Toggle:
            // 押された瞬間に 0/1 切り替え（イージングなし）
            if (pressed) {
                p.isAnimating = false;
                p.val = (p.val > 0.5f) ? 0.0f : 1.0f;
            }
            break;

        case LaunchPadSendMode::EaseShot:
            // 毎回 0 → 1 に 1 拍かけてイージング
            if (pressed) {
                p.animStartVal = 0.0f;
                p.animEndVal = 1.0f;
                p.animStartTime = now;
                p.isAnimating = true;
            }
            break;

        case LaunchPadSendMode::EaseLoop:
            // 押された瞬間に 0 → 1 → 0 を 1 拍で
            if (pressed) {
                p.animStartTime = now;
                p.isAnimating = true;
                p.val = 0.0f;
            }
            break;

        case LaunchPadSendMode::EasePingPong:
            // 押すごとに 0→1 / 1→0 を交互に
            if (pressed) {
                float from = (p.pingUp ? 0.0f : 1.0f);
                float to = (p.pingUp ? 1.0f : 0.0f);

                p.animStartVal = from;
                p.animEndVal = to;
                p.animStartTime = now;
                p.isAnimating = true;

                p.pingUp = !p.pingUp; // 次の方向を反転
            }
            break;

        case LaunchPadSendMode::EaseAccum:
            // 押すごとに
            //   0→1, 1→2, 2→3 ... accMaxStep まで
            //   accMaxStep まで行ったら max→0 に戻る
            if (pressed && !p.isAnimating) {
                if (p.accStep >= p.accMaxStep) {
                    // maxStep → 0 に戻す
                    p.animStartVal = (float)p.accMaxStep;
                    p.animEndVal = 0.0f;
                    p.accStep = 0; // 0 にリセット
                }
                else {
                    // accStep → accStep+1 に上げる
                    p.animStartVal = (float)p.accStep;
                    p.animEndVal = (float)(p.accStep + 1);
                    p.accStep++;
                }
                p.animStartTime = now;
                p.isAnimating = true;
            }
            break;
        }

        p.lastCcValue = cc;
    }
}

void ofApp::setupSequencer()
{
    seqStartTime = ofGetElapsedTimef();
    seqCurStep = 0;
    seqLastStep = -1;
    seqStepPhase = 0.0f;
    bSeqPlay = true;


    for (int r = 0; r < SEQ_ROWS; ++r) {
        seqRowAddress[r] = "/seq/" + std::to_string(r);

        // ★ 追加：初期値を必ず代入（未初期化を潰す）
        seqRowEnabled[r] = true;   // ← デフォルトONにしたいなら true（OFF開始なら false）
        seqRowMode[r] = 0;         // 0=Button, 1=EaseShot, 2=EaseLoop

        seqRowAnimating[r] = false;
        seqRowAnimStart[r] = 0.0f;
        seqRowLastSent[r] = 9999.0f;

        for (int s = 0; s < SEQ_STEPS; ++s) seqGrid[r][s] = false;
    }
}

void ofApp::sendSeqOsc(int row, float value)
{
    if (row < 0 || row >= SEQ_ROWS) return;

    ofxOscMessage m;
    m.setAddress(seqRowAddress[row]);
    m.addFloatArg(value);
    oscsender.sendMessage(m, false);

    // 既存のログ表示に乗せる
    logOsc(seqRowAddress[row], value);
}

void ofApp::updateSequencer()
{
    if (!bSeqPlay) return;

    float now = ofGetElapsedTimef();

    // 1 step = 8分音符 = 1拍の半分
    float stepDur = (60.0f / bpm) * 0.5f;
    if (stepDur <= 0.0f) return;

    float barDur = stepDur * (float)SEQ_STEPS;

    float elapsed = now - seqStartTime;
    if (elapsed < 0.0f) {
        seqStartTime = now;
        elapsed = 0.0f;
    }

    float barPos = fmodf(elapsed, barDur); // 0..barDur
    int step = (int)floorf(barPos / stepDur);
    if (step < 0) step = 0;
    if (step >= SEQ_STEPS) step = SEQ_STEPS - 1;

    float stepPos = barPos - (float)step * stepDur; // 0..stepDur
    float phase = stepPos / stepDur;                // 0..1

    seqCurStep = step;
    seqStepPhase = phase;

    // そのステップの開始時刻（now基準で逆算）
    float stepStartTime = now - phase * stepDur;

    // --- stepが進んだ瞬間の処理（トリガ） ---
    if (step != seqLastStep) {
        seqLastStep = step;

        for (int r = 0; r < SEQ_ROWS; ++r) {
            if (!seqRowEnabled[r]) {
                seqRowAnimating[r] = false;
                continue;
            }

            if (seqGrid[r][step]) {
                int mode = seqRowMode[r];

                if (mode == 0) {
                    // Button: タイミングが来たら 1 を一回送る
                    sendSeqOsc(r, 1.0f);
                }
                else {
                    // EaseShot / EaseLoop: そのstepDurの間アニメ送信
                    seqRowAnimating[r] = true;
                    seqRowAnimStart[r] = stepStartTime;
                    seqRowLastSent[r] = 9999.0f; // 強制的に最初の値を送る
                }
            }
            else {
                // このステップがOFFなら、その行のアニメは止める（次のONで再開）
                seqRowAnimating[r] = false;
            }
        }
    }

    // --- Ease系の連続送信 ---
    const float EPS = 0.0005f;

    for (int r = 0; r < SEQ_ROWS; ++r) {
        if (!seqRowEnabled[r]) continue;
        if (!seqRowAnimating[r]) continue;

        int mode = seqRowMode[r];
        if (mode == 0) continue; // Buttonは連続送信しない

        float t = (now - seqRowAnimStart[r]) / stepDur; // 0..1
        if (t >= 1.0f) {
            // step終端：戻しを送って終了（扱いやすいように0へ戻す）
            sendSeqOsc(r, 0.0f);
            seqRowAnimating[r] = false;
            continue;
        }

        t = ofClamp(t, 0.0f, 1.0f);

        float v = 0.0f;

        if (mode == 1) {
            // EaseShot: 0 -> 1
            v = easeInOutQuad(t);
        }
        else if (mode == 2) {
            // EaseLoop: 0 -> 1 -> 0
            if (t <= 0.5f) {
                float u = t / 0.5f; // 0..1
                v = easeInOutQuad(u);
            }
            else {
                float u = (1.0f - t) / 0.5f; // 0..1
                v = easeInOutQuad(u);
            }
        }

        if (fabsf(v - seqRowLastSent[r]) > EPS) {
            sendSeqOsc(r, v);
            seqRowLastSent[r] = v;
        }
    }
}

void ofApp::drawSequencerUI()
{
    static const char* modeItems[] = { "Button", "EaseShot", "EaseLoop" };

    if (ImGui::Begin("Sequencer")) {
        ImGui::Text("1 bar = 8 steps (8th notes) / rows = 6");
        ImGui::Checkbox("Play", &bSeqPlay);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            seqStartTime = ofGetElapsedTimef();
            seqLastStep = -1;
        }

        float stepDur = (60.0f / bpm) * 0.5f;
        ImGui::Text("BPM %.1f  StepDur %.3fs  CurStep %d", bpm, stepDur, seqCurStep);

        ImVec2 cellSize(22, 22);

        for (int r = 0; r < SEQ_ROWS; ++r) {
            ImGui::PushID(r);

            ImGui::Checkbox("##rowOn", &seqRowEnabled[r]);
            ImGui::SameLine();
            ImGui::Text("Row %d", r + 1);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(110);
            ImGui::Combo("##mode", &seqRowMode[r], modeItems, IM_ARRAYSIZE(modeItems));
            ImGui::SameLine();
            ImGui::Text("%s", seqRowAddress[r].c_str());

            // steps
            for (int s = 0; s < SEQ_STEPS; ++s) {
                ImGui::PushID(s);

                bool on = seqGrid[r][s];
                bool playhead = (bSeqPlay && (s == seqCurStep));

                // 色（見やすさ用）
                ImVec4 colOff(0.18f, 0.18f, 0.18f, 1.0f);
                ImVec4 colOn(0.90f, 0.20f, 0.20f, 1.0f);
                ImVec4 colPh(0.20f, 0.60f, 0.90f, 1.0f); // playhead

                ImVec4 base = on ? colOn : colOff;
                if (playhead) {
                    // playheadは少し青みを足す
                    base.x = (base.x + colPh.x) * 0.5f;
                    base.y = (base.y + colPh.y) * 0.5f;
                    base.z = (base.z + colPh.z) * 0.5f;
                }

                ImGui::PushStyleColor(ImGuiCol_Button, base);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, base);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, base);

                if (ImGui::Button("##cell", cellSize)) {
                    seqGrid[r][s] = !seqGrid[r][s];
                }

                ImGui::PopStyleColor(3);

                if (s < SEQ_STEPS - 1) ImGui::SameLine();
                ImGui::PopID();
            }

            ImGui::PopID();
        }
    }
    ImGui::End();
}


void ofApp::logOsc(const std::string& addr, float value)
{
    // "address : 0.123" のような文字列に
    std::string line = addr + " : " + ofToString(value, 3);

    oscLog.push_back(line);
    if ((int)oscLog.size() > maxOscLogLines) {
        oscLog.erase(oscLog.begin());  // 古いものから消す
    }
}


void ofApp::initLaunchPad() {
    for (int i = 0; i < LP_PADS; ++i) {
        int col = i % LP_COLS;   // 0〜7
        int row = i / LP_COLS;   // 0〜7

        LaunchPad& p = launchPads[i];

        p.ParName = "Pad" + std::to_string(i + 1);
        p.val = 0.0f;
        p.lastSentVal = p.val;

        p.oscAddress = "/launchpad/" + std::to_string(row) + "/" + std::to_string(col);

        p.ccNumber = i;
        p.lastCcValue = 0;

        // 8×8 を 4 象限に分割
        bool upper = (row < LP_ROWS / 2); // 0〜3
        bool left = (col < LP_COLS / 2); // 0〜3

        if (upper && left) {
            // 左上 4×4 をさらに列で分割
            if (col < 2) {
                // col 0,1 → Toggle
                p.sendMode = LaunchPadSendMode::Toggle;
            }
            else if (col == 2) {
                // col 2 → PingPong
                p.sendMode = LaunchPadSendMode::EasePingPong;
            }
            else { // col == 3
                // col 3 → Accum
                p.sendMode = LaunchPadSendMode::EaseAccum;
            }
        }
        else if (upper && !left) {
            // 右上：EaseShot
            p.sendMode = LaunchPadSendMode::EaseShot;
        }
        else if (!upper && left) {
            // 左下：EaseLoop
            p.sendMode = LaunchPadSendMode::EaseLoop;
        }
        else {
            // 右下：Button
            p.sendMode = LaunchPadSendMode::Button;
        }

        // イージングの初期化
        p.isAnimating = false;
        p.animStartTime = 0.0f;
        p.animStartVal = 0.0f;
        p.animEndVal = 0.0f;

        p.pingUp = true;

        p.accStep = 0;
        p.accMaxStep = 4; // 必要に応じて変えてOK
    }

    int fxIdx = 0;
    for (int row = 0; row < LP_ROWS / 2; ++row) {          // 0〜3
        for (int col = LP_COLS / 2; col < LP_COLS; ++col) { // 4〜7
            int i = row * LP_COLS + col;   // 右上ブロックのインデックス

            if (i == 7) {
                // ★ Pad index 7 は「今のまま保持」したいのでスキップ
                continue;
            }

            if (fxIdx >= (int)allPasses.size()) {
                // allPasses が 15 個あればここには来ない想定
                break;
            }

            LaunchPad& p = launchPads[i];

            // エフェクト専用ボタン：押している間だけ ON
            p.sendMode = LaunchPadSendMode::Button;
            p.sendOsc = false;           // OSC は送らない
            p.effectIndex = fxIdx;

            // UI 上でわかりやすいように名前を変えてもよい
            p.ParName = "FX " + std::to_string(fxIdx + 1);

            ++fxIdx;
        }
    }
}


void ofApp::updateLaunchPadEasing() {
    float beatDuration = 60.0f / bpm;  // 1 拍の長さ [秒]
    float now = ofGetElapsedTimef();

    for (int i = 0; i < LP_PADS; ++i) {
        LaunchPad& p = launchPads[i];
        if (!p.isAnimating) continue;

        float t = (now - p.animStartTime) / beatDuration; // 経過拍数(0〜1)

        switch (p.sendMode) {
        case LaunchPadSendMode::EaseLoop: {
            // 0〜0.5 拍 : 0→1
            // 0.5〜1 拍 : 1→0
            if (t >= 1.0f) {
                p.val = 0.0f;
                p.isAnimating = false;
            }
            else {
                float phase = ofClamp(t, 0.0f, 1.0f);
                if (phase <= 0.5f) {
                    float u = phase / 0.5f;  // 0〜1
                    p.val = easeInOutQuad(u);
                }
                else {
                    float u = (1.0f - phase) / 0.5f; // 0〜1
                    p.val = easeInOutQuad(u);
                }
            }
            break;
        }

        case LaunchPadSendMode::EaseShot:
        case LaunchPadSendMode::EasePingPong:
        case LaunchPadSendMode::EaseAccum: {
            if (t >= 1.0f) {
                p.val = p.animEndVal;
                p.isAnimating = false;
            }
            else {
                float e = easeInOutQuad(ofClamp(t, 0.0f, 1.0f));
                p.val = ofLerp(p.animStartVal, p.animEndVal, e);
            }
            break;
        }

        default:
            // 他モードはここでアニメしない
            p.isAnimating = false;
            break;
        }
    }
}


void ofApp::drawMidiMix() {
    if (ImGui::Begin("MIDIMIX")) {
        for (int col = 0; col < 8; ++col) {
            ImGui::BeginGroup();
            for (int row = 0; row < 3; ++row) {
                int i = row * 8 + col; // 0〜
                ImGui::PushID(i);
                ImGuiKnobs::Knob("##knob",
                    &midimixarray[i].val,
                    0.0f, 1.0f, 0.01f, "%.2f",
                    ImGuiKnobVariant_Wiper);
                ImGui::Text("%s", midimixarray[i].ParName.c_str());
                ImGui::PopID();
            }
            ImGui::EndGroup();
            if (col < 7) ImGui::SameLine();
        }

        ImGui::Separator();
        ImVec2 sliderSize(52, 120);
        for (int col = 0; col < 8; ++col) {
            int idx = NUM_KNOBS + col; // 24〜31
            ImGui::PushID(idx);
            ImGui::BeginGroup();
            ImGui::VSliderFloat("##fader", sliderSize,
                &midimixarray[idx].val, 0.0f, 1.0f, "");
            ImGui::Text("%s", midimixarray[idx].ParName.c_str());
            ImGui::EndGroup();
            ImGui::PopID();
            if (col < 7) ImGui::SameLine();
        }
    }
    ImGui::End();
}

void ofApp::drawLaunchPad() {
    if (ImGui::Begin("LaunchPad")) {

        ImVec2 padSize(32, 32);

        for (int col = 0; col < LP_COLS; ++col) {
            ImGui::BeginGroup();
            for (int row = 0; row < LP_ROWS; ++row) {
                int i = row * LP_COLS + col;
                LaunchPad& p = launchPads[i];

                ImGui::PushID(i);

                ImVec4 offCol(0.2f, 0.2f, 0.2f, 1.0f);
                ImVec4 onCol(0.9f, 0.1f, 0.1f, 1.0f);

                float f = 0.0f;

                switch (p.sendMode) {
                case LaunchPadSendMode::Button:
                    f = p.val; // 0 or 1
                    break;

                case LaunchPadSendMode::Toggle:
                    f = p.val; // 0 or 1 (ラッチ)
                    break;

                case LaunchPadSendMode::EaseShot:
                case LaunchPadSendMode::EaseLoop:
                case LaunchPadSendMode::EasePingPong:
                case LaunchPadSendMode::EaseAccum:
                    if (p.isAnimating) {
                        // 0〜1 の部分だけ明るさに使う
                        float frac = p.val - std::floor(p.val);
                        f = ofClamp(frac, 0.0f, 1.0f);
                    }
                    else {
                        f = 0.0f;
                    }
                    break;
                }

                ImVec4 colMix(
                    offCol.x + (onCol.x - offCol.x) * f,
                    offCol.y + (onCol.y - offCol.y) * f,
                    offCol.z + (onCol.z - offCol.z) * f,
                    1.0f
                );

                ImGui::PushStyleColor(ImGuiCol_Button, colMix);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colMix);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, colMix);

                ImGui::Button("##pad", padSize);

                ImGui::PopStyleColor(3);

                ImGui::Text("%s", p.ParName.c_str());
                ImGui::PopID();
            }
            ImGui::EndGroup();
            if (col < LP_COLS - 1) ImGui::SameLine();
        }
    }
    ImGui::End();
}


void ofApp::drawDebugTexture() {

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_Always);

    if (ImGui::Begin("Post Preview",
        nullptr,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar))
    {
        const ofTexture& tex = finalFbo.getTexture();  // ★ postprocess ではなく finalFbo

        ImGui::Image(
            (void*)(uintptr_t)tex.getTextureData().textureID,
            ImVec2(960, 540),
            ImVec2(1, 1),  // ← 左右反転したいなら ここ (U:1→0, V:0→1)
            ImVec2(0, 0)
        );
    }
    ImGui::End();

    ImGui::PopStyleVar();
}




void ofApp::setupOsc(){
	// Setup OSC here if 
	ipAddress = xml.getValue("settings:OSC:ip", "localhost");
	port = xml.getValue("settings:OSC:port", 9000);
    oscsender.setup(ipAddress, port);
}

void ofApp::setupSpout() {
    // Spout 用テクスチャだけ GL_TEXTURE_2D で作りたい
    bool wasArb = ofGetUsingArbTex();   // 現在の設定を保存

    ofDisableArbTex();                  // この間に作るテクスチャは GL_TEXTURE_2D
    spoutSenderName = xml.getValue("settings:spout:name", "ofxSpoutSender");
    spouttexture.allocate(1280, 720, GL_RGBA8);
    spout.setupAuto(spoutSenderName, spouttexture, true);

    // ★ ofxPostProcessing は ARB(GL_TEXTURE_RECTANGLE) 前提なので元に戻す
    if (wasArb) {
        ofEnableArbTex();
    }
}

void ofApp::drawMask(float y, float h)
{
    if (!maskVideo.isLoaded()) return;
    if (h <= 0) return;

    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    ofSetColor(255);  // 動画の輝度そのままを使う

    // 中央エリアと同じ範囲にマスクをかける
    maskVideo.getTexture().draw(0, y, OutWidth, h);

    ofDisableBlendMode();
}


void ofApp::drawLogo(float bandY, float bandH)
{
    if (!logoImage.isAllocated()) {
        // デバッグ用：ファイルが無いときはプレースホルダ描画
        ofPushStyle();
        ofSetColor(255, 0, 0);
        ofDrawBitmapString("NO logo.png", 20, bandY + 20);
        ofPopStyle();
        return;
    }

    // Midimix Fader2 の値でアルファ制御
    int fader2Index = NUM_KNOBS + 1; // Fader2
    float a = 1.0f;
    if (fader2Index >= 0 && fader2Index < NUM_PARAMS) {
        a = ofClamp(midimixarray[fader2Index].val, 0.0f, 1.0f);
    }

    // アルファが 0 なら実質見えないけど、一応処理はそのまま
    ofEnableAlphaBlending();
    ofSetColor(255, 255, 255, (int)(a * 255));

    // ★ 画面全体を「覆う」スケール（cover）
    float texW = logoImage.getWidth();
    float texH = logoImage.getHeight();

    float sx = (float)OutWidth / texW;
    float sy = (float)OutHeight / texH;
    float scale = std::max(sx, sy);   // cover: どちらか大きい方を採用

    float w = texW * scale;
    float h = texH * scale;

    // 画面中心に配置（はみ出した分は外に出る）
    float x = (OutWidth - w) * 0.5f;
    float y = (OutHeight - h) * 0.5f;

    logoImage.draw(x, y, w, h);

    ofDisableAlphaBlending();
}




void ofApp::Crop()
{
    if (!finalFbo.isAllocated()) {
        bool wasArb = ofGetUsingArbTex();
        ofDisableArbTex();
        finalFbo.allocate(OutWidth, OutHeight, GL_RGBA);
        if (wasArb) ofEnableArbTex();
    }

    const ofTexture& baseTex = postprocess.getProcessedTextureReference();

    // 上下帯の高さ（Crop 有効時のみ）
    float topH = bEnableCrop ? OutHeight * cropTopRatio : 0.0f;
    float bottomH = bEnableCrop ? OutHeight * cropBottomRatio : 0.0f;

    // マスクをかけるメインエリア（上下帯を除いた部分）
    float mainY = topH;
    float mainH = OutHeight - topH - bottomH;
    if (mainH < 0) mainH = 0;

    finalFbo.begin();
    ofClear(0, 0, 0, 255);

    // ① ベース画像（Spout＋ポストエフェクト）をフルで描画
    ofSetColor(255);
    baseTex.draw(0, 0, OutWidth, OutHeight);

    // ② メインエリアにだけ Mask を掛ける
    if (bEnableMask && mainH > 0) {
        drawMask(mainY, mainH);
    }

    // ③ 上帯：用意した横長画像を描画（Crop ON のとき）
    if (bEnableCrop && topH > 0) {
        if (topBandImage.isAllocated()) {
            // 画像を上帯いっぱいにフィットさせる（16:9 以外でもOKなようにスケール）
            ofSetColor(255); // フルカラー
            topBandImage.draw(0, 0, OutWidth, topH);   // ← ここで帯の中に拡大縮小
        }
        else {
            // 画像が無い場合は黒帯でフォールバック
            ofPushStyle();
            ofSetColor(0, 0, 0, 220);
            ofDrawRectangle(0, 0, OutWidth, topH);
            ofPopStyle();
        }
    }

    // ④ 下帯：黒帯＋OSCログ（Crop ON のとき）
    if (bEnableCrop && bottomH > 0) {
        // 黒帯背景
        ofPushStyle();
        ofSetColor(0, 0, 0, 220);
        ofDrawRectangle(0, OutHeight - bottomH, OutWidth, bottomH);
        ofPopStyle();

        // 中にログ描画
        drawOscStatusStrip(OutHeight - bottomH, bottomH);
    }

    // ⑤ ロゴは「全画面オーバーレイ」として最後に描画（Fader2 でフェード）
    if (bEnableLogo) {
        // すでに作ってあるフルスクリーン版 drawLogo を呼ぶ
        drawLogo(0.0f, (float)OutHeight);
    }

    finalFbo.end();
}



void ofApp::drawOscStatusStrip(float bandY, float bandH)
{
    if (bandH <= 0) return;

    ofPushStyle();

    float margin = 10.0f;
    float x0 = margin;
    float y0 = bandY + margin;
    float w = OutWidth - margin * 2.0f;
    float h = bandH - margin * 2.0f;
    if (h <= 0) {
        ofPopStyle();
        return;
    }

    // 背景（黒帯）
    ofSetColor(0, 0, 0, 220);
    ofDrawRectangle(x0, y0, w, h);

    // テキスト色
    ofSetColor(255);

    float lineH = 14.0f;
    int maxLines = (int)(h / lineH);
    if (maxLines <= 0) {
        ofPopStyle();
        return;
    }

    int startIdx = std::max(0, (int)oscLog.size() - maxLines);

    float y = y0 + lineH;
    for (int i = startIdx; i < oscLog.size(); ++i) {
        ofDrawBitmapString(oscLog[i], x0 + 5, y);
        y += lineH;
    }

    ofPopStyle();
}




void ofApp::setup(){
    ofSetFrameRate(60);
    ofSetVerticalSync(false);
    xml.loadFile("config.xml");
	setupSequencer();


    bEnableLogo = true;          // とりあえず起動時は ON にしてデバッグしやすく
    bEnableMask = true;          // 同上（邪魔ならあとで false に戻してOK）
    bEnableCrop = true;          // 帯がちゃんと見えるか確認するため一旦 ON

    cropTopRatio = 0.2f;      // 画面上 20%
    cropBottomRatio = 0.2f;      // 画面下 20%


    initMidiMixArray();
    setupMidi();
    setupOsc();
    setupSpout();

    OutWidth = xml.getValue("settings:outputResolution:width", 1280);
    OutHeight = xml.getValue("settings:outputResolution:height", 720);

    postprocess.init(OutWidth, OutHeight);
    setupPostEffects();      // ★ ここで allPasses が埋まる

    {
        bool wasArb = ofGetUsingArbTex();
        ofDisableArbTex();
        finalFbo.allocate(OutWidth, OutHeight, GL_RGBA);
        if (wasArb) ofEnableArbTex();
    }

    // ====== 追加: ロゴ画像 & マスク動画の読み込み ======
    // パスは好きなものに変えてOK
    logoImage.load("logo.png");       // data/logo.png を想定
    maskVideo.load("mask.mp4");       // data/mask.mp4 を想定（グレースケール）
    maskVideo.setLoopState(OF_LOOP_NORMAL);
    maskVideo.play();

    topBandImage.load("top_band.png");

    initLaunchPad();         // ★ それから effectIndex を割り当てる

    ofxImGui::BaseTheme* theme = nullptr;
    bool autoDraw = true;
    ImGuiConfigFlags flags = ImGuiConfigFlags_None;
    bool restoreGuiState = true;
    bool showImGuiMouseCursor = false;

    gui.setup(theme, autoDraw, flags, restoreGuiState, showImGuiMouseCursor);


}

//--------------------------------------------------------------
void ofApp::update() {
    updateMidi();
	updateSequencer();

    sendOscChangedParamsOsc();
    sendLaunchPadOsc();

    if (maskVideo.isLoaded()) {
        maskVideo.update();
    }
}




//--------------------------------------------------------------
void ofApp::draw() {
    postprocess.begin();
    ofClear(0, 0, 0, 255);                         // ★一度クリアしておくと安心
    spouttexture.draw(0, 0, OutWidth, OutHeight);  // ★FBOと同じサイズで描画
    postprocess.end(false);

    Crop();

    // ③ ImGui（中で finalFbo をプレビュー表示）
    drawImGui();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

    // スペースキーでタップテンポ
    if (key == ' ') {  // or: if (key == OF_KEY_SPACE)
        float now = ofGetElapsedTimef();

        // 前回タップからの経過時間
        if (lastTapTime > 0.0f) {
            float interval = now - lastTapTime;  // 秒

            // 異常に長い/短いタップは「リセット」とみなす
            //   interval 0.2s → 300BPM
            //   interval 2.0s → 30BPM
            if (interval < 0.2f || interval > 2.0f) {
                // 変な間が空いたら履歴リセット
                tapIntervals.clear();
            }
            else {
                // 有効な間隔として履歴に追加
                tapIntervals.push_back(interval);
                if ((int)tapIntervals.size() > maxTapHistory) {
                    tapIntervals.erase(tapIntervals.begin()); // 古いのを削除
                }

                // 平均間隔から BPM を計算
                float sum = 0.0f;
                for (float v : tapIntervals) sum += v;
                float avg = sum / tapIntervals.size();   // 秒/拍
                if (avg > 0.0f) {
                    bpm = 60.0f / avg;
                    ofLogNotice() << "Tap BPM = " << bpm;
                }
            }
        }

        // 最後のタップ時刻を更新
        lastTapTime = now;
    }

    if (key == '1') {
        bEnableLogo = !bEnableLogo;
        ofLogNotice() << "Logo " << (bEnableLogo ? "ON" : "OFF");
    }

    // 2 → Mask On/Off
    if (key == '2') {
        bEnableMask = !bEnableMask;
        ofLogNotice() << "Mask " << (bEnableMask ? "ON" : "OFF");
    }

    // 3 → Crop On/Off
    if (key == '3') {
        bEnableCrop = !bEnableCrop;
        ofLogNotice() << "Crop " << (bEnableCrop ? "ON" : "OFF");
    }

    // 他のキー割り当てしたくなったらここに追加
    // if (key == 'r') { ... } など
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::sendOscChangedParamsOsc() {
    const float EPS = 0.0001f;

    for (int i = 0; i < NUM_PARAMS; ++i) {
        auto& p = midimixarray[i];

        if (fabs(p.val - p.lastSentVal) > EPS) {
            ofxOscMessage m;
            m.setAddress(p.oscAddress);
            m.addFloatArg(p.val);
            oscsender.sendMessage(m, false);

            // ★ ここでログも残す
            logOsc(p.oscAddress, p.val);

            p.lastSentVal = p.val;
        }
    }
}

void ofApp::sendLaunchPadOsc() {
    const float EPS = 0;

    for (int i = 0; i < LP_PADS; ++i) {
        auto& p = launchPads[i];

        // ★ FX 用パッドなど、OSC を送りたくないものはスキップ
        if (!p.sendOsc) {
            p.lastSentVal = p.val;   // 差分判定だけ進めておく
            continue;
        }

        if (fabs(p.val - p.lastSentVal) > EPS) {
            ofxOscMessage m;
            m.setAddress(p.oscAddress);
            m.addFloatArg(p.val);
            oscsender.sendMessage(m, false);

            // ★ ログ追加
            logOsc(p.oscAddress, p.val);

            p.lastSentVal = p.val;
        }
    }
}

void ofApp::drawOutput() {
    ofPushMatrix();
    //ofScale(1.2, 1.2);
    finalFbo.draw(0, 0, 1920, 1080);   // ★ postprocess.draw → finalFbo.draw
    ofPopMatrix();
}
