#include "ofMain.h"
#include "ofApp.h"
#include "SubApp.h"


int main() {
    // メインウィンドウ設定
    ofGLFWWindowSettings settings;
    settings.setSize(1920, 1080);
	settings.windowMode = OF_WINDOW;
    settings.setPosition(glm::vec2(100, 100));
    settings.resizable = true;
	//settings.setGLVersion(4, 3);  // OpenGL 3.2
	settings.decorated = true;
    auto mainWindow = ofCreateWindow(settings);

    // サブウィンドウ設定（メインとコンテキスト共有）
    settings.setSize(1920, 1080);
    settings.setPosition(glm::vec2(4000, 100));
    settings.shareContextWith = mainWindow;  // ★ GPUリソース共有したいとき
	settings.decorated = true;
	settings.windowMode = OF_WINDOW;
    auto subWindow = ofCreateWindow(settings);

    auto mainApp = std::make_shared<ofApp>();
    auto subWindowApp = std::make_shared<SubApp>(mainApp);

    

    // ウィンドウごとに別の App を割り当て
    ofRunApp(mainWindow, mainApp);
    ofRunApp(subWindow, subWindowApp);
    ofRunMainLoop();
}
