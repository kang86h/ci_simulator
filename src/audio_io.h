#pragma once
#include <portaudio.h>
#include <functional>
#include <stdexcept>

class AudioIO {
public:
	using Callback = std::function<void(const float* in, float* out, unsigned long frames)>;

	AudioIO() { PaError e = Pa_Initialize(); if (e != paNoError) throw std::runtime_error("Pa_Initialize failed"); }
	~AudioIO() { closeStream(); Pa_Terminate(); }

	void openStream(double sampleRate, unsigned long framesPerBuffer, int inputDeviceId, int outputDeviceId, Callback cb) {
		cb_ = std::move(cb);
		PaStreamParameters inParams{}; PaStreamParameters outParams{};
		const PaDeviceInfo* dinfo = nullptr; const PaDeviceInfo* oinfo = nullptr;
		if (inputDeviceId < 0) inputDeviceId = Pa_GetDefaultInputDevice();
		if (outputDeviceId < 0) outputDeviceId = Pa_GetDefaultOutputDevice();
		if (inputDeviceId >= 0) {
			inParams.device = inputDeviceId;
			dinfo = Pa_GetDeviceInfo(inputDeviceId);
			inParams.channelCount = 1;
			inParams.sampleFormat = paFloat32;
			inParams.suggestedLatency = dinfo ? dinfo->defaultLowInputLatency : 0.01;
			inParams.hostApiSpecificStreamInfo = nullptr;
		}
		if (outputDeviceId >= 0) {
			outParams.device = outputDeviceId;
			oinfo = Pa_GetDeviceInfo(outputDeviceId);
			outParams.channelCount = 1;
			outParams.sampleFormat = paFloat32;
			outParams.suggestedLatency = oinfo ? oinfo->defaultLowOutputLatency : 0.01;
			outParams.hostApiSpecificStreamInfo = nullptr;
		}
		PaError e = Pa_OpenStream(&stream_,
			inputDeviceId >= 0 ? &inParams : nullptr,
			outputDeviceId >= 0 ? &outParams : nullptr,
			sampleRate, framesPerBuffer, paNoFlag, &AudioIO::paCallback, this);
		if (e != paNoError) throw std::runtime_error("Pa_OpenStream failed");
	}

	void start() { if (stream_) Pa_StartStream(stream_); running_ = true; }
	void stop() { if (stream_) Pa_StopStream(stream_); running_ = false; }
	void wait() {
		while (running_) { Pa_Sleep(50); if (stream_ && Pa_IsStreamActive(stream_) == 0) break; }
	}
	void closeStream() { if (stream_) { Pa_CloseStream(stream_); stream_ = nullptr; } }

private:
	static int paCallback(const void* input, void* output,
		unsigned long frameCount, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
		AudioIO* self = static_cast<AudioIO*>(userData);
		const float* in = static_cast<const float*>(input);
		float* out = static_cast<float*>(output);
		if (!out) return paContinue;
		self->cb_(in, out, frameCount);
		return paContinue;
	}

	PaStream* stream_{nullptr};
	Callback cb_{};
	bool running_{false};
};


