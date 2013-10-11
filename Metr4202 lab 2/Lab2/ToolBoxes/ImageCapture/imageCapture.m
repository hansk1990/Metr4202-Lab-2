function imageCapture(level) 
    if ~(exist('kinect_mex')==3),
        fprintf('compiling the mex file... (probably need to change path names to get this to work!)\n');
        % NOTE probably need to change path names...
      mex  -I/Users/aberg/work/kinect/new_kinect/libfreenect/include  -I/usr/local/include /Users/aberg/work/kinect/new_kinect/libfreenect/build/lib/libfreenect.0.1.2.dylib /Users/aberg/work/kinect/new_kinect/libusb/libusb/.libs/libusb-1.0.0.dylib kinect_mex.cc

    end
    if(level == 1)
        fprintf('Making first call to initalize the kinect driver and listening thread\n');
        kinect_mex(); % call one to initialize the freenect interface
        pause(2)
        fprintf('Making second call starts getting data\n');
        kinect_mex(); % get first data...
    else
        fprintf('tilting down...\n');
        for i=1:100,
        kinect_mex('x');
        end
    end

%     figure(1);
%     clf

%     tic;

%     last = toc;
%     draw_cum = 0;
%     draw_start = toc; draw_time=0;
%     for i=1:10,
%         [a,b]=kinect_mex();
%         last = toc;
%         fprintf('\r frame time = %4.4f  drawing_time = %4.4f',last-draw_start,draw_time);
%         draw_start = toc;
%         if (length(b)>307200),
%             imagesc(permute(reshape(b,[3,640,480]),[3 2 1]))
%             image = permute(reshape(b,[3,640,480]),[3 2 1]);
%         else
%             imagesc(repmat(permute(reshape(b,[640,480]),[2 1]),[1 1 3]))
%             image = permute(reshape(b,[640,480]),[2 1]);
%         end
%         drawnow
%         draw_cum=draw_cum+toc-draw_start;
%         draw_time=toc-draw_start;
%     end


    kinect_mex('q');

    fprintf('done.  Have Fun!\n')
    close all;
end




