function [I XYZ] = getImageDepth
close all;
clear all;
clc;
%addpath('Mex')
SAMPLE_XML_PATH='/Users/hansk1990/Documents/MATLAB/Lab2/ToolBoxes/OpenNI1/Config/SamplesConfig.xml';

% To use the Kinect hardware use :
KinectHandles=mxNiCreateContext(SAMPLE_XML_PATH);

I=mxNiPhoto(KinectHandles); I=permute(I,[3 2 1]);
XYZ=mxNiDepthRealWorld(KinectHandles);
X=XYZ(:,:,1); Y=XYZ(:,:,2); Z=XYZ(:,:,3);
maxZ=3000; Z(Z>maxZ)=maxZ; Z(Z==0)=nan;

[ind,map] = rgb2ind(permute(I,[2 1 3]),255);
figure,
h=surf(X,Y,maxZ-Z,double(ind)/256,'EdgeColor','None');
colormap(map); 
axis equal; view(3);
pause(5);

for i=1:20
    mxNiUpdateContext(KinectHandles);
    I=mxNiPhoto(KinectHandles); I=permute(I,[3 2 1]);
    XYZ=mxNiDepthRealWorld(KinectHandles);
    X=XYZ(:,:,1); Y=XYZ(:,:,2); Z=XYZ(:,:,3);
    maxZ=3000; Z(Z>maxZ)=maxZ; Z(Z==0)=nan;
    set(h,'XData',X);
    set(h,'YData',Y);
    set(h,'ZData',maxZ-Z);
    [ind,map] = rgb2ind(permute(I,[2 1 3]),255);
    set(h,'CData',double(ind)/256);
    colormap(map); 
    drawnow
end
I = permute(I, [1 2 3]);
% Stop the Kinect Process
mxNiDeleteContext(KinectHandles);
end
