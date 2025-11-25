#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "imgui-knobs.h"
#include "Parameter.h"
#include "ofxXmlSettings.h"
#include "ofxOsc.h"
#include "ofxMidiManager.h"
#include "ofxSpout2Receiver.h"
#include "ofxPostProcessing.h"


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void keyPressed(int key);


		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		//Util
		void drawImGui();
		void setupOsc();
		void LinkMidi();
		void initMidiMixArray();
		void drawMidiMix();
		void sendLaunchPadOsc();
		void sendOscChangedParamsOsc();
		void initLaunchPad();
		void drawLaunchPad();
		void setupSpout();
		void drawDebugTexture();
		void updateLaunchPadEasing();
		void updateLaunchPadFromMidi();
		void setupMidi();
		void updateMidi();
		void updateMidiMixFromMidi();

		ofxMidiManager midiManager;
		ofxOscSender oscsender;

		ofxMidiManager launchpad;
		ofxMidiManager midimix;


		int OutWidth;
		int OutHeight;
		ofxPostProcessing postprocess;

		ofTexture spouttexture;
		string spoutSenderName;

		ofx::spout2::Receiver spout;

		ofxXmlSettings xml;
		ofxImGui::Gui gui;
		//OSC
		string ipAddress;
		int port;

		static const int NUM_KNOBS = 24;
		static const int NUM_FADERS = 8;
		static const int NUM_PARAMS = NUM_KNOBS + NUM_FADERS;

		static const int LP_COLS = 8;
		static const int LP_ROWS = 8;
		static const int LP_PADS = LP_COLS * LP_ROWS;

		float bpm = 240.0f;

		LaunchPad launchPads[LP_PADS];

		MidiMix midimixarray[NUM_PARAMS];
		
};
