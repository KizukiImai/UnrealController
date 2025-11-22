#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::drawImGui(){

	gui.begin();
    drawMidiMix();
	drawLaunchPad();
	gui.end();

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

void ofApp::initLaunchPad() {
    for (int i = 0; i < LP_PADS; ++i) {
        int col = i % LP_COLS;    // 0〜7
        int row = i / LP_COLS;    // 0〜7

        launchPads[i].ParName = "Pad" + std::to_string(i + 1);
        launchPads[i].val = 0.0f;
        launchPads[i].lastSentVal = launchPads[i].val;

        // 例: /launchpad/row/col （好きに変えてOK）
        launchPads[i].oscAddress = "/launchpad/" + std::to_string(row) + "/" + std::to_string(col);
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
                int i = row * LP_COLS + col;  // 0〜63

                ImGui::PushID(i);

                ImGui::Button("##pad", padSize);
                launchPads[i].val = ImGui::IsItemActive() ? 1.0f : 0.0f;
                ImGui::Text("%s", launchPads[i].ParName.c_str());

                ImGui::PopID();
            }
            ImGui::EndGroup();
            if (col < LP_COLS - 1) ImGui::SameLine();
        }
    }
    ImGui::End();
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
	setupOsc();
	setupSpout();

	ofxImGui::BaseTheme* theme = nullptr;
	bool autoDraw = true;
	ImGuiConfigFlags flags = ImGuiConfigFlags_None;
	bool restoreGuiState = true;
	bool showImGuiMouseCursor = false;

	gui.setup(theme, autoDraw, flags, restoreGuiState, showImGuiMouseCursor);

}

//--------------------------------------------------------------
void ofApp::update(){
    
	sendOscChangedParamsOsc();
	sendLaunchPadOsc();
}

//--------------------------------------------------------------
void ofApp::draw(){
	drawImGui();
    spouttexture.draw(0, 0);
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
