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
#include <utility>




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
		void drawOutput();

		void setupPostEffects();
		ofxMidiManager midiManager;
		ofxOscSender oscsender;

		ofxMidiManager launchpad;
		ofxMidiManager midimix;



		ofxPostProcessing postprocess;

		std::shared_ptr<BleachBypassPass>       bleachPass;
		std::shared_ptr<BloomPass>              bloomPass;
		std::shared_ptr<ContrastPass>           contrastPass;
		std::shared_ptr<DofAltPass>             dofAltPass;
		std::shared_ptr<DofPass>                dofPass;
		std::shared_ptr<EdgePass>               edgePass;
		std::shared_ptr<FakeSSSPass>            fakeSSSPass;
		std::shared_ptr<GodRaysPass>            godRaysPass;
		std::shared_ptr<KaleidoscopePass>       kaleidoPass;
		std::shared_ptr<LimbDarkeningPass>      limbDarkPass;
		std::shared_ptr<NoiseWarpPass>          noiseWarpPass;
		std::shared_ptr<PixelatePass>           pixelatePass;
		std::shared_ptr<RGBShiftPass>           rgbShiftPass;
		std::shared_ptr<SSAOPass>               ssaoPass;
		std::shared_ptr<ToonPass>               toonPass;
		std::shared_ptr<ZoomBlurPass>           zoomBlurPass;

		// Launchpad 右上用に使う 15 個
		std::vector<std::shared_ptr<itg::RenderPass>> allPasses;

		int OutWidth;
		int OutHeight;



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

		float bpm = 140.0f;

		LaunchPad launchPads[LP_PADS];

		MidiMix midimixarray[NUM_PARAMS];

		float lastTapTime = 0.0f;             // 前回タップした時間
		std::vector<float> tapIntervals;      // 直近の拍間隔をいくつか記録
		int   maxTapHistory = 4;              // 何拍分平均するか

		//overlay追加機能
		ofFbo finalFbo;        // postprocess の結果 + ロゴ + マスク + クロップ を入れる最終FBO

		ofImage      logoImage;   // ロゴ画像（例: data/logo.png）
		ofVideoPlayer maskVideo;  // グレースケールのマスク動画

		bool bEnableLogo = true;
		bool bEnableMask = false;
		bool bEnableCrop = false;

		// 上下クロップ量（画面高さに対する割合）
		float cropTopRatio = 0.15f;  // 上 15%
		float cropBottomRatio = 0.20f;  // 下 20%

		// コンポジット関数
		void Crop();                            // postprocess の結果に Mask / Logo / Crop を適用して finalFbo を作る
		void drawLogo(float bandY, float bandH);
		// 上バンドにロゴを描画（fader2 でアルファ）
		void drawMask(float y, float h);        // 中央エリアにマスク動画を Multiply
		// ofApp.h の class ofApp { ... } 内に追加

	// ===== OSC ログ表示用 =====
		std::vector<std::string> oscLog;
		int maxOscLogLines = 32;     // 下の帯に表示する最大行数

		void logOsc(const std::string& addr, float value);
		void drawOscStatusStrip(float bandY, float bandH);

		ofImage topBandImage;

		// ===== Step Sequencer =====
		static const int SEQ_STEPS = 8; // 8th notes in 1 bar (4/4)
		static const int SEQ_ROWS = 6; // 6 tracks

		bool  bSeqPlay = true;
		float seqStartTime = 0.0f;
		int   seqCurStep = 0;
		int   seqLastStep = -1;
		float seqStepPhase = 0.0f; // 0..1 within step

		// grid[row][step]
		bool seqGrid[SEQ_ROWS][SEQ_STEPS] = { false };

		// row settings
		bool        seqRowEnabled[SEQ_ROWS] = { true, true, true, true, true, true };
		int         seqRowMode[SEQ_ROWS] = { 0, 0, 0, 0, 0, 0 }; // 0=Button, 1=EaseShot, 2=EaseLoop
		std::string seqRowAddress[SEQ_ROWS];

		// per-row animation state (for EaseShot/EaseLoop)
		bool  seqRowAnimating[SEQ_ROWS] = { false };
		float seqRowAnimStart[SEQ_ROWS] = { 0 };
		float seqRowLastSent[SEQ_ROWS] = { 9999,9999,9999,9999,9999,9999 };

		// functions
		void setupSequencer();
		void updateSequencer();
		void drawSequencerUI();
		void sendSeqOsc(int row, float value);


		
};
