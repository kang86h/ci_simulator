#pragma once
#include "biquad.h"
#include <vector>

struct EnvelopeDetector {
	double sr{48000.0};
	double envSr{250.0};
	double lpfCut{160.0};
	Biquad envLpf;
	double decimRatio{192.0}; // sr/envSr (computed)
	double decimAcc{0.0};
	float held{0.0f};

	void prepare(double sampleRate, double envelopeSampleRate, double cutoff) {
		sr = sampleRate; envSr = envelopeSampleRate; lpfCut = cutoff;
		if (envSr <= 0) envSr = 250.0;
		if (lpfCut <= 0) lpfCut = 160.0;
		decimRatio = sr / envSr; if (decimRatio < 1.0) decimRatio = 1.0;
		envLpf = Biquad::designLowpass(sr, lpfCut);
		envLpf.reset();
		decimAcc = 0.0; held = 0.0f;
	}

	inline float processSample(float x) {
		// Full-rate rectification
		float env = std::fabs(x);
		float sm = envLpf.process(env);
		// Decimate by simple sample-and-hold at envSr
		decimAcc += 1.0;
		if (decimAcc >= decimRatio) { held = sm; decimAcc -= decimRatio; }
		return held; // envelope at envSr (held between updates)
	}
};

struct BandProcessor {
	Biquad bpf;
	Biquad carrierBpf;
	EnvelopeDetector env;
	float lastEnv{0.0f};

	void prepare(double sr, double f1, double f2, double envSr, double lpfCut) {
		bpf = Biquad::designBandpass(sr, f1, f2);
		bpf.reset();
		carrierBpf = Biquad::designBandpass(sr, f1, f2);
		carrierBpf.reset();
		env.prepare(sr, envSr, lpfCut);
		lastEnv = 0.0f;
	}

	inline float process(float x) {
		float y = bpf.process(x);
		lastEnv = env.processSample(y);
		return lastEnv;
	}
};


