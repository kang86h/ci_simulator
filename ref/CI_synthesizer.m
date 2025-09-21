% Generating the CI-simulated sound
function [Synthesized_sig] = CI_synthesizer(DATA,Synthesized_sig_sr)
% Synthesized_sig_sr = 44100;

duration = size(DATA.CIsignal,2)/DATA.CIsignal_sr;
Noise = rand(size(DATA.CIsignal,1), duration*Synthesized_sig_sr)*2-1;
Envelope = zeros(size(DATA.CIsignal,1), duration*Synthesized_sig_sr);

for band = 1:size(DATA.CIsignal,1) % 1 = low-frequency
    Noise(band,:) = bandpass(Noise(band,:),DATA.electrode_frange(band,:),Synthesized_sig_sr);
    Envelope(band,:) = resample(DATA.CIsignal(band,:),Synthesized_sig_sr,DATA.CIsignal_sr)';

    MIX = Noise .* Envelope;
end
MIX = sum(MIX,1);

% Volume amplification
Amplify = rms(DATA.signal_ori)/rms(MIX);
Synthesized_sig = MIX*Amplify;

end