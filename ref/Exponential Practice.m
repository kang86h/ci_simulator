% Exponential Curve Practice

% X = linspace(0,1);
X = linspace(0,1,22+1);
r = (1:5);

for i = 1:length(r)
    lambda = 2^r(i);
    Y(:,i) = r(i).*exp(X*r(i));
    Y_norm(:,i) = rescale(Y(:,i));
end

% without for loop
X = linspace(0,1, 22+1);
r = (1:5)'; 
% lambda = 2.^r;

Y = r.*exp(X.*r);
Min = min(Y,[],2); Max = max(Y,[],2);
Y_norm = rescale(Y,'InputMin',Min,'InputMax',Max);
%

figure
plot(X,X,':k','LineWidth',2)
hold on
for i = 1:length(r)
%     plot(X,Y(:,i))
    plot(X,Y_norm(i,:),'o-')
end

legend({'Linear','\lambda=1','\lambda=2','\lambda=3','\lambda=4','\lambda=5'},...
    'Location','northwest')
ylabel('Frequency mapping (log scale)')
yticks([0 1])
yticklabels({'300Hz','7200Hz'})

xlabel('Electrode indicies')
xticks([0 1])
xticklabels({'Ch1', 'Ch22'})

title('Low-freq weighted mapping system')

X = linspace(0,1);
Y2 = X.^2;
plot(X,Y2)
% ylim([min(Y2) max(Y2)])
xlim([min(X) 1])

Y1 = 2.^X;
plot(X,Y1)

