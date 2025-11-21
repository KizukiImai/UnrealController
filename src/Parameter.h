#pragma once
float colorR = 0.0f;
float colorG = 0.0f;
float colorB = 0.0f;

struct MidiMix {
	string ParName;
	float val;
	float lastSentVal;
	string oscAddress;
};