% There are two important data-structure.
% 1. Original sound signal
% 2. Encoded sound signal = struct(signal,electrode,norm)

% fpath = '/Users/jong/Documents/JONG/2. Research/on going_cochlear_implant/Analysis/';
fname = 'stimuli/speech.wav';
fname = '1.mp3';
[y,sr] = audioread(fname);
y = y(:,1); % to mono file
% set the length(y) to 'integer sec. duration'
if mod(length(y)/sr,1)
    taeget_length = ceil(length(y)/sr) *sr;
    y = [y; zeros(taeget_length-length(y),1)];
end
% y = resample(y,sr/2,sr);
% sr = sr/2;
clear fname taeget_length
ori_signal = y;
ori_signal_sr = sr;

f_range = [300 7200];
target_sr = 250; % simulated stimulation sample rate = 250hz (SPEAK processor parameter)
electrode_set = [22];
% electrode_set = [1 2 3 4 5 6 7 8 9 10 12 15 18 22 28 40 64 100];
normalize_set_size = 3000;

% initial data structure setup
DATA = struct('signal_ori',[],'signal_ori_sr',[],'signal_bpf',[],...
    'CIsignal',[],'CIsignal_sr',[],'CIsignal_norm',[],...
    'electrode',[],'electrode_frange',[],...
    'Synthesized_sig',[],'Synthesized_sig_sr',[]);
DATA(length(electrode_set)) = DATA;

% Linear system: Lambda=0, Low-weighted system: Lambda>0
Lambda = [0 1 2 4];
Lambda = [0];

for i = 1:length(electrode_set)
    fprintf('\n %d ',i);
    DATA(i).signal_ori = ori_signal;
    DATA(i).signal_ori_sr = ori_signal_sr;
    DATA(i).electrode = electrode_set(i);
    
    DATA(i) = SpeechProcessor(DATA(i),f_range,target_sr,Lambda(1));
    DATA(i).CIsignal_norm = normalize_speech_signal(...
        DATA(i).CIsignal,normalize_set_size);
    
    DATA(i).Synthesized_sig_sr = 44100;
    DATA(i).Synthesized_sig = CI_synthesizer(DATA(i),DATA(i).Synthesized_sig_sr);
end

for i = 1:length(Lambda)
    fprintf('\n %d ',i);
    DATA(i).signal_ori = ori_signal;
    DATA(i).signal_ori_sr = ori_signal_sr;
    DATA(i).electrode = electrode_set(1);
    
    DATA(i) = SpeechProcessor(DATA(i),f_range,target_sr,Lambda(i));
%     DATA(i).CIsignal_norm = normalize_speech_signal(...
%         DATA(i).CIsignal,normalize_set_size);
    
    DATA(i).Synthesized_sig_sr = 44100;
    DATA(i).Synthesized_sig = CI_synthesizer(DATA(i),DATA(i).Synthesized_sig_sr);
end

for i = 1:length(DATA)
    fname = sprintf('speech_22ch_300-7200_lambda-%d.wav',Lambda(i));
    audiowrite(fname,DATA(i).Synthesized_sig,DATA(i).Synthesized_sig_sr)
end

sound(Noise,44100)
sound(MIX,44100)
clear sound;

% Test
sound(ori_signal,44100)

sound(DATA(1).Synthesized_sig,44100)

filtered = bandpass(ori_signal,[1 3000],44100);
sound(filtered,44100)

audiowrite('1_synthesized_sig_22bands.wav',DATA(1).Synthesized_sig,DATA(1).Synthesized_sig_sr);

%% plotting
for i = 1
%     figure
    hold on
    h = zeros(DATA(i).electrode+1,1);
    for j = 1:DATA(i).electrode
        t_ori = (0:1/DATA(i).signal_ori_sr:(length(DATA(i).signal_ori)/DATA(i).signal_ori_sr));
        t_ori = t_ori(2:length(t_ori));
        t_target = (0:1/target_sr:length(DATA(i).signal_ori)/DATA(i).signal_ori_sr);
        t_target = t_target(2:length(t_target));
        
        h(j) = subplot(DATA(i).electrode+1,1, DATA(i).electrode-j+1);
        plot(t_ori, DATA(i).signal_bpf(j,:),'k');
        hold on
        if DATA(i).electrode-j+1 == 1
            title('Speech Signal Processing Strategy')
        end
        plot(t_target, DATA(i).CIsignal(j,:), 'LineWidth',2,'Color',[1 .3 .3])
        xlabel([]);
        ylab_txt = sprintf('%.0f~%.0fHz',...
            DATA(i).electrode_frange(j,1),...
            DATA(i).electrode_frange(j,2));
        ylabel(ylab_txt);
        xticks([]);
    end
end
h(DATA(i).electrode+1) = subplot(DATA(i).electrode+1,1,DATA(i).electrode+1);
plot(t_ori, DATA(i).signal_ori,'k');
ylabel('Original');
xlabel('Seconds');
linkaxes(h,'x')
xlim([0 3.5])
xticks([0 1 2 3])

%% imagesc
subplot(111)
imagesc(flipud(DATA(1).CIsignal))
title('Signal Stimulation: Electrodes to SGC')
yticks([1 2 3 4])
yticklabels({'Electrode 1' 'Electrode 2' 'Electrode 3' 'Electrode 4'})
xticks([0 1 2 3]*250)
xticklabels({'0' '1' '2' '3'})
xlim([1 3.5*250])
xlabel('Seconds')
colorbar()

%% Entropy and KLD
for i = [3 6 10 15 17]
%     figure
    imagesc(flipud(DATA(i).CIsignal_norm));
    yticks([])
    xticks([])
    xlabel('Time')
    ylabel('Electrodes (L~H)')

    ttext = sprintf('SGC stimulation with %d electrodes',DATA(i).electrode);
    title(ttext)
    
end

KL = zeros(1,length(DATA)-1);
rawdata = DATA(end).CIsignal_norm +1;
for i = 1:length(DATA)-1
    encoded = DATA(i).CIsignal_norm+1;
    KL(i) = -1*sum(rawdata .* (log2(rawdata ./ encoded)),'all');
end
encoded = DATA(4).CIsignal_norm+1;
KL_matrix = rawdata .* (log2(rawdata ./ (encoded+eps)));
imagesc(KL_matrix); colorbar();

showN = [1 2 3 4 5 8 10 12 14 16 17];
plot(electrode_set(showN), KL(showN),'o-','LineWidth',2,...
    'Color',[1 .3 .3],'MarkerEdgeColor',[1 .3 .3],'MarkerFaceColor',[1 .3 .3]);
xticks(electrode_set(showN))
xticklabels({'1' ' ' ' ' ' ' '5' ' ' '10' '15' '22' '40' '64'})
xlabel('Number of Electrodes')
ylabel('KL-Divergence')
title('Auditory Information Loss')
xlim([-2 70])
ylim([-0.2*10^8 6.3*10^8])
yticks([0 6]*10^8)

%% Shannon's Data

% Easy Sentences in Quiet
data_NH = [1 2 3 4 5 6 8 12 16 20; 3 24 62 82 91 93.5 98 99 99 100];
% NH difficult sentences or difficult conditions
data_difficult = [1 2 4 6 8 16; 2 7.7 38.8 63.5 84 94];
% Complex Music - Smith et al Nature 02 music-music chimeras
data_music = [8 16 32 48 64; 0 6 39 95 98];
% KL -> 0~100
KL_scale = 100 - 100*rescale(KL(1:8));

figure
semilogx(data_NH(1,:),data_NH(2,:),'^-','LineWidth',2,...
    'Color',[.3 .3 1],'MarkerEdgeColor',[.3 .3 1],'MarkerFaceColor',[.3 .3 1]);
hold on
semilogx(data_difficult(1,:),data_difficult(2,:),'v-','LineWidth',2,...
    'Color',[.5 .6 1],'MarkerEdgeColor',[.5 .6 1],'MarkerFaceColor',[.5 .7 1]);
semilogx(data_music(1,:),data_music(2,:),'s-','LineWidth',2,...
    'Color',[.2 .85 1],'MarkerEdgeColor',[.2 .85 1],'MarkerFaceColor',[.2 .85 1]);

semilogx(electrode_set(1:8),KL_scale,'o-','LineWidth',2,...
    'Color',[1 .3 .3],'MarkerEdgeColor',[1 .3 .3],'MarkerFaceColor',[1 .3 .3]);
xticks([data_NH(1,:) 64])
% xticklabels({'1' ' ' ' ' ' ' '5' ' ' '10' '15' '22' '40' '64'})
xlabel('Number of Electrodes')
ylabel('Accuracy')
title('Speech Recognition Task')
xlim([0 100])
yticks([0 25 50 75 100])
legend('Easy sentences in quiet', 'Difficult sentences',...
    'Complex Music','Loss Model:(1-KLD)', 'Location','southeast')
hold off

%%

entropy(encoded)
entropy(DATA(1).CIsignal_norm)
entropy(DATA(4).CIsignal_norm)
entropy(DATA(6).CIsignal_norm)
entropy(DATA(7).CIsignal_norm)


KL = sum(rawdata .* (log(rawdata ./ encoded)),'all');
mean(encoded,'all')
std(encoded,0,'all')


%% test
signal_hilbert = hilbert(ori_signal);
signal_hilbert_abs = abs(signal_hilbert);
signal_hilbert_abs_lpf = lowpass(signal_hilbert_abs,8,ori_signal_sr);
signal_hilbert_abs_lpf_resample = resample(signal_hilbert_abs_lpf,...
    60,ori_signal_sr);


t_ori = (0:1/ori_signal_sr:(length(ori_signal)/ori_signal_sr));
t_ori = t_ori(2:length(t_ori));
t_60 = (0:1/60:length(ori_signal)/ori_signal_sr);
t_60 = t_60(2:length(t_60));
h = zeros(5,1);

figure
hold on

h(1) = subplot(5,1,1);
plot(t_ori, ori_signal);
h(2) = subplot(5,1,2);
plot(t_ori, signal_hilbert);
h(3) = subplot(5,1,3);
plot(t_ori, signal_hilbert_abs);
h(4) = subplot(5,1,4);
plot(t_ori, signal_hilbert_abs_lpf);
h(5) = subplot(5,1,5);
plot(t_60, signal_hilbert_abs_lpf_resample);
linkaxes(h,'x')






