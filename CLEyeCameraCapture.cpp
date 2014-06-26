#include "stdafx.h"
#include "CLEyeCameraCapture.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"

namespace wdd{
	CLEyeCameraCapture::CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps) :
		_cameraGUID(cameraGUID), _cam(NULL), _mode(mode), _resolution(resolution), _fps(fps), _running(false), _visual(true)
	{
		strcpy_s(_windowName, windowName);

		if(resolution == CLEYE_QVGA)
		{
			_FRAME_WIDTH = 320;
			_FRAME_HEIGHT = 240;
		}
		else if(resolution == CLEYE_VGA)
		{
			_FRAME_WIDTH = 640;
			_FRAME_HEIGHT = 480;
		}
		else
		{
			std::cerr<<"ERROR! Unknown resolution format!"<<std::endl;
			exit(-11);
		}
	}
	bool CLEyeCameraCapture::StartCapture()
	{
		_running = true;
		
		cvNamedWindow(_windowName, CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO | CV_GUI_NORMAL);

		// Start CLEye image capture thread
		_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
		if(_hThread == NULL)
		{
			MessageBox(NULL,"Could not create capture thread","WaggleDanceDetector", MB_ICONEXCLAMATION);
			return false;
		}
		return true;
	}
	void CLEyeCameraCapture::StopCapture()
	{
		if(!_running)	return;
		_running = false;
		WaitForSingleObject(_hThread, 1000);
		cvDestroyWindow(_windowName);
	}
	void CLEyeCameraCapture::setVisual(bool visual)
	{
		_visual = visual;
	}
	void CLEyeCameraCapture::IncrementCameraParameter(int param)
	{
		if(!_cam)	return;
		CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)+10);
	}
	void CLEyeCameraCapture::DecrementCameraParameter(int param)
	{
		if(!_cam)	return;
		CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)-10);
	}
	void CLEyeCameraCapture::Run()
	{
		//
		//	Global: video configuration
		//
		int FRAME_RED_FAC = 1;

		//
		//	Layer 1: DotDetector Configuration
		//
		int DD_FREQ_MIN = 11;
		int DD_FREQ_MAX = 17;
		double DD_FREQ_STEP = 1;
		double DD_MIN_POTENTIAL = 16444*2;

		//
		//	Layer 2: Waggle SIGNAL Configuration
		//
		double WDD_SIGNAL_DD_MAXDISTANCE = 2.3;
		int		WDD_SIGNAL_MIN_CLUSTER_SIZE = 6;

		//
		//	Layer 3: Waggle Dance Configuration
		//
		double	WDD_DANCE_MAX_POSITION_DISTANCEE = sqrt(2);
		int		WDD_DANCE_MAX_FRAME_GAP = 3;
		int		WDD_DANCE_MIN_CONSFRAMES = 20;

		//
		//	Develop: Waggle Dance Configuration
		//
		bool visual = false;
		bool wdd_write_dance_file = false;
		bool wdd_write_signal_file = false;
		int wdd_verbose = 1;

		// prepare frame_counter
		unsigned long long frame_counter_global = 0;
		unsigned long long frame_counter_warmup = 0;

		// prepare videoFrameBuffer
		VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(_FRAME_WIDTH, _FRAME_HEIGHT), cv::Size(20,20));	


		// prepare buffer to hold mono chromized input frame
		cv::Mat frame_input_monochrome = cv::Mat(_FRAME_HEIGHT, _FRAME_WIDTH, CV_8UC1);

		// prepare buffer to hold target frame
		double resize_factor =  pow(2.0, FRAME_RED_FAC);

		int frame_target_width = cvRound(_FRAME_WIDTH/resize_factor);
		int frame_target_height = cvRound(_FRAME_HEIGHT/resize_factor);
		cv::Mat frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);


		std::cout<<"Printing WaggleDanceDetector frame parameter:"<<std::endl;
		printf("frame_height: %d\n", frame_target_height);
		printf("frame_width: %d\n", frame_target_width);
		printf("frame_rate: %d\n", WDD_FRAME_RATE);
		printf("frame_red_fac: %d\n", FRAME_RED_FAC);

		//
		// prepare DotDetectorLayer config vector
		//
		std::vector<double> ddl_config;
		ddl_config.push_back(DD_FREQ_MIN);
		ddl_config.push_back(DD_FREQ_MAX);
		ddl_config.push_back(DD_FREQ_STEP);
		ddl_config.push_back(WDD_FRAME_RATE);
		ddl_config.push_back(FRAME_RED_FAC);
		ddl_config.push_back(DD_MIN_POTENTIAL);

		std::vector<cv::Point2i> dd_positions;
		for(int i=0; i<frame_target_width; i++)
		{
			for(int j=0; j<frame_target_height; j++)
			{
				// x (width), y(height)
				dd_positions.push_back(cv::Point2i(i,j));
			}
		}
		printf("Initialize with %d DotDetectors (DD_MIN_POTENTIAL=%.1f).\n", 
			dd_positions.size(), DD_MIN_POTENTIAL);

		//
		// prepare WaggleDanceDetector config vector
		//
		std::vector<double> wdd_config;
		// Layer 2
		wdd_config.push_back(WDD_SIGNAL_DD_MAXDISTANCE);
		wdd_config.push_back(WDD_SIGNAL_MIN_CLUSTER_SIZE);
		// Layer 3
		wdd_config.push_back(WDD_DANCE_MAX_POSITION_DISTANCEE);
		wdd_config.push_back(WDD_DANCE_MAX_FRAME_GAP);
		wdd_config.push_back(WDD_DANCE_MIN_CONSFRAMES);

		WaggleDanceDetector wdd(			
			dd_positions,
			&frame_target,
			ddl_config,
			wdd_config,
			&videoFrameBuffer,
			wdd_write_signal_file,
			wdd_write_dance_file,
			wdd_verbose
			);

		// Create camera instance
		_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
		if(_cam == NULL)
		{
			std::cerr<<"ERROR! Could not create camera instance!"<<std::endl;
			return;
		}
		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_GAIN = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_EXPOSURE = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_WHITEBALANCE = true!"<<std::endl;

		PBYTE pCapBuffer = NULL;
		// create the appropriate OpenCV image
		IplImage *pCapImage = cvCreateImage(cvSize(_FRAME_WIDTH, _FRAME_HEIGHT), IPL_DEPTH_8U, 1);
		IplImage frame_target_bc = frame_target;

		// Get camera frame dimensions
		CLEyeCameraGetFrameDimensions(_cam, _FRAME_WIDTH, _FRAME_HEIGHT);

		// Set some camera parameters
		CLEyeSetCameraParameter(_cam, CLEYE_GAIN, 10);
		CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, 511);

		// Start capturing
		CLEyeCameraStart(_cam);

		//
		// WARMUP LOOP
		//
		printf("Start camera warmup..\n");
		bool WARMUP_DONE = false;
		unsigned int WARMUP_FPS_HIT = 0;
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		while(_running)
		{
			cvGetImageRawData(pCapImage, &pCapBuffer);

			CLEyeCameraGetFrame(_cam, pCapBuffer);

			frame_input_monochrome = cv::Mat(pCapImage, true);

			// save to global Frame Buffer
			videoFrameBuffer.addFrame(&frame_input_monochrome);

			// subsample
			cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
				0, 0, cv::INTER_AREA);			

			// 
			if(_visual)
				cvShowImage(_windowName, &frame_target_bc);
			
			if(!WARMUP_DONE)
			{
				wdd.copyInitialFrame(frame_counter_global, false);
			}
			else
			{
				// save number of frames needed for camera warmup
				frame_counter_warmup = frame_counter_global;
				wdd.copyInitialFrame(frame_counter_global, true);
				break;
			}
			// finally increase frame_input counter	
			frame_counter_global++;

			//test fps
			if((frame_counter_global % 100) == 0)
			{
				std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
				double fps = 100/sec.count();
				printf("%1.f fps ..", fps);
				if(abs(WDD_FRAME_RATE - fps) < 1)
				{
					printf("\t [GOOD]\n");
					WARMUP_DONE = ++WARMUP_FPS_HIT >= 3 ? true : false;
				}
				else
				{
					printf("\t [BAD]\n");
				}

				start = std::chrono::steady_clock::now();
			}
		}		
		printf("Camera warmup done!\n\n\n");

		//
		// MAIN LOOP
		//
		std::vector<double> bench_res;
		while(_running)
		{

			cvGetImageRawData(pCapImage, &pCapBuffer);

			CLEyeCameraGetFrame(_cam, pCapBuffer);

			frame_input_monochrome = cv::Mat(pCapImage, true);

			// save to global Frame Buffer
			videoFrameBuffer.addFrame(&frame_input_monochrome);

			// subsample
			cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
				0, 0, cv::INTER_AREA);

			// feed WDD with tar_frame
			wdd.copyFrame(frame_counter_global);

			if(_visual)
				cvShowImage(_windowName, &frame_target_bc);

#ifdef WDD_DDL_DEBUG_FULL
			if(frame_counter_global >= WDD_FBUFFER_SIZE-1)
				printf("Frame# %llu\t DD_SIGNALS_NUMBER: %d\n", WaggleDanceDetector::WDD_SIGNAL_FRAME_NR, DotDetectorLayer::DD_SIGNALS_NUMBER);

			// check exit condition
			if((frame_counter_global-frame_counter_warmup) >= WDD_DDL_DEBUG_FULL_MAX_FRAME-1)
			{
				std::cout<<"************** WDD_DDL_DEBUG_FULL DONE! **************"<<std::endl;
				printf("WDD_DDL_DEBUG_FULL captured %d frames.\n", WDD_DDL_DEBUG_FULL_MAX_FRAME);
				_running = false;				
			}
#endif
			// finally increase frame_input counter	
			frame_counter_global++;
			// benchmark output
			if((frame_counter_global % 100) == 0)
			{
				std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
				bench_res.push_back(100/sec.count());
				start = std::chrono::steady_clock::now();
			}

			if((frame_counter_global % 500) == 0)
			{
				printf("collected fps: ");
				double avg = 0;
				for(auto it=bench_res.begin(); it!=bench_res.end(); ++it)
				{
					printf("%.1f ", *it);
					avg += *it;
				}
				printf("(avg: %.1f)\n", avg/bench_res.size());
				bench_res.clear();
			}
		}
		//
		// release objects
		//

		cvReleaseImage(&pCapImage);
		// Stop camera capture
		CLEyeCameraStop(_cam);
		// Destroy camera object
		CLEyeDestroyCamera(_cam);
		// Destroy the allocated OpenCV image
		cvReleaseImage(&pCapImage);
		_cam = NULL;
	}
	DWORD WINAPI CLEyeCameraCapture::CaptureThread(LPVOID instance)
	{
		// seed the rng with current tick count and thread id
		srand(GetTickCount() + GetCurrentThreadId());
		// forward thread to Capture function
		CLEyeCameraCapture *pThis = (CLEyeCameraCapture *)instance;
		pThis->Run();
		return 0;
	}
}