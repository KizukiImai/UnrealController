#pragma once
#include "ofMain.h"
#include "ofApp.h"


class SubApp : public ofBaseApp {
public:
    SubApp(std::shared_ptr<ofApp> mainApp)
        : mainApp(mainApp) {
    }

    void setup();
    void update();
    void draw();

    std::shared_ptr<ofApp> mainApp;  // ƒƒCƒ“ƒAƒvƒŠ‚ğ•Û
};
