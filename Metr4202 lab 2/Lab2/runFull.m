% Function for running complete Lab 2 assignments.  Coin detection not
% Working completely, only requires some small adjustments to complete.
% Images required for calibration are stored in ImageFiles folder.
% Filepaths to specific folders can be found in ExampleRW in OpenNI folder
% and camera calibration section of this function.  They will need to be
% adjusted to reflect current system.  

%
%
% Toolboxes used:
%   Kinect_mex - image capture and depth mapping.
%       Adjusted for mac compatibility (file pointers to
%       OpenNI information will need to be adjusted for personal system.
%   CCFinder - Macbeth Colour Checker finder.
%   RADOCCToolbox - Automated camera calibration (Includes automatic corner
%       finder.
%   Coin Detection - Functions in CoinDetect folder written specifically
%       for this lab.  Also includes houghcircles.
%   Caltag - automated caltag detection program used for localisation of
%       central frame (and by association, coins and camera).
%   
%   

function [Gold Silver Intrinsics Extrinsics coinCount num] = runFull(n, plateSize)


%------------------Execute colour calibration-----------------------------%
fprintf(1, '\nTime for colour calibration!\n');
fprintf(1, '\nPress ENTER to continue\n');
pause();
close all;

[I XYZ] = getImageDepth();
Image = matrixFlip(I);
imshow(Image);
[X C] = CCFind(Image);

%Extract colours
imageRGB = Image;
imageHSV = rgb2hsv(Image);
imageYCbCr = rgb2ycbcr(Image);

Gold.rgb = imageRGB(X(12,2), X(12,1), :);
Gold.hsv = imageHSV(X(12,2), X(12,1), :);
Gold.lab = imageYCbCr(X(12,2), X(12,1), :);

Silver.rgb = imageRGB(X(21,2), X(21,1), :);
Silver.hsv = imageHSV(X(21,2), X(21,1), :);
Silver.lab = imageYCbCr(X(21,2), X(21,1), :);

fprintf(1, '\nColour calibration complete\n');

%----------------------------Capture images-------------------------------%
imageCapture(1);
clc;
fprintf(1, '\n Time to capture images for calibration!\n Press ENTER to continue\n');
pause();

getImageWrite(n);

%---------------------------Calibrate camera------------------------------%
fprintf(1, '\n Time for camera calibration!\n');
cd ..;
cd('/Users/hansk1990/Documents/MATLAB/Lab2/ImageFiles/');
[Intrinsics Extrinsics] = Calibration_main(n);
cd ..;
cd('/Users/hansk1990/Documents/MATLAB/Lab2/');
fprintf(1, '\n Camera calibration complete\n');
clc;

imageCapture(4); %Drop camera to lowest position
pause(1);

%---------------------------Detection of coins----------------------------%
fprintf(1, '\nTime to detect coins \n');
  [I XYZ] = getImageDepth();
clc;

fprintf(1, '\nDetecting coins now!\n');
Flip image
  Image = matrixFlip(I);
[coinCount, num, coinLocations] = coinFind(Image, plateSize);
fprintf(1, '\nEnd of coin detection\n');
clc;

%-------------------------Localisation and Mapping------------------------%
fprintf(1, '\nTime to capture images for localisation!\n Press ENTER to continue\n');
pause();


fprintf(1, '\nImage capture complete\n');
[wPt,iPt] = caltag(Image, '/ToolBoxes/caltag-master/test/output.mat', false);

%Obtain origin
Y = sort(iPt(:, 1), 'descend');
X = sort(iPt(:, 2), 'ascend');

xImage = round(X(1));
yImage = round(Y(1));
xyzCam = XYZ(640-xImage, yImage, :);
xyzCam(1) = xyzCam(1) + 20;
xyzCam(2) = xyzCam(2)-30;

% This section is part of localisation but is not completely tested.  Should
% accomodate the real world mapping of discovered coins

% coinRWLocations = zeros(length(coinLocations));
% for i=1:length(coinLocations)
%     coinRW = XYZ(640-coinLocations(i).coord(1), coinLocations(i).coord(2), :);
%     coinRW = xyzCam-coinRW;
%     coinRWLocations(i) = coinRW;
% end

end
