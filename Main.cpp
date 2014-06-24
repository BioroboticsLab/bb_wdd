#include "stdafx.h"
#include "DotDetectorLayer.h"
#include "InputVideoParameters.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"
#include "WaggleDanceExport.h"

using namespace wdd;

double uvecToDegree(cv::Point2d in)
{
	if(_isnan(in.x) | _isnan(in.y))
		return std::numeric_limits<double>::quiet_NaN();
	/*
	% rotatet 90 degree counterclock wise - 0° is at top center
	R= [  0     1; -1     0];

	u = u * R;

	theta = atan2(u(2),u(1));
	ThetaInDegrees = (theta*180/pi);
	*/

	double theta = atan2(in.y,in.x);
	return theta * 180/CV_PI;
}
void getExeFullPath(char * out, std::size_t size)
{
	char BUFF[MAX_PATH];

	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		GetModuleFileName(hModule, BUFF, sizeof(BUFF)/sizeof(char)); 
		// remove WaggleDanceDetector.exe part (23 chars :P)
		_tcsncpy_s(out, size, BUFF, strlen(BUFF)-23-1);
	}
	else
	{
		std::cerr << "Error! Module handle is NULL - can not retrive exe path!" << std::endl ;
		exit(-2);
	}
}

bool fileExists (const std::string& file_name)
{
	struct stat buffer;
	return (stat (file_name.c_str(), &buffer) == 0);
}

bool dirExists(const char * dirPath)
{
	int result = PathIsDirectory((LPCTSTR)dirPath);

	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}

/* saves to full path to executable */
char _FULL_PATH_EXE[MAX_PATH]; 

enum RUN_MODE {TEST, LIVE};

int main(int nargs, char** argv)
{
	// get the full path to executable 
	getExeFullPath(_FULL_PATH_EXE, sizeof(_FULL_PATH_EXE));
	char videoFilename[MAXCHAR];
	WaggleDanceExport::execRootExistChk();

	int FRAME_WIDTH=-1;
	int FRAME_HEIGHT=-1;
	int FRAME_RATE=-1;

	RUN_MODE RM;

	//WaggleDanceOrientator
	cv::Size videoFrameBufferExtractSize(20,20);
	
	//WaggleDanceExport
	WaggleDanceExport::execRootExistChk();

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
	bool wdd_write_dance_file = true;
	bool wdd_write_signal_file = false;
	int wdd_verbose = 1;

	

	cv::VideoCapture capture(0);
	if (nargs == 1)
	{
		std::cout<<"************** Run started in live mode **************"<<std::endl;
		if(!capture.isOpened())
		{
			std::cerr<<"Error! No webcam detected!"<<std::endl;
			exit(-1);
		}

		/* WEBCAM INPUT STREAM SETTINGS */
		FRAME_WIDTH= 320;
		FRAME_HEIGHT = 240;
		//DEBUG
		FRAME_RATE = WDD_FRAME_RATE;
		//FRAME_RATE = atoi(argv[1]);

		capture.set(CV_CAP_PROP_FRAME_WIDTH , FRAME_WIDTH);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT , FRAME_HEIGHT);
		capture.set(CV_CAP_PROP_FPS, FRAME_RATE);
		InputVideoParameters::printPropertiesOf(&capture);
		RM = LIVE;
	}
	// one argument passed to executable - TEST MODE ON
	else if (nargs == 2)
	{
		std::cout<<"************** Run started in test mode **************"<<std::endl;
		sprintf_s(videoFilename,"%s", argv[1]);

		if(!fileExists(videoFilename))
		{
			std::cerr<<"Error! Wrong video path!"<<std::endl;
			exit(-201);
		}

		if(!capture.open(videoFilename))
		{
			std::cerr << "Error! Video input stream broken - check openCV install "
				"(https://help.ubuntu.com/community/OpenCV)" << std::endl;

			exit(-202);
		}
		InputVideoParameters vp(&capture);
		FRAME_WIDTH= vp.getFrameWidth();
		FRAME_HEIGHT = vp.getFrameHeight();
		FRAME_RATE = WDD_FRAME_RATE;

		InputVideoParameters::printPropertiesOf(&capture);
		RM = TEST;
	}else {
		std::cerr<<"Error! Wrong number of arguments!"<<std::endl;
	}
#ifdef WDD_DDL_DEBUG_FULL
	std::cout<<"************** WDD_DDL_DEBUG_FULL ON!   **************"<<std::endl;
#endif
	cv::Mat frame_input;
	cv::Mat frame_input_monochrome;
	cv::Mat frame_target;


	if(visual)
		cv::namedWindow("Video");


	/* prepare frame_counter */
	unsigned long long frame_counter_global = 0;
	unsigned long long frame_counter_warmup = 0;

	/* prepare videoFrameBuffer */
	VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(FRAME_HEIGHT, FRAME_WIDTH));
	videoFrameBuffer.setSequecenFrameSize(videoFrameBufferExtractSize);

	/* prepare buffer to hold mono chromized input frame */
	frame_input_monochrome =
		cv::Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC1);

	/* prepare buffer to hold target frame */
	double resize_factor =  pow(2.0, FRAME_RED_FAC);

	int frame_target_width = cvRound(FRAME_WIDTH/resize_factor);
	int frame_target_height = cvRound(FRAME_HEIGHT/resize_factor);

	std::cout<<"Printing WaggleDanceDetector frame parameter:"<<std::endl;
	printf("frame_height: %d\n", frame_target_height);
	printf("frame_width: %d\n", frame_target_width);
	printf("frame_rate: %d\n", FRAME_RATE);
	printf("frame_red_fac: %d\n", FRAME_RED_FAC);
	frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);

	/*
	* prepare DotDetectorLayer config vector
	*/
	std::vector<double> ddl_config;
	ddl_config.push_back(DD_FREQ_MIN);
	ddl_config.push_back(DD_FREQ_MAX);
	ddl_config.push_back(DD_FREQ_STEP);
	ddl_config.push_back(FRAME_RATE);
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

	/*
	* prepare WaggleDanceDetector config vector
	*/
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


	const std::map<std::size_t,cv::Point2d> * WDDSignalId2PointMap = wdd.getWDDSignalId2PointMap();
	const std::vector<DANCE> * WDDFinishedDances = wdd.getWDDFinishedDancesVec();
	const std::map<std::size_t,cv::Point2d>  WDDDance2PointMap;


	int Cir_radius = 3;
	cv::Scalar Cir_color_yel = cv::Scalar(0,255,255);
	cv::Scalar Cir_color_gre = cv::Scalar(128,255,0);
	cv::Scalar Cir_color_som = cv::Scalar(128,255,255);
	//make the circle filled with value < 0
	int Cir_thikness = -1;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	std::vector<double> bench_res;
	//loop_bench_res_sing.reserve(dd_positions.size());
	//loop_bench_avg.reserve(1000);

	if(RM == LIVE)
	{
		printf("Start camera warmup..\n");
		bool WARMUP_DONE = false;
		unsigned int WARMUP_FPS_HIT = 0;
		while(capture.read(frame_input))
		{
			//convert BGR -> Gray
			cv::cvtColor(frame_input,frame_input_monochrome, CV_BGR2GRAY);

			// save to global Frame Buffer
			videoFrameBuffer.addFrame(&frame_input_monochrome);

			// subsample
			cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
				0, 0, cv::INTER_AREA);
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
	}

	while(capture.read(frame_input))
	{

		//convert BGR -> Gray
		cv::cvtColor(frame_input,frame_input_monochrome, CV_BGR2GRAY);

		// save to global Frame Buffer
		videoFrameBuffer.addFrame(&frame_input_monochrome);

		// subsample
		cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
			0, 0, cv::INTER_AREA);

		// feed WDD with tar_frame
		if(frame_counter_global < WDD_FBUFFER_SIZE-1)
		{
			wdd.copyInitialFrame(frame_counter_global, false);
		}
		else if (frame_counter_global == WDD_FBUFFER_SIZE-1)
		{
			wdd.copyInitialFrame(frame_counter_global, true);			
		}
		else 
		{
			wdd.copyFrame(frame_counter_global);
		}

		// output visually if enabled
		if(visual)
		{
			if(DotDetectorLayer::DD_SIGNALS_NUMBER>0)
			{
				for(std::size_t i=0; i< DotDetectorLayer::DD_NUMBER; i++)
				{
					if(DotDetectorLayer::DD_SIGNALS[i])
						cv::circle(frame_input, DotDetectorLayer::DD_POSITIONS[i]*std::pow(2,FRAME_RED_FAC),
						2, Cir_color_som, Cir_thikness);
				}

			}
			//check for WDD Signal
			if(wdd.isWDDSignal())
			{
				for(std::size_t i=0; i< wdd.getWDDSignalNumber(); i++)
				{
					cv::circle(frame_input, (*WDDSignalId2PointMap).find(i)->second*std::pow(2,FRAME_RED_FAC),
						Cir_radius, Cir_color_yel, Cir_thikness);
				}
			}
			if(wdd.isWDDDance())
			{

				for(auto it = WDDFinishedDances->begin(); it!=WDDFinishedDances->end(); ++it)
				{
					if( (*it).DANCE_FRAME_END >= frame_counter_global-10)
					{
						cv::circle(frame_input, (*it).positions[0]*std::pow(2,FRAME_RED_FAC),
							Cir_radius, Cir_color_gre, Cir_thikness);
					}
				}
			}

			cv::imshow("Video", frame_input);
			cv::waitKey(1);
		}
#ifdef WDD_DDL_DEBUG_FULL
		if(frame_counter_global >= WDD_FBUFFER_SIZE-1)
			printf("Frame# %llu\t DD_SIGNALS_NUMBER: %d\n", WaggleDanceDetector::WDD_SIGNAL_FRAME_NR, DotDetectorLayer::DD_SIGNALS_NUMBER);

		// check exit condition
		if((frame_counter_global-frame_counter_warmup) >= WDD_DDL_DEBUG_FULL_MAX_FRAME-1)
		{
			std::cout<<"************** WDD_DDL_DEBUG_FULL DONE! **************"<<std::endl;
			printf("WDD_DDL_DEBUG_FULL captured %d frames.\n", WDD_DDL_DEBUG_FULL_MAX_FRAME);
			capture.release();
			exit(0);
		}
#endif
		// finally increase frame_input counter	
		frame_counter_global++;
		// benchmark output
		if((frame_counter_global % 100) == 0)
		{
			std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;

			//std::cout<<"fps: "<< 100/sec.count() <<"("<< cvRound(sec.count()*1000)<<"ms)"<<std::endl;

			bench_res.push_back(100/sec.count());

			start = std::chrono::steady_clock::now();
		}

		if(RM == LIVE)
		{
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
	}
	capture.release();
	
	/*
	unsigned __int64 avgUL = 0;
	for(auto it=DotDetectorLayer::DDL_DEBUG_PERF.begin(); it!=DotDetectorLayer::DDL_DEBUG_PERF.end(); ++it)
		avgUL += *it;

	std::cout<<"average perf ticks: "<<avgUL / (DotDetectorLayer::DDL_DEBUG_PERF.size())<<std::endl;
	*/

	double avg = 0;
	for(auto it=bench_res.begin()+1; it!=bench_res.end(); ++it)
	{
		std::cout<<"fps: "<<*it<<std::endl;
		avg += *it;
	}

	std::cout<<"average fps: "<<avg / (bench_res.size()-1)<<std::endl;
	/*
	std::size_t total = DotDetector::_NRCALL_EXECFULL+DotDetector::_NRCALL_EXECSING+DotDetector::_NRCALL_EXECSLEP;
	printf("FULL: %lu (%.1f %%), SING: %lu (%.1f %%), SLEP: %lu (%.1f %%)\n", 
	DotDetector::_NRCALL_EXECFULL, 
	((DotDetector::_NRCALL_EXECFULL + 0.0) / (total + 0.0) )*100,
	DotDetector::_NRCALL_EXECSING, 
	((DotDetector::_NRCALL_EXECSING + 0.0) / (total + 0.0) )*100,
	DotDetector::_NRCALL_EXECSLEP,
	((DotDetector::_NRCALL_EXECSLEP + 0.0) / (total + 0.0) ) *100) ;

	printf("Collected %d AMPERE samples.\n", DotDetector::_AMP_SAMPLES.size());

	arma::Row<double> asamp = (arma::conv_to<arma::Row<double>>::from(DotDetector::_AMP_SAMPLES));

	printf("mean %f\n", arma::mean( asamp ));
	printf("median %f\n", arma::median( asamp ));
	printf("stddev %f\n", arma::stddev( asamp ));
	printf("var %f\n", arma::var( asamp ));
	printf("min %f\n", arma::min( asamp ));
	printf("max %f\n", arma::max( asamp ));
	/*
	unsigned __int64 sum = 0;
	for(auto it=loop_bench_avg.begin(); it!=loop_bench_avg.end(); ++it)
	{
	//printf("%ul\n", *it);
	sum += *it;
	}

	printf("total avg %I64u\n", sum/loop_bench_avg.size());
	exit(0);
	*/
}
