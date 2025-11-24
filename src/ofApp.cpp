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
    drawMidiMix();
	drawLaunchPad();
	drawDebugTexture();
	gui.end();

}

void ofApp::setupMidi() {
	midimix.listPorts();
	//launchpad.listPorts();
    midimix.setup(0);
	launchpad.setup(1);

}

void ofApp::updateMidi() {
	midimix.update();
	launchpad.update();
    updateLaunchPadFromMidi();   // ★ まず実機の状態を反映
    updateLaunchPadEasing();     // ★ EaseShot/EaseLoop のアニメを進める
}

void ofApp::initMidiMixArray() {
    for (int i = 0; i < NUM_KNOBS; ++i) {
        midimixarray[i].ParName = "Knob" + std::to_string(i + 1);
        midimixarray[i].val = 0.0f;
        midimixarray[i].lastSentVal = midimixarray[i].val;
        midimixarray[i].oscAddress = "/midimix/knob/" + std::to_string(i + 1);
    }

    // フェーダー 24〜31
    for (int i = 0; i < NUM_FADERS; ++i) {
        int idx = NUM_KNOBS + i;
        midimixarray[idx].ParName = "Fader" + std::to_string(i + 1);
        midimixarray[idx].val = 0.0f;
        midimixarray[idx].lastSentVal = midimixarray[idx].val;
        midimixarray[idx].oscAddress = "/midimix/fader/" + std::to_string(i + 1);
    }
}

void ofApp::updateLaunchPadFromMidi() {
    float now = ofGetElapsedTimef();

    for (int i = 0; i < LP_PADS; ++i) {
        LaunchPad& p = launchPads[i];

        int cc = launchpad.getCCValue(p.ccNumber); // 0〜127
        bool down = (cc > 0);                         // 押してる？
        bool wasDown = (p.lastCcValue > 0);
        bool pressed = down && !wasDown;              // 立ち上がり
        bool released = !down && wasDown;              // 立ち下がり（Button 用）

        switch (p.sendMode) {
        case LaunchPadSendMode::Button:
            // 押している間だけ 1, 離したら 0
            p.val = down ? 1.0f : 0.0f;
            break;

        case LaunchPadSendMode::Toggle:
            // 押された瞬間に 0/1 切り替え
            if (pressed) {
                p.val = (p.val > 0.5f) ? 0.0f : 1.0f;
            }
            break;

        case LaunchPadSendMode::EaseShot:
            // 押された瞬間に 0→1 イージング開始
            if (pressed) {
                p.isAnimating = true;
                p.animStartTime = now;
                p.val = 0.0f;
            }
            break;

        case LaunchPadSendMode::EaseLoop:
            // 押された瞬間に 0→1→0 イージング開始
            if (pressed) {
                p.isAnimating = true;
                p.animStartTime = now;
                p.val = 0.0f;
            }
            break;
        }

        p.lastCcValue = cc;
    }
}


void ofApp::initLaunchPad() {
    for (int i = 0; i < LP_PADS; ++i) {
        int col = i % LP_COLS;
        int row = i / LP_COLS;

        LaunchPad& p = launchPads[i];

        p.ParName = "Pad" + std::to_string(i + 1);
        p.val = 0.0f;
        p.lastSentVal = p.val;

        // /launchpad/row/col
        p.oscAddress = "/launchpad/" + std::to_string(row) + "/" + std::to_string(col);

        // CC 番号はそのまま i に対応（Pad1→CC0, Pad2→CC1 ...）
        p.ccNumber = i;
        p.lastCcValue = 0;

        if (row == 0) p.sendMode = LaunchPadSendMode::Button;
        else if (row == 1) p.sendMode = LaunchPadSendMode::Toggle;
        else if (row == 2) p.sendMode = LaunchPadSendMode::EaseShot;
        else               p.sendMode = LaunchPadSendMode::EaseLoop;

        p.isAnimating = false;
        p.animStartTime = 0.0f;
    }
}

void ofApp::updateLaunchPadEasing() {
    float beatDuration = 60.0f / bpm;        // 一拍の長さ [秒]
    float now = ofGetElapsedTimef();

    for (int i = 0; i < LP_PADS; ++i) {
        LaunchPad& p = launchPads[i];
        if (!p.isAnimating) continue;

        float t = (now - p.animStartTime) / beatDuration; // 0〜1 超える

        switch (p.sendMode) {
        case LaunchPadSendMode::EaseShot: {
            if (t >= 1.0f) {
                p.val = 1.0f;
                p.isAnimating = false;
            }
            else {
                p.val = easeInOutQuad(t);   // 0→1 イージング
            }
            break;
        }
        case LaunchPadSendMode::EaseLoop: {
            if (t >= 1.0f) {
                p.val = 0.0f;               // 一周して戻ったところで止める
                p.isAnimating = false;
            }
            else {
                float phase = ofClamp(t, 0.0f, 1.0f);
                if (phase <= 0.5f) {
                    // 前半 0〜0.5 : 0→1
                    float u = phase / 0.5f; // 0〜1
                    p.val = easeInOutQuad(u);
                }
                else {
                    // 後半 0.5〜1 : 1→0
                    float u = (1.0f - phase) / 0.5f; // 1→0 を 0〜1 にマッピング
                    p.val = easeInOutQuad(u);
                }
            }
            break;
        }
        default:
            // 他モードはここでは何もしない
            break;
        }
    }
}


void ofApp::drawMidiMix() {
    if (ImGui::Begin("MIDIMIX")) {
        for (int col = 0; col < 8; ++col) {
            ImGui::BeginGroup();
            for (int row = 0; row < 3; ++row) {
                int i = row * 8 + col; // 0〜23
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

                // （ここでは UI からは状態を変えず、見た目だけのボタンにしておく例）
                ImVec4 offCol(0.2f, 0.2f, 0.2f, 1.0f);   // 消灯時
                ImVec4 onCol(0.9f, 0.1f, 0.1f, 1.0f);   // 赤

                float f = 0.0f;  // 0〜1 の色係数

                switch (p.sendMode) {
                case LaunchPadSendMode::Button:
                    // 押されてる間だけ val=1 → 赤
                    // （MIDI連携後は val が 0/1 になる想定）
                    f = p.val;             // 0 or 1
                    break;

                case LaunchPadSendMode::Toggle:
                    // トグルだけは latched で赤を保持
                    f = p.val;             // 0 or 1（押したまま赤）
                    break;

                case LaunchPadSendMode::EaseShot:
                case LaunchPadSendMode::EaseLoop:
                    // Ease 系は「アニメ中だけ」赤。終わったら必ず消灯。
                    f = p.isAnimating ? p.val : 0.0f;
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

                ImGui::Button("##pad", padSize);  // 見た目用ボタン（入力は MIDI 側）

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

    // 常に 960x540 のウィンドウにする
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_Always);

    if (ImGui::Begin("Post Preview",
        nullptr,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar))  // ★ タイトルバーも消す
    {
        const ofTexture& tex = postprocess.getProcessedTextureReference();

        ImGui::Image(
            (void*)(uintptr_t)tex.getTextureData().textureID,
            ImVec2(960, 540),        // 内容サイズ＝ウィンドウサイズ
            ImVec2(0, 0),
            ImVec2(1, 1)
        );
    }
    ImGui::End();

    ImGui::PopStyleVar();
}




void ofApp::setupOsc(){
	// Setup OSC here if 
	ipAddress = xml.getValue("settings:OSC:ip", "localhost");
	port = xml.getValue("settings:OSC:port", 9000);
	//oscsender.setup("127.0.0.1",9000);
    oscsender.setup(ipAddress, port);
}
void ofApp::setupSpout(){
    // Setup Spout here if needed
	spoutSenderName = xml.getValue("settings:spout:name", "ofxSpoutSender");
	spout.setupAuto(spoutSenderName, spouttexture, false);
}

void ofApp::LinkMidi(){
	// Link MIDI here if needed
}

void ofApp::setup(){
	xml.loadFile("config.xml");
	initMidiMixArray();
	initLaunchPad();

    setupMidi();
	setupOsc();
	setupSpout();
	spouttexture.allocate(1280 , 720, GL_RGBA);
	OutWidth = xml.getValue("settings:outputResolution:width", 1280);
    OutHeight = xml.getValue("settings:outputResolution:height", 720);

	postprocess.init(OutWidth, OutHeight);
    postprocess.createPass<PixelatePass>()->setEnabled(true);
    

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
    sendOscChangedParamsOsc();
    sendLaunchPadOsc();

    if (launchpad.getCCValue(0) != 0) {
        cout << ofToString(launchpad.getCCValue(0)) << endl;
    }
}




//--------------------------------------------------------------
void ofApp::draw() {

    postprocess.begin();
    spouttexture.draw(0,0);
    postprocess.end(false);


    drawImGui();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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

void ofApp::sendOscChangedParamsOsc(){
    const float EPS = 0.0001f; // 許容誤差（微妙な揺れ無視したければ少し大きめに）

    for (int i = 0; i < NUM_PARAMS; ++i) {
        auto& p = midimixarray[i];

        if (fabs(p.val - p.lastSentVal) > EPS) {   // 値が変わった？
            ofxOscMessage m;
            m.setAddress(p.oscAddress);           // そのパラメータ専用のアドレス
            m.addFloatArg(p.val);
            oscsender.sendMessage(m, false);

            p.lastSentVal = p.val;                // 送った値を記録
        }
    }
}

void ofApp::sendLaunchPadOsc() {
    const float EPS = 0.0001f;

    for (int i = 0; i < LP_PADS; ++i) {
        auto& p = launchPads[i];
        if (fabs(p.val - p.lastSentVal) > EPS) {
            ofxOscMessage m;
            m.setAddress(p.oscAddress);
            m.addFloatArg(p.val);       // 0.0 or 1.0
            oscsender.sendMessage(m, false);
            p.lastSentVal = p.val;
        }
    }
}
