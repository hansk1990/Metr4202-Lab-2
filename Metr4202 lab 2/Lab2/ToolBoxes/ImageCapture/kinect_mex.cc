/*
 * This is demo code by Alex Berg (c) 2010 and is adapted from and relies on
 * code in the OpenKinect Project. http://www.openkinect.org
 * Copyright (c) 2010 individual OpenKinect contributors. See the end of this file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

//
// See acberg.com/kinect for more information.
//
//
// COMPILING THE MEX FILE:
//
//  can compile the mex function from matlab with something like:
//
// mex  -I/Users/aberg/work/kinect/libfreenect/include -I/Users/aberg/work/kinect/libfreenect/examples/../wrappers/c_sync  -I/usr/local/include lib/libfreenect.0.0.1.dylib /usr/local/lib/libusb-1.0.dylib kinect_mex.cc
//
//
//  modify the paths as needed (especially to  libfreenect)
//
//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "libfreenect.h"
#include <pthread.h>
#include <math.h>
#include "mex.h"


pthread_t freenect_thread;

volatile int die = 0;

pthread_mutex_t gl_backbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

// back: owned by libfreenect (implicit for depth)
// mid: owned by callbacks, "latest frame ready"
// front: ready to copy to matlab variables

uint16_t *depth_mid, *depth_front;
uint8_t *rgb_back, *rgb_mid, *rgb_front;

freenect_context *f_ctx;
freenect_device *f_dev;
int freenect_angle = 0;
int freenect_led;

freenect_video_format requested_format = FREENECT_VIDEO_RGB;
freenect_video_format current_format = FREENECT_VIDEO_RGB;

pthread_cond_t gl_frame_cond = PTHREAD_COND_INITIALIZER;
int got_rgb = 0;
int got_depth = 0;

int g_state=0;


void Update()
{
	pthread_mutex_lock(&gl_backbuf_mutex);
	// When using YUV_RGB mode, RGB frames only arrive at 15Hz, so we shouldn't force them to draw in lock-step.
	// However, this is CPU/GPU intensive when we are receiving frames in lockstep.
	if (current_format == FREENECT_VIDEO_YUV_RGB) {
		while (!got_depth && !got_rgb) {
			pthread_cond_wait(&gl_frame_cond, &gl_backbuf_mutex);
		}
	} else {
		while ((!got_depth || !got_rgb) && requested_format != current_format) {
			pthread_cond_wait(&gl_frame_cond, &gl_backbuf_mutex);
		}
	}

	if (requested_format != current_format) {
		pthread_mutex_unlock(&gl_backbuf_mutex);
		return;
	}

	void *tmp;

	if (got_depth) {
		tmp = depth_front;
		depth_front = depth_mid;
		depth_mid = (uint16_t*)tmp;
		got_depth = 0;
	}
	if (got_rgb) {
		tmp = rgb_front;
		rgb_front = rgb_mid;
		rgb_mid = (uint8_t*)tmp;
		got_rgb = 0;
	}

	pthread_mutex_unlock(&gl_backbuf_mutex);
}

// void keyPressed(unsigned char key, int x, int y)
// {
// 	if (key == 27) {
// 		die = 1;
// 		pthread_join(freenect_thread, NULL);
// 		glutDestroyWindow(window);
// 		free(depth_mid);
// 		free(depth_front);
// 		free(rgb_back);
// 		free(rgb_mid);
// 		free(rgb_front);
// 		pthread_exit(NULL);
// 	}
// 	if (key == 'w') {
// 		freenect_angle++;
// 		if (freenect_angle > 30) {
// 			freenect_angle = 30;
// 		}
// 	}
// 	if (key == 's') {
// 		freenect_angle = 0;
// 	}
// 	if (key == 'f') {
// 		if (requested_format == FREENECT_VIDEO_IR_8BIT)
// 			requested_format = FREENECT_VIDEO_RGB;
// 		else if (requested_format == FREENECT_VIDEO_RGB)
// 			requested_format = FREENECT_VIDEO_YUV_RGB;
// 		else
// 			requested_format = FREENECT_VIDEO_IR_8BIT;
// 	}
// 	if (key == 'x') {
// 		freenect_angle--;
// 		if (freenect_angle < -30) {
// 			freenect_angle = -30;
// 		}
// 	}
// 	if (key == '1') {
// 		freenect_set_led(f_dev,LED_GREEN);
// 	}
// 	if (key == '2') {
// 		freenect_set_led(f_dev,LED_RED);
// 	}
// 	if (key == '3') {
// 		freenect_set_led(f_dev,LED_YELLOW);
// 	}
// 	if (key == '4') {
// 		freenect_set_led(f_dev,LED_BLINK_YELLOW);
// 	}
// 	if (key == '5') {
// 		freenect_set_led(f_dev,LED_BLINK_GREEN);
// 	}
// 	if (key == '6') {
// 		freenect_set_led(f_dev,LED_BLINK_RED_YELLOW);
// 	}
// 	if (key == '0') {
// 		freenect_set_led(f_dev,LED_OFF);
// 	}
// 	freenect_set_tilt_degs(f_dev,freenect_angle);
// }

// void ReSizeGLScene(int Width, int Height)
// {
// 	glViewport(0,0,Width,Height);
// 	glMatrixMode(GL_PROJECTION);
// 	glLoadIdentity();
// 	glOrtho (0, 1280, 480, 0, -1.0f, 1.0f);
// 	glMatrixMode(GL_MODELVIEW);
// }

// void InitGL(int Width, int Height)
// {
// 	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
// 	glClearDepth(1.0);
// 	glDepthFunc(GL_LESS);
// 	glDisable(GL_DEPTH_TEST);
// 	glEnable(GL_BLEND);
// 	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 	glShadeModel(GL_SMOOTH);
// 	glGenTextures(1, &gl_depth_tex);
// 	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glGenTextures(1, &gl_rgb_tex);
// 	glBindTexture(GL_TEXTURE_2D, gl_rgb_tex);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	ReSizeGLScene(Width, Height);
// }

// void *gl_threadfunc(void *arg)
// {
// 	printf("GL thread\n");

// 	glutInit(NULL, NULL);

// 	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
// 	glutInitWindowSize(1280, 480);
// 	glutInitWindowPosition(0, 0);

// 	window = glutCreateWindow("LibFreenect");

// 	glutDisplayFunc(&DrawGLScene);
// 	glutIdleFunc(&DrawGLScene);
// 	glutReshapeFunc(&ReSizeGLScene);
// 	glutKeyboardFunc(&keyPressed);

// 	InitGL(1280, 480);

// 	glutMainLoop();

// 	return NULL;
// }

uint16_t t_gamma[2048];

void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
	int i;
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(&gl_backbuf_mutex);
	for (i=0; i<1280*1024; i++) {
	  depth_mid[i] = depth[i];
	}
	got_depth++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
}

void rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp)
{
	pthread_mutex_lock(&gl_backbuf_mutex);

	// swap buffers
	assert (rgb_back == rgb);
	rgb_back = rgb_mid;
	freenect_set_video_buffer(dev, rgb_back);
	rgb_mid = (uint8_t*)rgb;

	got_rgb++;
	pthread_cond_signal(&gl_frame_cond);
	pthread_mutex_unlock(&gl_backbuf_mutex);
}

void *freenect_threadfunc(void *arg)
{
  printf("1\n");
	freenect_set_tilt_degs(f_dev,freenect_angle);
  printf("2\n");
	freenect_set_led(f_dev,LED_RED);
  printf("3\n");
	fflush(stdout);
	freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_video_callback(f_dev, rgb_cb);
	freenect_set_video_mode(f_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, current_format));
	freenect_set_depth_mode(f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
	freenect_set_video_buffer(f_dev, rgb_back);

	freenect_start_depth(f_dev);
	freenect_start_video(f_dev);

	//	printf("'w'-tilt up, 's'-level, 'x'-tilt down, '0'-'6'-select LED mode, 'f'-video format\n");

	
	while (!die && freenect_process_events(f_ctx) >= 0) {
		freenect_raw_tilt_state* state;
		freenect_update_tilt_state(f_dev);
		state = freenect_get_tilt_state(f_dev);
		double dx,dy,dz;
		freenect_get_mks_accel(state, &dx, &dy, &dz);
		//		printf("\r raw acceleration: %4d %4d %4d  mks acceleration: %4f %4f %4f", state->accelerometer_x, state->accelerometer_y, state->accelerometer_z, dx, dy, dz);
				fflush(stdout);
	

		if (requested_format != current_format) {
			freenect_stop_video(f_dev);
			freenect_set_video_mode(f_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, requested_format));
			freenect_start_video(f_dev);
			current_format = requested_format;
		}
	}

	printf("\nshutting down streams...\n");

	freenect_stop_depth(f_dev);
	freenect_stop_video(f_dev);

	freenect_close_device(f_dev);
	freenect_shutdown(f_ctx);

	printf("-- done!\n");
	g_state=0;
	die = 0;
	return NULL;
}

void print_usage()
{
  printf("usage simple_kinect_mex( [ c ] )  where c is an optional character:\n");
  printf("'' init or get depth and color image\n");
  printf("'q' stop kinect thread\n");
  printf("'w' angle kinect up\n");
  printf("'x' angle kinect down\n");
  printf("'s' level kinect\n");
  printf("'R' video format RGB (default)");
  printf("'Y' video format YUV_RGB");
  printf("'I' video format IR");
  printf("'1' set kinect led GREEN\n");
  printf("'2' set kinect led RED\n");
  printf("'3' set kinect led YELLOW\n");
  printf("'4' set kinect led BLINK YELLOW\n");
  printf("'5' set kinect led BLINK GREEN\n");
  printf("'6' set kinect led BLINK RED YELLOW\n");
  printf("'7' set kinect led OFF\n");
  printf("---------------------------------------\n");
}


// 3 640 480
#define ROWS 921600
#define COLUMNS 1
#define ELEMENTS 921600

// 640 480
#define DROWS 307200
#define DCOLUMNS 1
#define DELEMENTS 307200

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    uint16_t *depth_data;
    uint8_t *rgb_data; 
    mwSize index;

	/* Check for proper number of arguments. */

    depth_data = (uint16_t*)mxCalloc(DELEMENTS, sizeof(uint16_t));
    rgb_data = (uint8_t*)mxCalloc(ELEMENTS, sizeof(uint8_t));
    //    for ( index = 0; index < ELEMENTS; index++ ) {
    //        dynamicData[index] = data[index];
    //    }

    plhs[0] = mxCreateNumericMatrix(0, 0, mxUINT16_CLASS, mxREAL);
    plhs[1] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);

    mxSetData(plhs[0], depth_data);
    mxSetM(plhs[0], DROWS);
    mxSetN(plhs[0], DCOLUMNS);

    mxSetData(plhs[1], rgb_data);
    mxSetM(plhs[1], ROWS);
    mxSetN(plhs[1], COLUMNS);


    if ( nrhs >1  ) {
      print_usage();
      return;
    } 

    //      printf("in kinect_mex\n");

    if (0==g_state){ /* do init */
      printf("initializing libfreenect\n");
      g_state=1;
      depth_mid = (uint16_t*)malloc(640*480*3*sizeof(uint16_t));
      depth_front = (uint16_t*)malloc(640*480*3*sizeof(uint16_t));
      rgb_back = (uint8_t*)malloc(640*480*3);
      rgb_mid = (uint8_t*)malloc(640*480*3);
      rgb_front = (uint8_t*)malloc(640*480*3);
      
      printf("Kinect camera test\n");
      
      int i;
      for (i=0; i<2048; i++) {
	float v = i/2048.0;
	v = powf(v, 3)* 6;
	t_gamma[i] = v*6*256;
	}
      
      
      if (freenect_init(&f_ctx, NULL) < 0) {
	printf("freenect_init() failed\n");
	return;
      }
      
      //      freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);
      
      int nr_devices = freenect_num_devices (f_ctx);
      printf ("Number of devices found: %d\n", nr_devices);
      
      int user_device_number = 0;
      
      printf("opening device...\n");
      if (nr_devices < 1){
	g_state=0;
	return;
      }
      printf("opening device...\n");
      if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
	printf("Could not open device\n");
	return;
      }
      int res;
      printf("starting thread...\n");
      res = pthread_create(&freenect_thread, NULL, freenect_threadfunc, NULL);
      if (res) {
	printf("pthread_create failed\n");
	return;
      }
      printf("returning after starting thread...\n");
      return;
      
    }

    //      printf("checking variables\n");

    if ( nrhs > 0 ){
      if ( mxIsChar(prhs[0]) != 1){
	print_usage();
	return;
      }
      if ((mxGetM(prhs[0])!=1)||(mxGetN(prhs[0])!=1)){
	; //normal usage
      }else{
	mxChar c = *(mxGetChars(prhs[0]));
	//	printf("char is %c\n",c);
	switch ( c ){
	  case 'q':
	    die=1;
	    //	    printf("dying\n");
            break;
	  case 'w':
	    freenect_angle++;
	    if (freenect_angle > 30) {
	      freenect_angle = 30;
	    }
	    freenect_set_tilt_degs(f_dev,freenect_angle);
	    break;
	  case 'x':
	    freenect_angle--;
	    if (freenect_angle < -30) {
	      freenect_angle = -30;
	    }
	    freenect_set_tilt_degs(f_dev,freenect_angle);
	    break;
	  case 's':
	    freenect_angle=0;
	    freenect_set_tilt_degs(f_dev,freenect_angle);
	    break;
	  case 'I':
            requested_format = FREENECT_VIDEO_IR_8BIT;
            break;
	  case 'R':
            requested_format = FREENECT_VIDEO_RGB;
            break;
	  case 'Y':
            requested_format = FREENECT_VIDEO_YUV_RGB;
            break;
	  case '1':
	    freenect_set_led(f_dev,LED_GREEN);
            break;
	  case '2':
	    freenect_set_led(f_dev,LED_RED);
            break;
	  case '3':
	    freenect_set_led(f_dev,LED_YELLOW);
            break;
	  case '4':
	    freenect_set_led(f_dev,LED_BLINK_GREEN);
            break;
	  case '5':
	    freenect_set_led(f_dev,LED_BLINK_RED_YELLOW);
            break;
	  case '0':
	    freenect_set_led(f_dev,LED_OFF);
            break;
	}
      }
    }


    //    printf("before update\n");
    Update();
    //    printf("after update\n");

    //    printf("copying data\n");
    for ( index = 0; index < DELEMENTS; index++ ) {
        depth_data[index] = depth_mid[index];
    }

    //    printf("copied data 1\n");

    if ( FREENECT_VIDEO_IR_8BIT==current_format ){
      //      printf("changing sizes\n");
      mxFree(rgb_data);
      rgb_data = (uint8_t*)mxCalloc(DELEMENTS, sizeof(uint8_t));
      mxSetData(plhs[1], rgb_data);
      mxSetM(plhs[1], DROWS);
      mxSetN(plhs[1], DCOLUMNS);
      
      for ( index = 0; index < DELEMENTS; index++ ) {
        rgb_data[index] = rgb_mid[index];
      }
      //      printf("copied data 2.1\n");
    }else{
      for ( index = 0; index < ELEMENTS; index++ ) {
        rgb_data[index] = rgb_mid[index];
      }
      //      printf("copied data 2.2\n");
    }

    //    printf("returning\n");
    return;
}


/* 
Previous contributors before Alex Berg (all for the OpenKinect project)

commit bdd9219144ddd7b3ed94c9f64f1d80415d13a61c
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Dec 12 02:54:54 2010 +0100

    Add CONTRIB file and mkcontrib.sh
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 6c3cf934bdc019520017b0a2082ac6e98d9c2521
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Dec 8 20:09:34 2010 +0100

    Rework streaming to support compressed streams, cleanup
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit c6ed7fe41478d1728da581fd55ebb2c3554e97cd
Author: Kai Ritterbusch <kai.ritterbusch@gmx.de>
Date:   Wed Dec 8 18:27:37 2010 +0100

    Fix std::map::at issue on OSX
    
    Problem: std::map::at not recognized by OSX compiler
    Fix: changed to std::map::operator[] (I think according to C++ standard:)
    
    Signed-off-by: Kai Ritterbusch <kai.ritterbusch@gmx.de>

commit 0ba42a4bab21367f50ebf480fd6bc07b5c34a5b8
Author: Kai Ritterbusch <kai.ritterbusch@gmx.de>
Date:   Tue Dec 7 17:44:24 2010 +0100

    Fixed libfreenect.hpp: check of return values now according to convention
    
    Signed-off-by: Kai Ritterbusch <kai.ritterbusch@gmx.de>

commit 90e839e17bc675eb2f6a8b9121b06b3a87ed63d8
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Mon Dec 6 17:35:40 2010 -0500

    1. Fixed compiler warning in fakenect, libfreenect.hpp, and c_sync
    2. Implemented start/stop for video/depth in fakenect
    3. Ran astyle
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 63936ec8e8d92ba23736e4d03dfdbebc09ef39c1
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Dec 8 19:42:41 2010 +0100

    Move -Wall CFLAGS to toplevel and change the way it works
    
    This should let env CFLAGS override -O2 while forcing -Wall
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit f550f3aae46668fd5ab83af9eb52ea9ff60765a0
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Dec 6 19:50:25 2010 -0800

    Removing duplicate udev rules
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit eb3b04c1ab3e307cb27ea26085b998bc8c9364e4
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Mon Dec 6 17:35:40 2010 -0500

    1. Made glpclview not go "ape-shit" when no kinect is found
    2. cvdemo and glpclview give nicer messages when no kinect is found
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 325095110aba087099741e578e3de14882f71011
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Mon Dec 6 00:58:19 2010 -0500

    1. Updated libfreenect_sync to support changing kinects and video types
    2. Updated python wrappers to use new libfreenect_sync
    3. Fixed a compile warning in fakenect
    4. Updated python demos to match new wrapper
    5. Added new python demo for multiple kinects (currently only uses one at a time)
    6. Added opencv wrappers (credit: Andrew <amiller@dappervision.com>)
    7. Added nogil to sync python calls (credit: Mathieu Virbel <mathieu@pymt.eu>)
    8. Updated glpclview to use new sync interface, updated calibration parameters (credit: Andrew <amiller@dappervision.com>)
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 1c0ddd6a37573ab418bbed37df645621e8d9563b
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 05:14:50 2010 +0100

    Rename YUV size constant and add YUV_RGB one for consistency
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 82d7b6b92f8e1a3afe404025bede6dc00c5452a4
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 05:00:39 2010 +0100

    glpclview: Don't crash when init fails, just keep retrying
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 50db38bf2ed7df6f834bb287cbbbb8b72bcceb4a
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:53:53 2010 +0100

    glpclview: Shut down the kinect on exit
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 39da995cdcca8a9f45a203f73720e0bae4a5b4bb
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:51:09 2010 +0100

    glpclview: Set point size to 2.0 for a smoother image
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 514d26eaf3b59ddcc38e9ab52b8898155787a8ef
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:50:24 2010 +0100

    glpclview: Fix typing issues
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit ca9b389a23978d7cd3491df87347bdcae51d0df3
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:38:33 2010 +0100

    c_sync: Fix multiple issues
    
    - Make globals static
    - Don't crash if initialization fails (no device)
    - Functions with no args should be f(void)
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 93feb07f079275d5a23aa640a7c5cd504f2ac0ed
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:32:21 2010 +0100

    fakenect: Fix indentation
    
    Please stick to the coding style documented in HACKING.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit b0da516c60b17ee84226ca07dca222cc3f7b35a6
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:27:56 2010 +0100

    c_sync: Fix indentation
    
    Please stick to the coding style documented in HACKING.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit f43b01954a8439fe298169088f34bc10a59162a3
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Dec 6 04:25:16 2010 +0100

    glpclview: Fix indentation
    
    Please stick to the coding style documented in HACKING.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 50d90f00db240dea86eb39b0aebd0810ee26bbf8
Author: Mark Renouf <mark.renouf@gmail.com>
Date:   Sat Dec 4 14:34:12 2010 -0500

    Created astyle config to match the current freenect C code style
    
    Signed-off-by: Mark Renouf <mark.renouf@gmail.com>

commit a19a1a194bd885f777961667862ddb6f9344d96e
Author: Drew Fisher <drew.m.fisher@gmail.com>
Date:   Fri Dec 3 04:47:22 2010 -0800

    Add UYVY image format support.
    
    Since UYVY uses twice the bandwidth for a given resolution as the bayer
    pattern, we have to drop the video camera framerate to 15Hz to get a
    640x480 image.  Since depth and RGB no longer update anywhere near
    lock-step, the pthread_cond_wait no longer waits for both images to be
    updated.
    
    Signed-off-by: Drew Fisher <drew.m.fisher@gmail.com> (zarvox)

commit f382dc081cd06284fe1353948ce40abcc6990886
Author: Andrew <amiller@dappervision.com>
Date:   Sat Dec 4 01:34:15 2010 -0500

    Added RGB color alignment to the GlPclView demo. It's not perfect, but it's a reasonable start using earlier published values.
    
    Signed-off-by: Andrew <amiller@dappervision.com>

commit b7e589249b68f77fe9248b09c2f9445e2c96d4b2
Author: Andrew <amiller@dappervision.com>
Date:   Thu Dec 2 13:26:22 2010 -0500

    Added extern C to the freenect_sync.h
    Signed-off-by: Andrew <amiller@dappervision.com>

commit c46e39c0243d9b4f921d01cff313eb1fcbb4aaa3
Author: Andrew <amiller@dappervision.com>
Date:   Thu Dec 2 13:15:04 2010 -0500

    Added a new demo! This is a 3D point cloud viewer in under 200 lines of C. It's modeled after glview.c, but it uses the c_sync api and removes a lot of clutter. Consider it a supplement rather than a replacement to glview.
    Signed-off-by: Andrew <amiller@dappervision.com>

commit 70eb87e25be672371acbb6b22e1c25eba5b4ad36
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Sat Dec 4 10:52:14 2010 -0800

    Fixed issue with PTHREAD_MUTEX_INITIALIZE not working in object constructors on OS X gcc
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 3a24bb86921ee1267902759684d8ce02fc13916d
Author: Kai Ritterbusch <kai@kai.(none)>
Date:   Wed Dec 1 15:55:59 2010 +0100

    Fixed libfreenect.hpp: made FreenectDeviceState copyable
    Added cppview.cpp to examples, as an example for the C++ header
    
    Signed-off-by: Kai Ritterbusch <kai.ritterbusch@gmx.de>
    Signed-off-by: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>

commit 2402fcdbe95f2e898c0fe7e8618995195ee746b5
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Wed Dec 1 01:36:19 2010 -0600

    Server depth format fix. RGB/DEPTH FIX, images should look OK now in client.
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit ffdaebb6c56347ab96a03968920897bdbe74ff33
Author: Andrew <amiller@dappervision.com>
Date:   Sun Nov 28 20:48:54 2010 -0500

    Updated the README with a ./configure fix for libusb on OX
    
    Signed-off-by: Andrew <amiller@dappervision.com>

commit 7e802d125af43766d20cfe15de883608ac96b106
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Wed Dec 1 01:03:04 2010 -0500

    1. Updated more RGB->Video stuff.
    2. Changed C sync to use void *'s as some wrappers (MATLAB) treat strings in a special way
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 99306000709b52b45066587d59b9e35605789f8e
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Tue Nov 30 21:02:18 2010 -0800

    Fixes to AS3 server for new naming conventions
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 806a90d816d9ffe3a5c6191e83c74572f4ed924c
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Tue Nov 30 20:49:09 2010 -0800

    Added missing stdlib include to glview so compilers would stop complaining about malloc
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit ac2124e8567d8e6be6824fc58f7e9eab47c7bbee
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 07:14:04 2010 +0100

    Add IR video option to glview
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 334c1eea41bf7930c603f9ebcc9a3a7155a51958
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 07:13:54 2010 +0100

    Support IR video stream
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 9c2f4f4af33c633c983243f10eaf15436e46655a
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 06:35:09 2010 +0100

    Get rid of freenect_[packed_]depth
    
    We aren't going to have one typedef per format, and they don't really
    work well for packed ones anyway, so let's get rid of them.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 7ad5499a96a3a1e7f7615b4a5b9f719f7c70cf6d
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 06:12:13 2010 +0100

    Rename *rgb* functions to *video* + internals
    
    Since we'll be adding IR support, make it consistent.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>
    
    More changes to typenames after merge
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>
    
    Fixed function name
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit b0e7440690bd46cba5edf0472d06ef093fb352e1
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 05:33:04 2010 +0100

    Rename format/size constants and enums
    
    In preparation for IR stuff, since it's becoming a bit inconsistent. New
    convention:
    
    FREENECT_(CLASS)_(FORMAT)[_PACKED][_SIZE]
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 3cd680513a30432ac9a426d98118b67272e9dad6
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 30 03:56:11 2010 +0100

    Make libfreenect endian-independent
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 55a55f0fd8f90a76aea807355e8534c9ef1e1d07
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 30 03:18:31 2010 +0100

    Move tilt code back into tilt.c and rename device_state -> tilt_state
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit c3d165b6ddaa431c2d390fea9718231922d280bc
Author: Andrew <amiller@dappervision.com>
Date:   Mon Nov 29 14:54:18 2010 -0500

    Added a wrappers section to the readme
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit af369a665ff1d62a92b1767c6119abc286c64187
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Tue Nov 30 18:30:51 2010 -0600

    Accelerometer fixes to work with new structure.
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit e91d4c5b01fe8f62fd8a6ff616c0a9d7f75be3bd
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Mon Nov 29 12:56:41 2010 -0500

    1. Fixed fakenect to support the new set_rgb/depth_buffer stuff which fixes support for glview
    2. Made fakenect use internal linkage for all internal stuff, this now allows you to do the craziest test of running record to get a dump, then using fakenect to replay the dump to record again.
    3. Minor Python readme fix for troubleshooting.
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit b1aa98b7dda289d1ea2b656c4293443953b11650
Author: Hector Martin <hector@marcansoft.com>
Date:   Sun Nov 28 18:49:15 2010 +0100

    Move bayer to RGB to a separate function
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit bac69ea85c73f9f25b5a024dec589a7b6f750b6a
Author: Jochen Kerdels <jochen@kerdels.de>
Date:   Sun Nov 28 13:01:39 2010 +0100

    Added slightly faster bayer pattern to rgb conversion
    
    Signed-off-by: Jochen Kerdels <jochen@kerdels.de>

commit 5dcf307ea94d5c2f614333272388567114f23f89
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 23:35:14 2010 +0100

    Use new buffer stuff in glview
    
    This gets rid of most memcpys, swapping buffers instead
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 1f2859a5bab6609d4ea53eb190ed52211b7ac62e
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 23:34:15 2010 +0100

    Add support for setting user-controlled buffers
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 4bc1925f69f4ce25a32f9bc2362537e34f68e5ba
Author: Andrew <amiller@dappervision.com>
Date:   Fri Nov 26 21:58:05 2010 -0500

    Fixed demo sync and renamed it to demo_ipython
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit d2dadc840f9f6e60d95c623d0974f79c3f176d9c
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Sun Nov 28 00:36:27 2010 -0500

    Updated readme
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit d4bbfb1d90b1cadf7c0855b663262709e8bca2f2
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 23:34:41 2010 -0500

    Typo
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 8537f56ea0138e7ddf5039863545ae21798fd7bc
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 23:29:34 2010 -0500

    1. Changed from exposing strings to just using numpy as practically the strings are useless in python and they make it messier to use numpy.
    2. Added a 'body' function pointer to the runloop that is called between event checks
    3. Added a 'Kill' exception that can be used to disable the runloop from inside the body
    4. Updated all demos
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit e0f3458bb30acbc893c08b099668ce89ba1327c3
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 22:34:40 2010 -0500

    Fixed tilt demo.  Added a new "body" function pointer to the runloop that lets you call things between usb events
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 241d768de857db6681c91f49835c1285d9c04518
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 22:16:15 2010 -0500

    Working on demo tilt
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 154c3fcb6a9dcd1f1cb6498b10cc6e4dadc47417
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 22:00:30 2010 -0500

    Removed the python Ctypes code and added new accelerometer hooks
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit b5798b04b4cf52de7be5ba6fb9a5b395ac787336
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 22:40:08 2010 +0100

    Refactor stream buffering and move bufs to heap
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit e27c19f4a0120bac74523b93a92c3a56770f676d
Author: Jochen Kerdels <jochen@kerdels.de>
Date:   Sun Nov 28 00:01:02 2010 +0100

    Optimized unpacking of depth data regarding speed and memory boundaries
    
    Signed-off-by: Jochen Kerdels <jochen@kerdels.de>

commit bf72ae6ea63631dcba25d9e710db91b5b1f5798c
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 21:51:40 2010 +0100

    Fix broken demosaicing and out of bounds buffer access
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit c8211fdda9bc301e57cc41887e17abfc0ad95600
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 21:42:05 2010 +0100

    Fix out of bounds memory access in depth unpacking
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 767c49b18cacfd29baa70fe2d8195f31fd3eb8de
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 21:37:36 2010 +0100

    Free libusb transfers (fix memory leak)
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit d5b86cd142a155b25bc3eaebf26e0132f4225493
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 21:11:56 2010 +0100

    Move packed depth buffer sizes to public header
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 02215af74e117b8e60bf72acd554cd94bef1e23a
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 21:09:51 2010 +0100

    Reject format changes when streams are active
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 33184ded49ffc55416f5999a75c26ba8e3597053
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 20:55:54 2010 -0500

    Cleaned up python demos
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 3ea69a0686833427ab289461647a8994c2d1263e
Author: Andrew <amiller@dappervision.com>
Date:   Fri Nov 26 18:58:21 2010 -0500

    Include directory for libusb-1.0 in the main cmakelists.txt
    Fixed a typo for sync rgb getting
    Fixed a typo in sync_get_rgb that made it use depth data instead of  rgb
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit 16a4b503265cfd66871a27b8e0ab269fe9ad2ba5
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 18:27:26 2010 -0500

    Minor cleanups
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit f88c651ed2b88c9d5837869a5d9f5c21e0db1024
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 18:15:03 2010 -0500

    Readme
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 0fcbe9b2d66e83afaa475f983161214415229f31
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 18:00:04 2010 -0500

    Fixed fakenect to work with new interface
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 1abe902c5a4256a9f2bcb1d9f027ac04f1583cdf
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 03:19:59 2010 -0500

    Changed path
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit a92e0a76a728367826f03b1d8cbba019b2a4067a
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 03:05:27 2010 -0500

    Fixed having to use /usr/local/include
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 9d7b9c7b3f27cc666b01bdf9b5692489a3864c14
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 02:36:51 2010 -0500

    Added the python header
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 2a6ceeb1d7cd287bf8db98b3ae9834cc375e4373
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 02:16:34 2010 -0500

    Added demos as symlinks
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit bc42d54cc59b75a49df0d86809ddd5df41ed49a4
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 02:08:29 2010 -0500

    Moved ctypes version to it's own directory, cython is now the preferred version
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit ee2cd335c692a2f412984266293a71b5ea4b42ae
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 01:55:28 2010 -0500

    Removed freenect_sync.py as it is out of date with out current API, it will be added back in a merged form in the freenect.py.  The full functionality is available in the cython version

commit 5e00ab89f613b6ab1d2de43ed90bab9a4503059a
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Fri Nov 26 01:54:26 2010 -0500

    Combined cython_sync and cython
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 592380f64579baa2a20e0a4fb05ec79e8bb3393d
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 23:48:19 2010 -0500

    Missing file
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 23f639f23d0b5544dcaf55437fbfdc6bf51d5b08
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 23:17:30 2010 -0500

    Copyright notices
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit d1e7bcf24e3343f6b23c759d7bb2bf72158e3fa1
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 22:56:46 2010 -0500

    Changed to new interface to allow for getting timestamps
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 2c5ffe0d20325d9753e06ea48cf7258a72fb5cdb
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 22:26:37 2010 -0500

    Updated readmes
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit ea39d2d8cf6417c41acef02e825caf26da876fda
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 22:20:34 2010 -0500

    Updated cython_sync to use the new c_sync interface (also intended to test it)
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 98a1c209dfc6d6eb65703690de37c0bad2678b18
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Thu Nov 25 22:07:08 2010 -0500

    Added into build system
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 7062c42d0784695c4341d9972eb7d513658213db
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Wed Nov 24 01:42:28 2010 -0500

    Initial freenect synchronous wrapper with internal thread.  This is derived from amiller's cython_sync code.
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    This works, spawns a new thread
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added a way to stop the runloop (the shutdown functions are needed from the main lib though)
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added a kinect record function
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added a readme
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added fakenect and a cython demo.  Need to integrate in the build system next
    
    Updated build, readme
    
    Updated to use pgm and ppm
    
    Now can properly display the PGM/PPM format
    
    Use environmental variable now, dynamic linking, prevent overwriting directory, fixed getline as OSX doesn't have it
    
    Integrated into build system

commit 43a4f8b1aab3ba676c5bcd287c2c0fea1e1846b4
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 27 02:08:37 2010 +0100

    Fix dropped packet count calculation
    
    Josh please don't kill me :(
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 0aa83230cdc7690455343049e11d3f0b4323e5c3
Author: Stéphane Magnenat <stephane@magnenat.net>
Date:   Tue Nov 23 18:47:24 2010 +0100

    Added state retrieval for accelerometer and tilt position/status.
    User can now choose when they want to update the state of the device and the
    result is cached in freenect_raw_device_state.
    
    Signed-off-by: Stéphane Magnenat <stephane@magnenat.net>

commit ca98e628599dae5afdcc3f8a5ba3459a311a9beb
Author: Drew Fisher <drew.m.fisher@gmail.com>
Date:   Wed Nov 24 18:08:33 2010 -0800

    Remove unreferenced cmake modules.
    
    Signed-off-by: Drew Fisher <drew.m.fisher@gmail.com> (zarvox)

commit d4f08c28006dc769cfeb441de6377414d3735c9f
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 24 18:36:46 2010 +0100

    Implement library and device shutdown and clean up USB open somewhat
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 539b28e67c60237581400f2acd17d569a007fae9
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Tue Nov 23 17:55:09 2010 -0600

    Server Fixes. Data in/out threaded :). Client Fixes after clean UP.
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit a130eee115a34ccdc2487036fa2b837015cb33df
Author: Andrew <amiller@dappervision.com>
Date:   Tue Nov 23 15:36:59 2010 -0500

    Reverted back the async version of the async cython. I only meant to update the setup.py!
    Updated the README to fix a typo and old static file info
    Fixed the reference to freenect build
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit bef469a59434782d399ace7476dc29f94c6ad423
Author: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>
Date:   Tue Nov 23 10:40:22 2010 +0100

    Fixed C++ binding according to new depth callback prototype
    
    Signed-off-by: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>

commit 3d172d3ac5a330200c8b0f868f53afeb0ee8e8c6
Author: Andrew <amiller@dappervision.com>
Date:   Tue Nov 23 13:23:09 2010 -0500

    Updated the cython binding with GIL releasing callbacks and a synchronous api
    Updated the readme to reflect the new api. Fixed the setup.py paths for the new build layout
    Moved cython_sync to its own directory and restored cython
    Fixed the setup.py to refer to the new builds
    Updated the cython binding with GIL releasing callbacks and a synchronous api
    Updated the readme to reflect the new api. Fixed the setup.py paths for the new build layout
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit d969c0fc75dbca8d3c414907cee4181d95ff41e0
Author: Rich <richmattes@gmail.com>
Date:   Tue Nov 23 13:28:27 2010 -0500

    Small changes:
    - Make a CMake option for the CPack stuff, default to OFF
    - Update CPack paths to match project install paths
    - Update pkgconfig.in file to use the same install paths the project uses
    
    Signed-off-by: Rich Mattes <jpgr87@gmail.com>

commit 786298617cb61fa060c7c9196b3e080871a41f81
Author: Drew Fisher <drew.m.fisher@gmail.com>
Date:   Tue Nov 23 00:34:43 2010 -0800

    Fix CPack after massive cmake cleanup.
    
    Signed-off-by: Drew Fisher <drew.m.fisher@gmail.com> (zarvox)

commit 9aba304481603fed01227b5ff388371977eb82db
Author: Mark Renouf <mark.renouf@gmail.com>
Date:   Tue Nov 23 09:21:31 2010 -0500

    cmake: Configure and install a pkg-config file on Linux
    
    Signed-off-by: Mark Renouf <mark.renouf@gmail.com>

commit 57e7edf108e20811224ce600e8bd5c86024103ee
Author: Mark Renouf <mark.renouf@gmail.com>
Date:   Tue Nov 23 08:09:47 2010 -0500

    Fix file paths for installing docs on Linux
    
    Signed-off-by: Mark Renouf <mark.renouf@gmail.com>

commit dcf3c7980c1d291e05458f52db89b639ad6b2ba3
Author: Dominick D'Aniello <netpro2k@gmail.com>
Date:   Tue Nov 23 04:36:51 2010 -0500

    Add Mac .DS_Store files to .gitignore
    
    Signed-off-by: Dominick D'Aniello <netpro2k@gmail.com> (netpro2k)

commit ccbb4e1559c712dcc578cddec0e1ef1707aa7875
Author: Dominick D'Aniello <netpro2k@gmail.com>
Date:   Tue Nov 23 04:35:01 2010 -0500

    Update Homebrew Formula for new repository structure
    
    Signed-off-by: Dominick D'Aniello <netpro2k@gmail.com> (netpro2k)

commit c9731936cd54bc153027170cad52c8b4aed013c7
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Tue Nov 23 01:47:31 2010 -0600

    Cleaning up!
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit 12a39e384b625ebcaf5c7066cacb773356008b85
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 23:47:10 2010 -0800

    Cleaned up the README finally.
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 863532633986cd140965bd5100db6b16659c46f3
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:37:39 2010 -0800

    Moved udev into linux platform directory
    
      Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 8addbcb486849f81b571e7f28a02b3e4ed77768e
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:58:20 2010 -0800

    Added actionscript builder file
    Fixed includes in as3-server since it doesn't need libusb
    Fixed CMakeLists
    
      Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit c1ca60db7af7a04f60b13bd557b0b9bd678c9244
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:55:04 2010 -0800

    Removed platform excludes for windows. Just assume we're on *nix or die for the moment.
    Scrubbed examples CMake
    Removed need for threads in library linking because it's not needed.
    Moved as3-server to its wrapper directory
    Added/cleaned up options
    
      Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 1b9bd9fb94c2e6de4307baf0c956b03ddc633ace
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:37:57 2010 -0800

    Setup output directories
    Indention fixing
    
      Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 142015579d43b6d947c78dbeaac910ea70841427
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:28:46 2010 -0800

    Updated to a libusb-1.0 find module we're sure works on most platforms (but could probably use pkg-config finder too, so not deleting old one)
    Updated files to look for libusb-1.0/libusb.h, since you want to find the root path, not the deep path.
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 1913bf7c1c13b7509ed2ba3274f1f51632af8b49
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 22:24:18 2010 -0800

    Lots of cmake scrubbing
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit dd9ad07f27fa9f1d4de4c205ade10c37040bb4af
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 21:56:45 2010 -0800

    Cleaning up the repo some
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 275117d356674f83da5fd04d409b849c5db6798b
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 21:52:28 2010 -0800

    Moved language wrappers to subdirectories
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit f99f5bf80caf2907dd385e2eaa8ebf559464c93a
Author: Mark Renouf <mark.renouf@gmail.com>
Date:   Tue Nov 23 00:56:42 2010 -0500

    Udev rule for Kinect USB
    
    Signed-off-by: Mark Renouf <mark.renouf@gmail.com>

commit 8123ae1f00efc1d5e45e83dbd10d9a8cd3f96113
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 23 06:46:41 2010 +0100

    Convert actionscript files from CR (ugh) to LF line endings
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit e030003837a71498ec1e4623cee4ce6fec22d59f
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 21:37:42 2010 -0800

    Moved concise apache 2 license to more descriptive filename
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit e96897466236ba9d648ae0656a1564d198c19fd4
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 21:37:11 2010 -0800

    Removed doubled up apache license
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit c8613d9554da3c63ea1df2b3c3ac4058dd49bd47
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Mon Nov 22 04:53:59 2010 -0500

    Added back optional numpy module loading for ptone : )
    Initial cython commit
    readme
    Changed all python library names to the new convention
    
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Fixed a typo
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Removed headers.  Made the setup look for them.
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Updated readme with new path
    
    Added -fPIC
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Changed to use the shared libary, added help text
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added a new include path that is the default for OS x
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Back to static (dynamic worked but it will be annoying for devs)
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added another include search path for OSX
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 2d9dcd0bb2f2f443c0759b4f3b143c43854f0991
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 22 21:22:44 2010 -0800

    Added fPIC for linux building
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit ab647363685a109622dacc8952b76e10d6445cc0
Author: Rich <richmattes@gmail.com>
Date:   Mon Nov 22 15:17:41 2010 -0500

    Missed OpenGL Include directories from previous commit.
    
    Signed-off-by: Rich Mattes <jpgr87@gmail.com>

commit aa9a3bdfc979515aa45a9d676625377363d4493e
Author: Rich <richmattes@gmail.com>
Date:   Mon Nov 22 14:15:22 2010 -0500

    Fixed CMake error in glview: FindOpenGL.cmake (find_package(OpenGL)) sets variables of the form "OPENGL_LIBRARIES," not "OpenGL_LIBRARIES."
    
    Signed-off-by: Rich Mattes <jpgr87@gmail.com>

commit f3a8e29a08d536ba1e3f82b257f6071a998f26ad
Author: Dominick D'Aniello <netpro2k@gmail.com>
Date:   Mon Nov 22 22:38:19 2010 -0500

    Made homebrew script point to head with version "master" since we update too often to justify updating the formula every day
    
    Signed-off-by: Dominick D'Aniello <netpro2k@gmail.com> (netpro2k)

commit ba076a16be78d1605b2a07481dc9ebee7a87781d
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 23 05:04:28 2010 +0100

    Remove unnecessary libjpeg dependency
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 36a1e0011054b74e9a1448bde49191bb3db7c39d
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 23 04:22:45 2010 +0100

    glview: Switch Kinect stuff to secondary thread, GLUT to main
    
    Required to make GLUT work on OS X.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 3a73f930496dc3dfc28a9fab3b251a4f0100f6a2
Author: Hector Martin <hector@marcansoft.com>
Date:   Tue Nov 23 04:01:00 2010 +0100

    Disable RGB hflip after stream activation, otherwise it fails
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 9e74af830aecfd2cb75f7b3dae73cccc9edd347c
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 22 23:43:33 2010 +0100

    Support returning raw packed depth data
    
    This breaks API slightly, please adjust your depth callback prototype.
    The data is now passed as void* to accomodate both formats.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit d5d998585649e38c88007f2c1d13481ede8ec3bf
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 22 23:40:23 2010 +0100

    Depth unpacking macro -> inline function
    
    Static inline functions are as fast as macros on any decent compiler,
    and they're cleaner and provide better type checking.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 8f09a8c465300b41532b37d0ec2eae120d6a9d66
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 22 23:31:05 2010 +0100

    Squelch streaming errors until two valid frames arrive
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit f7f74dbb801a0ace3b7e39ed0da080fcd6581cbb
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 22 23:18:36 2010 +0100

    Add support for RGB/Depth stream shutdown
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 3574d9908a014128f4f77177ef7c3b9e70e0c0ea
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 22 22:59:51 2010 +0100

    Rewrite init code, split off depth and RGB inits
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit a0db7c8ac4571bfaa74bc32b52fac5476b9516c7
Author: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>
Date:   Mon Nov 22 10:48:59 2010 +0100

    Fixed CMake build errors and compilations warnings
    
    Signed-off-by: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>

commit 2d096ba907b9774421a19fec3a916b795ccb22f8
Author: Dominick D'Aniello <netpro2k@gmail.com>
Date:   Mon Nov 22 00:20:42 2010 -0500

    Added Homebrew formulas for easy install on OSX
    
    Signed-off-by: Dominick D'Aniello <netpro2k@gmail.com> (netpro2k)

commit 5f8acab237830da10191127c19856403e0a6b7c4
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Mon Nov 22 00:48:15 2010 -0600

    AS3 Server and Client
    
    Fix endiannes in as3 client. Motor control. Accelerometer data sent to as3.
    
    Server fixes.
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit bae0ec6a347936ffcb39a285d0f7bbe4780326aa
Author: Andrew <amiller@dappervision.com>
Date:   Sun Nov 21 22:50:58 2010 -0500

    Fixed README to point to the new location libfreenect/python
    
    Added #!/usr/bin/env python to the top of all python files
    
    Made the demos executable
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit fcd566ff957d364232e5678564748463e4948cb2
Author: Mark Renouf <mark.renouf@gmail.com>
Date:   Sun Nov 21 23:26:36 2010 -0500

    Added hash-bang to python scripts and made executable.
    
    Signed-off-by: Mark Renouf <mark.renouf@gmail.com>

commit 58067a23cdb597634aedddcee1cf7f255682f415
Author: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>
Date:   Fri Nov 19 19:40:51 2010 +0100

    Added C++ API
    
    Signed-off-by: Vincent Le Ligeour <yoda-jm@users.sourceforge.net>

commit 128028f406547bd30b36d5a604a936ec5afb5e7b
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Sun Nov 21 19:28:31 2010 -0800

    Moved synchronous demos to new python directory
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 4977755b03f66c6a0984d41e82189d20335a2191
Author: Andrew <amiller@dappervision.com>
Date:   Sun Nov 21 20:28:55 2010 -0500

    Added a synchronous wrapper to the python library. Includes a demo (requires opencv and numpy).
    
    Signed-off-by: Andrew Miller <amiller@dappervision.com>

commit 07f4754d6117348133c3fe90507a5c6e0a4faa94
Author: Kelvie Wong <kelvie@ieee.org>
Date:   Sat Nov 20 16:59:23 2010 -0800

    Manually wrap lines in the README.
    
    This makes it easlier to read in the terminal.
    
    Signed-off-by: Kelvie Wong <kelvie@ieee.org>

commit 7bcd52a804d46518a6667d8bb41c331b0e010ff9
Author: Kelvie Wong <kelvie@ieee.org>
Date:   Sat Nov 20 16:51:09 2010 -0800

    Move the python bindings away from the 'c' directory.
    
    Signed-off-by: Kelvie Wong <kelvie@ieee.org>

commit 299bf51e838928d6956cf74e0967f33cc5279435
Author: Kelvie Wong <kelvie@ieee.org>
Date:   Sat Nov 20 16:49:21 2010 -0800

    Fix minor spelling errors.
    
    Signed-off-by: Kelvie Wong <kelvie@ieee.org>

commit 27454fe1ff7a83451ad7ab5f37723c091a16e8fa
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Sun Nov 21 13:17:08 2010 -0800

    Updated readme to include location of snapshot tarball for os x libusb
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 116570bf67f2185bdb7f0f8c1acc3accb2164896
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Sat Nov 20 15:53:10 2010 -0500

    Updated README to point to the new python bindings
    
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit d464c65cfbae1fe24db439d2fb17b284bbd11d31
Author: Brandyn A. White <bwhite@dappervision.com>
Date:   Sat Nov 20 15:50:19 2010 -0500

    Removed older python bindings
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>

commit 3a19a617a5e57daf2c2d23e041e3c57ac11ca654
Author: Tully Foote <tfoote@willowgarage.com>
Date:   Sat Nov 20 00:08:35 2010 -0800

    commenting out uninstall target which doesn't use module paths correctly.  Ticketed at https://github.com/OpenKinect/libfreenect/issues/issue/44
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>
    
    proper fix for ticket 44.
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit a7fa6085028beb2753f5f107bf20bc21c49e492f
Author: Chris J (cryptk) <cryptk@gmail.com>
Date:   Sat Nov 20 07:33:59 2010 -0600

    Fixed the escape key... not sure how that happened...
    Signed-off-by: Chris J (cryptk) <cryptk@gmail.com>

commit 80688d83a9b0f91794cb74a5752a70c9528ea79d
Author: Chris J (cryptk) <cryptk@gmail.com>
Date:   Fri Nov 19 12:13:25 2010 -0600

    Better motor/LED demo
    Signed-off-by: Chris J (cryptk) <cryptk@gmail.com>

commit de686b9305c349fffe7a96e5beb2906795e8ed88
Author: brandyn <brandyn@router.(none)>
Date:   Sat Nov 20 01:13:28 2010 -0500

    Initial commit of ctypes freenect
    
    Signed-off-by: Brandyn A. White <bwhite@dappervision.com>
    
    Added freenect license
    
    Added freenect license
    
    Made the module namespace mimic the C library
    
    Added demos, cleaned up binding
    
    Added tilt demo and comments
    
    Updated readme
    
    Added opencv demo of depth
    
    Updated demos
    
    Cleaned up, moved to a better place
    
    Updated direction
    
    Cleaned up readme more, added a guard to the depth_show
    
    Added Ubuntu package commands
    
    Updated so that you don't need any additional setup aside from compiling the main project.  Also should work on OSX out of the box.
    
    OSX fix
    
    OSX agian
    
    OSx
    
    Added more paths to check
    
    Minor
    
    Modified so that numpy is not required.  Now only python is
    
    Fixed path
    
    Modified to track new header files and conventions

commit 552649b786669a16ce6ddf928e2f75552ecada7b
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 20 21:08:07 2010 +0100

    Add a logging subsystem and switch all printf() calls over to that
    
    Now you can select your logging level from just about nothing to
    ridiculous spam every time a packet is received.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit f6de20d83ac2b22bc1d7af14efdb738161f727a2
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 20 19:45:05 2010 +0100

    Update HACKING a bit and clean up some indentation in glview
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 68d949d999f00d291b31b927cdbde2ad6c65bc0b
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 20 19:42:01 2010 +0100

    Merge accelerometers.c and motors.c into tilt.c, clean up a bit
    
    We don't need two calls for degrees and radians for tilt, since everyone
    knows how to trivially convert between them.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 85f7d40a2c1a7e637e5321574a4b669ca69950f6
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 20 19:35:10 2010 +0100

    Add HACKING file with basic coding style guidelines
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 1e1d26157bde07835dc08639760e77c46c678fdb
Author: Hector Martin <hector@marcansoft.com>
Date:   Sat Nov 20 19:20:05 2010 +0100

    Formatting and whitespace fixes
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit ceb185892ad7b911797582cf3ed6f2b44a138537
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Sat Nov 20 00:08:34 2010 -0800

    Updating package maintainer to be mailing list for the moment. Probably not the best email to use for that, but it'll do for now.
    Updated version number configuration.
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 3e9b3a0f363230601168ddc21f9908a637922800
Author: arnebe <arne@alamut.de>
Date:   Tue Nov 16 13:50:58 2010 +0100

    Added support for building rpm/deb packages on linux
    
    Signed-off-by: arnebe <arne@alamut.de>

commit d5d95b0dcc738f97cf4d7bbf67a3a5228b0c64d6
Author: Kelvie Wong <kelvie@ieee.org>
Date:   Thu Nov 18 09:55:03 2010 -0800

    Add the initial python bindings.
    
    This initial implementation uses the ctypes library to make dynamic calls to the
    shared library.
    
    I don't think this is the most pythonic API to do this with, but it wraps most
    of the stuff we need it to.
    
    It's also named KinectCore because all it does is wrap the C library.  More
    user-friendly functions could be made in a "Kinect" class that uses this
    KinectCore class, in the future.
    
    Signed-off-by: Kelvie Wong <kelvie@ieee.org>

commit 881425789d37866f34a691f4fbcdc842b192da39
Author: Thomas Roefer <Thomas.Roefer@dfki.de>
Date:   Fri Nov 19 23:15:23 2010 -0800

    Fixed a memory leak in the MacOS version of libusb
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 57b8d64453d724452face2325bf09f7d33f449ed
Author: Steven Lovegrove <stevenlovegrove@gmail.com>
Date:   Thu Nov 18 16:20:45 2010 +0000

    Modified CSharp Tilt sample to demonstrate horizon following
    
    The Kinect tilt control is relative to the horizon. This update
    modifies the CSharp sample to demonstrate this.
    
    Signed-off-by: Steven Lovegrove <stevenlovegrove@gmail.com>

commit 626e3883c7beaeec812e5d9329fe2de0f8bfd3d7
Author: Steven <sl203@sharkey.(none)>
Date:   Thu Nov 18 03:12:48 2010 +0000

    Kinect Tilt argument is signed byte relative to horizontal. Updated CSharp lib.
    
    Argument to Kinect Tilt command is a signed byte, where 0 sets camera level,
    as must be internally described by the accelerometers.
    
    Signed-off-by: Steven Lovegrove <stevenlovegrove@gmail.com>

commit ffe24e0cf428b79906a8b40710bfa89f02ed806d
Author: Rich Mattes <jpgr87@gmail.com>
Date:   Fri Nov 12 20:29:38 2010 -0500

    Lots of new enhancements to buildsystem:
    - Set project version in root directory CMakeLists.txt
    - Added host OS and Architecture detection
    - Added install and uninstall targets for system-wide installation
    - Added support for building static and shared library, with soname versioning
    - Made CMake option to enable/disable building example application
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit ece9eef0a73f539da31d0a1aa23e161183fbe1c2
Author: Steven Lovegrove <stevenlovegrove@gmail.com>
Date:   Fri Nov 19 02:24:06 2010 +0000

    Support for 10bit depth buffer mode
    
    The Kinect supports different modes for returning depth data. From
    some experimentation, modifying the data of init 7 changes these
    modes. mode 0x02 is easy to interpret as a 10bit depth buffer mode
    which this commit adds support for.
    
    Signed-off-by: Steven Lovegrove <stevenlovegrove@gmail.com>

commit 0755600f17622e137f0f2d6f2eede5091041535a
Author: Melonee Wise <mwise@willowgarage.com>
Date:   Wed Nov 17 13:39:24 2010 -0800

    adding mks call for acceleraometer
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 7121f49fcdcbeb9b7084e114aab3db66a352103e
Author: Melonee Wise <mwise@willowgarage.com>
Date:   Wed Nov 17 10:53:32 2010 -0800

    adding example of setting led
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit ae716768f39401c7b10ee7943e543a38a4e7e2c2
Author: Melonee Wise <mwise@willowgarage.com>
Date:   Wed Nov 17 10:48:00 2010 -0800

    removing extra print and adding leveling the camera to the example
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 733b23640a831cddcb1baf69daad956067ccc7c9
Author: Stéphane Magnenat <stephane@magnenat.net>
Date:   Wed Nov 17 18:18:31 2010 +0800

    Added accelerometer support, fixed segfault in example code when no device is given in cmd line.
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 3167fd1c2d4ddbca6e925dbeb68c7d69c0732ff9
Author: Melonee Wise <mwise@willowgarage.com>
Date:   Tue Nov 16 22:14:47 2010 -0800

    adding ability to set LED color
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit bdae77dff283d0e3995188ccabe0eb8554ea0841
Author: Radu Bogdan Rusu <rusu@cs.tum.edu>
Date:   Tue Nov 16 20:41:38 2010 -0800

    finalized and tested the support for multiple cameras/motors
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit fefe9ede8ba1449d82aa15d49d47ceb028cbd9eb
Author: Radu Bogdan Rusu <rusu@cs.tum.edu>
Date:   Tue Nov 16 20:21:56 2010 -0800

    added support for multiple cameras. fixed camera indexing for open (),
    etc
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit ca0bb0e30e59e4dd29514bc444cc37ea59161f57
Author: Radu Bogdan Rusu <rusu@cs.tum.edu>
Date:   Tue Nov 16 20:21:03 2010 -0800

    test
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 606dbb84ea75ab0cf8d4120368e407b65f79dc33
Author: Melonee Wise <mwise@willowgarage.com>
Date:   Tue Nov 16 19:59:49 2010 -0800

    fixing prototypes
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 1d35e510c732830f6f2e1212fdc40b9d11acb97c
Author: Melonee Wise <mwise@bvo.willowgarage.com>
Date:   Tue Nov 16 19:56:17 2010 -0800

    adding set tilt functions to base library
    
    Signed-off-by: Tully Foote <tfoote@willowgarage.com>

commit 84e7c5506e835f125c7f8e113c968c64fecce55d
Author: Tully Foote <tfoote@willowgarage.com>
Date:   Tue Nov 16 19:43:02 2010 -0800

    switching to shared libs at Radu's request

commit 67c0d076abf40c7be5f041adc9b855326ccc4484
Author: Kenneth Johansson <ken@kenjo.org>
Date:   Wed Nov 17 04:46:50 2010 +0800

    Don't segfault if kinect not connected.
    
    Signed-off-by: Kenneth Johansson <kenneth@southpole.se>

commit 562a7b28eb7ab7d7f361d1d5f3e4396b4ffc9dca
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Tue Nov 16 09:29:31 2010 -0800

    Fix from roefer (https://github.com/roefer) to github issue 22 (https://github.com/OpenKinect/libfreenect/issues/#issue/22)
    
    Fixes issues with iso transfers in libusb, updates libusb patch
    Changes Packet/transfer numbers
    OS X now running at full 30fps
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 0015f13ab1cf0836eba53383b361215b03bceb43
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 15 23:57:44 2010 -0800

    Updated readme in the hopes of less linux package questions on IRC
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 50e17b9c317c8ba7a376e61021ed6a37c6168d25
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Mon Nov 15 23:32:13 2010 -0800

    Added asciidoc with protocol information, compiled from various sources
    
    Signed-off-by: Kyle Machulis <kyle@nonpolynomial.com>

commit 33db2f1bd5c14d60a98fef86244bb2f778d22824
Author: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
Date:   Mon Nov 15 23:23:24 2010 -0500

    glview: remove unused variables to fix warnings
    
    Signed-off-by: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>

commit 55b11e26d9b14793287e37ff487592e632943a57
Author: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
Date:   Mon Nov 15 23:22:18 2010 -0500

    Examples CMakeLists: add missing library
    
    Fixes build error on Fedora which requires explicit linking against libs
    of directly used symbols (powf in this case).
    
    Signed-off-by: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>

commit da6d96e27ac0ac15687d14aeb16a786881198415
Author: Juan Carlos del Valle <jc.ekinox@gmail.com>
Date:   Mon Nov 15 18:31:34 2010 -0600

    fix to work on osx
    
    Signed-off-by: Juan Carlos del Valle <jc.ekinox@gmail.com> (imekinox)

commit 420f5c63ec577022ff73fa08b42b9f0b32124134
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 15 22:14:46 2010 +0100

    New API, new streaming code, new demosaicing code, update gltest
    
    Now with less random buffer overflows on streaming, less random buffer
    overflows in the demosaicer, and actual bilinear demosaicing.
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 92ab7f37d08abbd6b610188edaca96a1db842311
Author: Hector Martin <hector@marcansoft.com>
Date:   Mon Nov 15 13:27:23 2010 +0100

    Add new license headers and files
    
    Signed-off-by: Hector Martin <hector@marcansoft.com>

commit 50fdac70f94934aa877a7af0ec63c71d4ccfc56e
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Fri Nov 12 22:57:53 2010 -0800

    Added extern guards for C++

commit 20296c873847dac83d02d622ded4cba42e86cc9f
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Fri Nov 12 19:55:55 2010 -0800

    Added patches to make OS X rendering slightly more stable.

commit a676d10a8a01fcbff8d29bcd9fbe07e8ed807ceb
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:58:55 2010 -0800

    Cmake instructions

commit 9bac9082f5538385e0f5ddf293e8967e120458f4
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:56:56 2010 -0800

    Updated README

commit c2a6ecbac9299fc4ebdc9f7cd4602dd3a31d2bc2
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:51:16 2010 -0800

    Added framework link calls to cmake in an amazingly jank fashion. But now is not the time for cmake prettiness.

commit 1617eddc13bacd4dd42ff3d5e03ebe09d02f148b
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:44:53 2010 -0800

    Fixed bug introduced while porting in OS X code with double initialization of GL thread

commit e1d5f3b5a4d1f12491a7c17872d1e47e781a2750
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:40:08 2010 -0800

    Moved modules into c for time being

commit 2ca90193ddf4319bc3fb64627272907223198524
Author: Theo Watson <theo@openframeworks.cc>
Date:   Thu Nov 11 19:34:37 2010 -0800

    Bug fixes (claim_interface call, buffer resizing) to get libfreenect C code working in OS X
    Patch to libusb-1.0 (repo head 7da756e09fd97efad2b35b5cee0e2b2550aac2cb) for getting libfreenect working in OS X
    
    Integrated by Kyle Machulis <kyle@nonpolynomial.com>

commit 545f8cbdb9e4ef2743d0e3fd32030bab58fa60ec
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 19:33:11 2010 -0800

    Moved inf to platform directory

commit 4e20bc1267f23c9bab00c5ef2947f761e18f52ec
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 18:23:34 2010 -0800

    Formatting

commit cd95adf2472ab9850a302d9da6d1623c64b2f534
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 18:22:41 2010 -0800

    Updated readme to asciidoc

commit 17b1402d87ed99762cc561c5cd00daa632ba8e33
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 18:21:59 2010 -0800

    Moved C stuff into c directory for the moment

commit f12cca94e93803e307800dfd2c4317451f4232cd
Merge: 3d1b42b 938a2d5
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 18:20:25 2010 -0800

    Merge remote branch 'marcan/master'
    
    Conflicts:
    	.gitignore
    	README.txt

commit 3d1b42bec2673f344822c8ff17f668d459f5eef4
Author: Kyle Machulis <kyle@nonpolynomial.com>
Date:   Thu Nov 11 18:18:51 2010 -0800

    Reshuffled csharp stuff into csharp directory

commit 7e8f676d0623073e2dfdeacef888ff196fc9d49a
Author: Joshua Blake <joshblake@gmail.com>
Date:   Thu Nov 11 02:15:59 2010 -0500

    Updated drivers

commit c80dfd3f24b0ab01e58a03faff6123238c578120
Author: Joshua Blake <joshblake@gmail.com>
Date:   Thu Nov 11 02:14:28 2010 -0500

    Added initial OpenKinect libraries

commit 7ff148fee06038c0abcc93c7668b3b97e7148d20
Author: Joshua Blake <joshblake@gmail.com>
Date:   Thu Nov 11 00:17:58 2010 -0500

    initial commit

commit 938a2d54f59e6c4488d0fed44c8588e9390a0fe8
Author: Hector Martin <hector@marcansoft.com>
Date:   Thu Nov 11 03:02:56 2010 +0100

    Be more verbose about async transfer submission

commit 92a7627aecdbfc294f3e458539508836a5861d2b
Author: Hector Martin <hector@marcansoft.com>
Date:   Thu Nov 11 02:46:42 2010 +0100

    Even more aggressive gamma correction

commit 5e30842729f92e0f29d4e95a20fe77f3704ee644
Author: Hector Martin <hector@marcansoft.com>
Date:   Thu Nov 11 02:44:10 2010 +0100

    Lock GL framerate to incoming data
    
    GL may drop frames now, but will not run faster than input data.

commit 405c2061a98252758c2092cec2090d1821a6b695
Author: Hector Martin <hector@marcansoft.com>
Date:   Thu Nov 11 02:43:27 2010 +0100

    Switch depth buffer to unshifted and add correction
    
    This uses some gamma correction and an offset to better map
    the depth values to a nice heat map style image.

commit 7cdeab714b42c994896e71c02f4a5f6d6e95fceb
Author: Hector Martin <hector@marcansoft.com>
Date:   Thu Nov 11 02:42:57 2010 +0100

    Switch to multipacket isoc transfers

commit d64b2f8aec91ccd0e8c65f7549561a5b947069c5
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 22:45:46 2010 +0100

    Fix geninits.py on Windows

commit 63eba92776d484ce278f334e3cb52348a267d012
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 19:53:05 2010 +0100

    Switch to a sane CMake-based buildsystem

commit 286e42ac76ac9d4eab11a82639d66def7858ce9f
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 13:53:35 2010 +0100

    README update

commit 630c5d8b542461bdfbcff76e36139070ce52a77e
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 13:41:53 2010 +0100

    Disable unnecessary init sequences
    
    Most of the init control requests don't appear to actually be required,
    so disable a whole bunch of them (more investigation needed)

commit 0fe680d9b71fcf64709c6b941bbebed41a9678ba
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 13:11:56 2010 +0100

    Add credits

commit 7655fcf7239ba4907654089dba535a196685dbe5
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 12:53:25 2010 +0100

    Add license

commit 4094151eb0a8eb71f24df9d204e04b89b1724ea1
Author: Hector Martin <hector@marcansoft.com>
Date:   Wed Nov 10 12:47:02 2010 +0100

    Initial commit
*/
