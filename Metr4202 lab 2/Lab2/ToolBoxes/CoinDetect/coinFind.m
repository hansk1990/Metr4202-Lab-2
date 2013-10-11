%Function to process an input image and separate a plate of australian currency 
%coins from the background and other noise, and to circularise the plate
%for use with hough circles
%Make sure contrast between the plate and background is large enough
%for the edge detection
%Inputs: Image, radius of plate
%Outputs: total value of coins on the plate, number of each type of coin,
%and the positions of the coins in the original input image
function [out, num, cointrans] = coinFind(image, platerad)
close all
%convert image to grayscale if it isn't already
[~, ~, p] = size(image);
if p == 3
    gray = rgb2gray(image);
else
    gray = image;
end
%determine edges using canny edge detector
E = edge(gray, 'canny', .8, 0.8);
%Used to check whether the plate edge detection is adequate for hough
%ellipses
%imshow(E);
%pause(1);

%parameters for ellipseDetection
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
params.minMajorAxis = 150;
params.maxMajorAxis = 250;
params.smoothStddev = 9;
params.rotation = 0;
params.rotationSpan = 20;
params. minAspectRatio = 0.5;
params.numBest = 3;
params.randomize = 0;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%Detect ellipse of plate for circular correction
bestFits = ellipseDetection(E, params);

%Draw ellipses on raw image
%figure;
%imshow(image);
%ellipse(bestFits(:,3),bestFits(:,4),bestFits(:,5)*pi/180,bestFits(:,1),bestFits(:,2),'g');
%figure;
%imshow(E);
%Draw ellipses on canny image
%ellipse(bestFits(:,3),bestFits(:,4),bestFits(:,5)*pi/180,bestFits(:,1),bestFits(:,2),'g');

%extract plate parameters from ellipse bestFit array
%Assuming hough ellipses gives the same ellipse for all matches
x0 = bestFits(1,1);
y0 = bestFits(1,2);
a = bestFits(1,3);
b = bestFits(1,4);
alpha = bestFits(1,5);

%Calculate max and min x and y coordinates for cropping
yhigh = y0+b*cosd(alpha);
ylow = y0-b*cosd(alpha);
xhigh = x0+a*cosd(alpha);
xlow = x0-a*cosd(alpha);

%Create input and base point arrays for transform matrix
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
point1 = [(x0-b*sind(alpha)) ylow];
point2 = [xlow (y0-a*sind(alpha))];
point3 = [(x0+b*sind(alpha)) yhigh];
point4 = [xhigh (y0+a*sind(alpha))];
input_points = [point1; point2; point3; point4];
b1 = [(xhigh-xlow)/2 0];
b2 = [0 (xhigh-xlow)/2];
b3 = [(xhigh-xlow)/2 2*a];
b4 = [2*a (xhigh-xlow)/2];
base_points = [b1; b2; b3; b4];
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%Create projective transform and inverse transform matrix to correct for camera position relative
%to plate
tform = cp2tform(input_points, base_points, 'projective');
invtform = cp2tform(base_points, input_points, 'projective');

%Crop plate from raw image
sub = image((round(ylow)):(round(yhigh)),(round(xlow)):(round(xhigh)),:);

%Transform plate to circle using transformation matrix
It = imtransform(sub, tform, 'XYScale', 1);

%Pass corrected plate ellipse to hough circles coin detection function to
%return values for each coin and the sum of their values. Position of the
%coins with their corresponding values are also returned for use in coin
%real world positioning function
[out, num, coinstruct] = houghCoin(It, 15, 50, platerad);

%Used to check whether the inverse transform of the plate reverses the
%correction step
%Iuntran = imtransform(imgout, invtform, 'XYScale', 1);
%Use this imshow to check whether the returned coin points after their
%transform match with the coins in the image
% imshow(image);
% hold on;

%Initialize struct for transformed coin points
cointrans = [];
%Copy and transform the coin coordinates from the corrected plate image to
%the original image
for i=1:length(coinstruct)
    cointrans(i).type = coinstruct(i).type;
    cointrans(i).coord = (invtform.tdata.T)*(coinstruct(i).coord)';
    cointrans(i).coord(1) = cointrans(i).coord(1) + xlow;
    cointrans(i).coord(2) = cointrans(i).coord(2) + ylow;
    %Use this debugging step to check the coin centres match with the
    %coins in the original image
%     switch cointrans(i).type
%         case 5
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xg')
%         case 10
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xb')
%         case 20
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xr')
%         case 50
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xy')
%         case 1
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xw')
%         case 2
%             plot(cointrans(i).coord(1),cointrans(i).coord(2), 'xc')
%     end
end