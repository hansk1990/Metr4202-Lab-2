% Function for flipping an image from kinect_mex as images are returned 
% mirrored (left to right)

function [newA] = matrixFlip(oldMatrix)
oldMatrix1 = fliplr(oldMatrix(:,:,1));
oldMatrix2 = fliplr(oldMatrix(:,:,2));
oldMatrix3 = fliplr(oldMatrix(:,:,3));

newB = cat(3, oldMatrix1, oldMatrix2);
newA = cat(3, newB, oldMatrix3);
