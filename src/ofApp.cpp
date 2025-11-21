#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::drawImGui(){

	gui.begin();
    drawMidiMix();
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

void ofApp::drawMidiMix() {
    if (ImGui::Begin("MIDIMIX")) {

        // === 上：3×8 のノブ ===
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

        // === 下：8本の縦フェーダー ===
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

void ofApp::setupOsc(){
	// Setup OSC here if 
	ipAddress = xml.getValue("settings:OSC:ip", "localhost");
	port = xml.getValue("settings:OSC:port", 9000);
	oscsender.setup("127.0.0.1",9000);
    // oscsender.setup(ipAddress, port);
}

void ofApp::LinkMidi(){
	// Link MIDI here if needed

}

void ofApp::setup(){
	xml.loadFile("config.xml");
	initMidiMixArray();
	setupOsc();

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
}

//--------------------------------------------------------------
void ofApp::draw(){
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