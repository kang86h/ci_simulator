function [DATA] = SpeechProcessor(DATA,f_range,target_sr,Lambda)

% Speech Processor
% first, set the bandpass filter range of each electrodes
[L,H] = deal(f_range(1), f_range(2));
% step = (H-L)/electrode;
N_electrode = DATA.electrode;
i = (1:N_electrode)';

if Lambda == 0
    electrode_frange = [L*(H/L).^((i-1)/N_electrode), L*(H/L).^((i)/N_electrode)];
else
    % Low weighted system (Exponently curved system)
    X = linspace(0,1,N_electrode+1)';
    Y = Lambda.*exp(X.*Lambda);
    Y = rescale(Y); % rescale Y to range of [0 to 1] 
    electrode_frange = [L*(H/L).^Y(1:end-1) L*(H/L).^Y(2:end)];

    % figure
    % semilogy(electrode_frange,'o-')
    % ylim(f_range)
    % xlim([1 N_electrode])
end

% preset the data structure of results.
signal_bpf = zeros(N_electrode,length(DATA.signal_ori));
signal_envelope = zeros(N_electrode,length(DATA.signal_ori));
encoded_signal = zeros(N_electrode,...
    length(DATA.signal_ori)/DATA.signal_ori_sr*target_sr);

% the simulation model of speech processor
for i = 1:N_electrode
    fprintf('%d',i);
    signal_bpf(i,:) = bandpass(DATA.signal_ori,...
        electrode_frange(i,:),DATA.signal_ori_sr);
	signal_envelope(i,:) = abs(hilbert(signal_bpf(i,:)));
    signal_envelope(i,:) = lowpass(signal_envelope(i,:),8,DATA.signal_ori_sr);
    encoded_signal(i,:) = resample(signal_envelope(i,:),target_sr,DATA.signal_ori_sr);
end

DATA.signal_bpf = signal_bpf;
DATA.CIsignal = encoded_signal;
DATA.CIsignal_sr = target_sr;
DATA.electrode_frange = electrode_frange;
end
