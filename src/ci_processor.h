#pragma once
#include "filters.h"
#include <random>
#include <vector>

struct CIConfig {
	int numElectrodes{22};
	double fmin{300.0};
	double fmax{7200.0};
	double lambda{0.0};
	double envSampleRate{250.0};
	double envLpfCutoff{160.0};
	double sampleRate{48000.0};
	int blockFrames{256};
	unsigned int noiseSeed{42};
	bool rmsMatch{true};
	double fixedGainDb{0.0};
};

class CIProcessor {
public:
	explicit CIProcessor(const CIConfig& cfg);
	void processBlock(const float* in, float* out, unsigned long frames);

private:
	CIConfig cfg_;
	std::vector<std::pair<double,double>> bands_;
	std::vector<BandProcessor> bandProcs_;
	std::vector<float> noiseState_;
	std::mt19937 rng_;
	std::uniform_real_distribution<float> uni_;

	double adaptGain_{1.0};
	double fixedGainLinear_{1.0};

	void buildBands();
};


