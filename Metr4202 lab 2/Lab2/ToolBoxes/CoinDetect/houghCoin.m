%Detects australian currency coins. Image is auto corrected to ensure
%constant pixel width of coins.
%Inputs: corrected plate image, min and max radius of coins (Use 15 and
%50), and the radius of the plate the coins are placed upon
%Outputs: total money on the plate; number and type of coins; type and position of each coin 
function [total_money_value, num, coinpos] = houghCoin(image, min_radius, max_radius, platerad)
%determine size of input image
[m, ~] = size(image);
%resize image to create constant pixel size for each coin
image = imresize(image, ((2.08*2*platerad)/m));
%apply guassian filter to smooth jagged edges from image resize
for i=1:10
    G = fspecial('gaussian',[5 5],2);
    image = imfilter(image,G,'same');
end
%grayscale image and then canny edge detect the coins
Ig = rgb2gray(image);
Ig = edge(Ig, 'canny', 0.2, 0.9);

%initialize struct for holding coin count
num.five = 0;
num.ten = 0;
num.twenty = 0;
num.fifty = 0;
num.one = 0;
num.two = 0;
thresh = 0.55;
%detect circles in the image
%use to view the output of the hough circles detection
%houghcircles(Ig, min_radius, max_radius, thresh);
h = houghcircles(Ig, min_radius, max_radius, thresh);
[n, ~] = size(h);
%threshold to separate similar sized coins between gold and silver values
bluethresh = 0.5;
%initialize array for holding the coin position structure
coinpos = [];
%convert image to hsv to use threshold value
image = rgb2hsv(image);
%iterate over detected coins to determine their individual size, position
%and value
for i = 1:n
     if ((14 <= h(i,3)) && (h(i,3) <= 17) && (image(h(i,2),h(i,1),1) >= bluethresh))
         num.five = num.five + 1;
         coin.type = 5;
     elseif ((20 <= h(i,3)) && (h(i,3) <= 23) && (image(h(i,2),h(i,1),1) >= bluethresh))
         num.ten = num.ten + 1;
         coin.type = 10;
     elseif ((23 <= h(i,3)) && (h(i,3) <= 30) && (image(h(i,2),h(i,1),1) >= bluethresh))
         num.twenty = num.twenty + 1;
         coin.type = 20;
     elseif ((31 <= h(i,3)) &&(h(i,3) <= 38) && (image(h(i,2),h(i,1),1) >= bluethresh))
         num.fifty = num.fifty + 1;
         coin.type = 50;
     elseif ((22 <= h(i,3)) && (h(i,3) <= 27) && (image(h(i,2),h(i,1),1) <= bluethresh))
         num.one = num.one + 1;
         coin.type = 1;
     elseif ((19 <= h(i,3)) && (h(i,3) <= 22) && (image(h(i,2),h(i,1),1) <= bluethresh))
         num.two = num.two + 1;
         coin.type = 2;
     end
     coin.coord = [(h(i,1)*(m/556)) (h(i,2)*(m/556)) 0];
     coinpos = [coinpos coin];
end
%calculate total value of all the coins
 total_money_value = 0.05*num.five + 0.1*num.ten + 0.2*num.twenty + 0.5*num.fifty + num.one + 2*num.two;
end

