
dx = 0.000635;
x = linspace(-dx, dx, 256);
y = linspace(-dx, dx, 256);
[X, Y] = meshgrid(x,y);

n = 1.4;
l = 0.0674;
lambda0 = 780e-9;
lambda = 2*n*l / round(2*n*l / lambda0);
F = 1;
f = 0.2;

alpha = ones(size(X));

%%
R = sqrt(X.^2 + Y.^2);
theta = atan(R/f);
delta = (2*pi/lambda)*2*n*l*cos(theta);

T = 1 ./ (1 + F*(sin(delta)).^2);
T = (T-min(T(:)));
T = 256*T./max(T(:));
T(R>dx) = 1;
alpha(R>dx) = 0;

%%
figure;
imagesc(T);

%%
map = parula(256);
RGB = ind2rgb(round(T), map);
imwrite(RGB, 'icon.png', 'Alpha', alpha);