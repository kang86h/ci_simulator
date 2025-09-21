#pragma once
#include <cmath>

namespace biquad_consts { constexpr double kPi = 3.14159265358979323846; }

struct Biquad {
	double b0{0}, b1{0}, b2{0}, a1{0}, a2{0};
	double z1{0}, z2{0};

	inline float process(float x) {
		double y = b0 * x + z1;
		z1 = b1 * x - a1 * y + z2;
		z2 = b2 * x - a2 * y;
		return static_cast<float>(y);
	}
	inline void reset() { z1 = z2 = 0; }

	static Biquad designLowpass(double sr, double cutoff, double q=0.70710678) {
		Biquad f; if (cutoff <= 0) return f;
		double w0 = 2.0 * biquad_consts::kPi * cutoff / sr;
		double c = std::cos(w0), s = std::sin(w0);
		double alpha = s / (2.0 * q);
		double b0 = (1 - c) / 2.0;
		double b1 = 1 - c;
		double b2 = (1 - c) / 2.0;
		double a0 = 1 + alpha;
		double a1 = -2 * c;
		double a2 = 1 - alpha;
		f.b0 = b0 / a0; f.b1 = b1 / a0; f.b2 = b2 / a0;
		f.a1 = a1 / a0; f.a2 = a2 / a0;
		return f;
	}

	static Biquad designBandpass(double sr, double f1, double f2) {
		// Design a simple 2nd-order bandpass using RBJ: center/bandwidth
		if (f1 <= 0) f1 = 1.0;
		if (f2 <= f1) f2 = f1 + 1.0;
		double fc = std::sqrt(f1 * f2);
		double bw = f2 - f1;
		double q = fc / bw; if (q < 0.1) q = 0.1;
		double w0 = 2.0 * biquad_consts::kPi * fc / sr;
		double c = std::cos(w0), s = std::sin(w0);
		double alpha = s / (2.0 * q);
		Biquad f;
		double b0 = alpha; double b1 = 0.0; double b2 = -alpha;
		double a0 = 1 + alpha; double a1 = -2 * c; double a2 = 1 - alpha;
		f.b0 = b0 / a0; f.b1 = b1 / a0; f.b2 = b2 / a0;
		f.a1 = a1 / a0; f.a2 = a2 / a0;
		return f;
	}
};


