% Normalization
% check whether 'encoded_signal' is exist.
% if you want to calculate KL divergence, then check 'raw_signal' also.

function [signal_norm] = normalize_speech_signal(current_signal,target_size)

current_size = size(current_signal,1);
step = round(target_size / current_size);
signal_norm = zeros(target_size,size(current_signal,2));

input = zeros(step,size(current_signal,2));
for i = 1:current_size
    for j = 1:step
        input(j,:) = current_signal(i,:);
    end
    signal_norm((i-1)*step+1:i*step, :) = input;
end
if size(signal_norm,1) > target_size
    signal_norm = signal_norm(1:target_size,:);
elseif size(signal_norm,1) < target_size
    moreRow = target_size - size(signal_norm,1);
    signal_norm = [signal_norm; remat(signal_norm(end,:),moreRow,1)];
end
y_scale = [0, 2^8];
signal_norm = rescale(signal_norm,y_scale(1),y_scale(2),'InputMin',0,'InputMax',0.2);
signal_norm = round(signal_norm);
end
