#include "audio_io.h"
#include "ci_processor.h"
#include <iostream>
#include <string>
#include <limits>
#include <portaudio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static void WriteLineUnicode(const std::wstring& wline) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    if (h && h != INVALID_HANDLE_VALUE) {
        WriteConsoleW(h, wline.c_str(), (DWORD)wline.size(), &written, nullptr);
        WriteConsoleW(h, L"\r\n", 2, &written, nullptr);
    }
}

static std::wstring Utf8ToWideBestEffort(const char* s) {
    if (!s) return L"";
    int len = (int)strlen(s);
    // Try UTF-8 first
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, len, nullptr, 0);
    if (wlen > 0) {
        std::wstring w(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, len, &w[0], wlen);
        return w;
    }
    // Fallback to system ANSI code page
    wlen = MultiByteToWideChar(CP_ACP, 0, s, len, nullptr, 0);
    if (wlen > 0) {
        std::wstring w(wlen, L'\0');
        MultiByteToWideChar(CP_ACP, 0, s, len, &w[0], wlen);
        return w;
    }
    return L"?";
}

static std::wstring AsciiToWide(const std::string& s) {
    if (s.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], wlen);
    return w;
}
#endif

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
	          << "\nNote: If you omit device IDs, you will be prompted to select devices at runtime.\n"
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

        // interactive device selection if not provided
        if (inId < 0 || outId < 0) {
            Pa_Initialize();
            int n = Pa_GetDeviceCount();
            int defIn = Pa_GetDefaultInputDevice();
            int defOut = Pa_GetDefaultOutputDevice();

#ifdef _WIN32
            // Prefer UTF-8 console for robust multilingual output
            UINT prevOutCp = GetConsoleOutputCP();
            UINT prevInCp  = GetConsoleCP();
            SetConsoleOutputCP(65001);
            SetConsoleCP(65001);
#endif

            // Ask whether to use current settings
            std::cout << "Use current devices? 1) Yes  2) No (manual)\nSelect: ";
            std::string sel; std::getline(std::cin, sel);
            bool manual = (sel == "2");
            if (manual) {
                // List input devices only
                std::cout << "\nInput devices (ID : name [in]):" << std::endl;
                for (int i = 0; i < n; ++i) {
                    const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
                    if (!info || info->maxInputChannels <= 0) continue;
#ifdef _WIN32
                    std::wstring line = AsciiToWide("  ") + AsciiToWide(std::to_string(i)) + AsciiToWide(" : ") +
                                        Utf8ToWideBestEffort(info->name) + AsciiToWide(" [") +
                                        AsciiToWide(std::to_string(info->maxInputChannels)) + AsciiToWide("]");
                    if (i == defIn) line += AsciiToWide(" (Default In)");
                    WriteLineUnicode(line);
#else
                    std::cout << "  " << i << " : " << info->name << " [" << info->maxInputChannels << "]";
                    if (i == defIn) std::cout << " (Default In)";
                    std::cout << std::endl;
#endif
                }
                auto promptId = [&](const char* label, bool isInput, int defId)->int{
                    for(;;){
                        std::cout << label << " device id (Enter for default " << defId << "): ";
                        std::cout.flush();
                        std::string line; std::getline(std::cin, line);
                        if (line.empty()) return defId;
                        try {
                            int v = std::stoi(line);
                            if (v >= 0 && v < n) {
                                const PaDeviceInfo* inf = Pa_GetDeviceInfo(v);
                                if (inf && ((isInput && inf->maxInputChannels>0) || (!isInput && inf->maxOutputChannels>0))) return v;
                            }
                            std::cout << "  Invalid id for this direction. Try again.\n";
                        } catch(...) { std::cout << "  Please enter a number.\n"; }
                    }
                };
                inId = promptId("Input", true, defIn);

                // List output devices only
                std::cout << "\nOutput devices (ID : name [out]):" << std::endl;
                for (int i = 0; i < n; ++i) {
                    const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
                    if (!info || info->maxOutputChannels <= 0) continue;
#ifdef _WIN32
                    std::wstring line = AsciiToWide("  ") + AsciiToWide(std::to_string(i)) + AsciiToWide(" : ") +
                                        Utf8ToWideBestEffort(info->name) + AsciiToWide(" [") +
                                        AsciiToWide(std::to_string(info->maxOutputChannels)) + AsciiToWide("]");
                    if (i == defOut) line += AsciiToWide(" (Default Out)");
                    WriteLineUnicode(line);
#else
                    std::cout << "  " << i << " : " << info->name << " [" << info->maxOutputChannels << "]";
                    if (i == defOut) std::cout << " (Default Out)";
                    std::cout << std::endl;
#endif
                }
                outId = promptId("Output", false, defOut);
            } else {
                // Keep defaults or CLI-provided ones (-1 = system default)
            }

#ifdef _WIN32
            // Restore console codepage
            SetConsoleOutputCP(prevOutCp);
            SetConsoleCP(prevInCp);
#endif

            Pa_Terminate();
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


