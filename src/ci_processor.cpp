#include "ci_processor.h"
#include <algorithm>
#include <cmath>

static inline std::pair<double,double> logInterp(double L, double H, double t0, double t1) {
	double r = H / L;
	double f0 = L * std::pow(r, t0);
	double f1 = L * std::pow(r, t1);
	return {f0, f1};
}

CIProcessor::CIProcessor(const CIConfig& cfg)
	: cfg_(cfg), rng_(cfg.noiseSeed), uni_(-1.0f, 1.0f) {
	buildBands();
	bandProcs_.resize(bands_.size());
	for (size_t i = 0; i < bands_.size(); ++i) {
		bandProcs_[i].prepare(cfg_.sampleRate, bands_[i].first, bands_[i].second,
			cfg_.envSampleRate, cfg_.envLpfCutoff);
	}
	noiseState_.resize(bands_.size(), 0.0f);
	fixedGainLinear_ = std::pow(10.0, cfg_.fixedGainDb / 20.0);
}

void CIProcessor::buildBands() {
	bands_.clear();
	int N = std::max(1, cfg_.numElectrodes);
	double L = cfg_.fmin, H = cfg_.fmax;
	if (cfg_.lambda <= 0.0) {
		for (int i = 0; i < N; ++i) {
			double t0 = static_cast<double>(i) / N;
			double t1 = static_cast<double>(i + 1) / N;
			bands_.push_back(logInterp(L, H, t0, t1));
		}
	} else {
		// Exponential weighting toward low frequency, rescaled to [0,1]
		std::vector<double> X(N + 1);
		for (int i = 0; i <= N; ++i) X[i] = static_cast<double>(i) / N;
		double lam = cfg_.lambda;
		std::vector<double> Y(N + 1);
		for (int i = 0; i <= N; ++i) Y[i] = lam * std::exp(X[i] * lam);
		// rescale to [0,1]
		double ymin = *std::min_element(Y.begin(), Y.end());
		double ymax = *std::max_element(Y.begin(), Y.end());
		for (double& y : Y) y = (y - ymin) / (ymax - ymin);
		for (int i = 0; i < N; ++i) {
			bands_.push_back(logInterp(L, H, Y[i], Y[i + 1]));
		}
	}
}

void CIProcessor::processBlock(const float* in, float* out, unsigned long frames) {
	// Noise vocoder: per-sample processing with bandpass carrier
	for (unsigned long n = 0; n < frames; ++n) {
		float x = in ? in[n] : 0.0f;
		float sum = 0.0f;
		for (size_t ch = 0; ch < bandProcs_.size(); ++ch) {
			float env = bandProcs_[ch].process(x);
			float noise = uni_(rng_);
			float carrier = bandProcs_[ch].carrierBpf.process(noise);
			sum += env * carrier;
		}
		out[n] = sum / static_cast<float>(std::max<size_t>(1, bandProcs_.size()));
	}
	// optional simple RMS matching (block-wise)
	if (cfg_.rmsMatch) {
		double inRms = 1e-9, outRms = 1e-9;
		if (in) {
			double acc = 0.0; for (unsigned long n = 0; n < frames; ++n) { double v = in[n]; acc += v * v; }
			inRms = std::sqrt(acc / frames);
		}
		{
			double acc = 0.0; for (unsigned long n = 0; n < frames; ++n) { double v = out[n]; acc += v * v; }
			outRms = std::sqrt(acc / frames);
		}
		double target = (outRms > 0.0) ? (inRms / outRms) : 1.0;
		adaptGain_ = 0.05 * target + 0.95 * adaptGain_; // smooth
		for (unsigned long n = 0; n < frames; ++n) out[n] = static_cast<float>(out[n] * adaptGain_ * fixedGainLinear_);
	} else {
		for (unsigned long n = 0; n < frames; ++n) out[n] = static_cast<float>(out[n] * fixedGainLinear_);
	}
}


