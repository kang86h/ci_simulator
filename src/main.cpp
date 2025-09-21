#include "audio_io.h"
#include "ci_processor.h"
#include <iostream>
#include <string>
#include <portaudio.h>

static void print_usage() {
	std::cout << "CI Real-time Simulator\n"
	          << "Options:\n"
	          << "  --electrodes N        (default 22)\n"
	          << "  --fmin Hz             (default 300)\n"
	          << "  --fmax Hz             (default 7200)\n"
	          << "  --lambda L            (default 0.0)\n"
	          << "  --env-sr Hz           (default 250)\n"
	          << "  --env-lpf Hz          (default 160)\n"
	          << "  --block-frames N      (default 256)\n"
	          << "  --input-device-id ID  (optional)\n"
	          << "  --output-device-id ID (optional)\n"
	          << "  --sample-rate Hz      (default 48000)\n"
	          << "  --gain-db dB          (post gain)\n"
	          << "  --no-rms-match        (disable RMS levelling)\n"
	          << "  --monitor             (bypass: mic->speaker)\n"
	          << "  --list-devices        (show device IDs and exit)\n"
	          << std::endl;
}

int main(int argc, char** argv) {
	CIConfig cfg;
	cfg.numElectrodes = 22;
	cfg.fmin = 300.0;
	cfg.fmax = 7200.0;
	cfg.lambda = 0.0; // 0 = log-uniform mapping
	cfg.envSampleRate = 250.0; // envelope sampling
	cfg.envLpfCutoff = 160.0;  // typical literature range 160-400 Hz
	cfg.sampleRate = 48000.0;  // I/O SR
	cfg.blockFrames = 256;
	cfg.noiseSeed = 42;
	cfg.rmsMatch = true;
	cfg.fixedGainDb = 0.0;

	int inId = -1, outId = -1;
	bool monitor = false;
	bool listDevices = false;

	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		auto next = [&](double& v){ if (i+1<argc) v = std::stod(argv[++i]); };
		auto nexti = [&](int& v){ if (i+1<argc) v = std::stoi(argv[++i]); };
		if (a == "--electrodes") nexti(cfg.numElectrodes);
		else if (a == "--fmin") next(cfg.fmin);
		else if (a == "--fmax") next(cfg.fmax);
		else if (a == "--lambda") next(cfg.lambda);
		else if (a == "--env-sr") next(cfg.envSampleRate);
		else if (a == "--env-lpf") next(cfg.envLpfCutoff);
		else if (a == "--block-frames") nexti(cfg.blockFrames);
		else if (a == "--sample-rate") next(cfg.sampleRate);
		else if (a == "--input-device-id") nexti(inId);
		else if (a == "--output-device-id") nexti(outId);
		else if (a == "--gain-db") next(cfg.fixedGainDb);
		else if (a == "--no-rms-match") { cfg.rmsMatch = false; }
		else if (a == "--monitor") { monitor = true; }
		else if (a == "--list-devices") { listDevices = true; }
		else if (a == "-h" || a == "--help") { print_usage(); return 0; }
	}

	try {
		if (listDevices) {
			Pa_Initialize();
			int n = Pa_GetDeviceCount();
			int defIn = Pa_GetDefaultInputDevice();
			int defOut = Pa_GetDefaultOutputDevice();
			std::cout << "Devices (ID : name [in,out]):" << std::endl;
			for (int i = 0; i < n; ++i) {
				const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
				if (!info) continue;
				std::cout << "  " << i << " : " << info->name << " [" << info->maxInputChannels << "," << info->maxOutputChannels << "]";
				if (i == defIn) std::cout << " (Default In)";
				if (i == defOut) std::cout << " (Default Out)";
				std::cout << std::endl;
			}
			Pa_Terminate();
			return 0;
		}

		CIProcessor processor(cfg);
		AudioIO io;
		io.openStream(cfg.sampleRate, cfg.blockFrames, inId, outId,
			[&](const float* in, float* out, unsigned long frames){
				if (monitor && in) {
					for (unsigned long n = 0; n < frames; ++n) out[n] = in[n];
				} else {
					processor.processBlock(in, out, frames);
				}
			});
		std::cout << "Running... Press Ctrl+C to quit." << std::endl;
		io.start();
		io.wait();
		io.stop();
		io.closeStream();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}


