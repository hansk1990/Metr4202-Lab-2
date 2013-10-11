function [intrinsics extrinsics] = Calibration_main(n)
%% This is the main calibration file
close all; clc;


% Access kinect sensor, read in image data for calibration

% Choose_n_images.m

% Read in images
fprintf(1, 'Reading in images for calibration:');
data_calib_append;

%Suppress/extract images
fprintf(1, '\nSuppressing images for calibration:');
add_suppress_append;

%Auto calibrate sensor using RADOCCToolbox
fprintf(1, '\nComputing calibration parameters:');
click_calib_append;

%Compute final intrinsic/extrincic calibration parameters
fprintf(1, '\nCompute final intrinsic/extrinsic calibration parameters:');
go_calib_optim;

%Reproject calibration grids onto images
fprintf(1, '\nReprojecting calibration grids onto images:');
reproject_calib_append;
ext_calib;

intrinsics.fc = fc;
intrinsics.cc = cc;
intrinsics.alpha_c = alpha_c;
intrinsics.kc = kc;
intrinsics.err = err_std;

extrinsics.transformation_matricies = zeros(4,4,n);
for i = 1:n
    extrinsics.transformation_matricies(1:3, 1:3, i) = ... 
        eval(strcat('Rc_', num2str(i)));
    extrinsics.transformation_matricies(4, 1:4, i) = [0 0 0 1];
    extrinsics.transformation_matricies(1:3, 4, i) = ...
        eval(strcat('Tc_', num2str(i)));
end

show_calib_results;
clc;
close all;
%fprintf(1, 'Calibration Complete!\n');
end
