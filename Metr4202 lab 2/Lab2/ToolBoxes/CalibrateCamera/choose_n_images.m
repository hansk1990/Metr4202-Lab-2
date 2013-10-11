function choose_n_images(rgb_images, n)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
for i = 1:n
    imwrite(uint8(rgb_images(:, :, :, round(100*rand))), ['imagefiles/' num2str(i) '.jpg']);
end

