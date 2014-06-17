#include "stdafx.h"
#include "DotDetectorLayer.h"
#include "InputVideoParameters.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"

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
void getExeFullPath(TCHAR * out, std::size_t size)
{
	TCHAR BUFF[MAX_PATH];

	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		GetModuleFileName(hModule, BUFF, sizeof(BUFF)/sizeof(TCHAR)); 
		// remove WaggleDanceDetector.exe part (23 chars :P)
		_tcsncpy_s(out, size, BUFF, _tcslen(BUFF)-23-1);
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

bool dirExists(const TCHAR * dirPath)
{
	int result = PathIsDirectory((LPCTSTR)dirPath);

	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}

/* saves to full path to executable */
TCHAR _FULL_PATH_EXE[MAX_PATH]; 

int main(int nargs, char** argv)
{
	int FRAME_WIDTH=-1;
	int FRAME_HEIGHT=-1;
	int FRAME_RATE=-1;		
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
	bool wdd_verbose = false;

	// get the full path to executable 
	getExeFullPath(_FULL_PATH_EXE, sizeof(_FULL_PATH_EXE));

	char videoFilename[MAXCHAR];

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
		FRAME_RATE = 102;
		//FRAME_RATE = atoi(argv[1]);

		capture.set(CV_CAP_PROP_FRAME_WIDTH , FRAME_WIDTH);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT , FRAME_HEIGHT);
		capture.set(CV_CAP_PROP_FPS, FRAME_RATE);
		InputVideoParameters::printPropertiesOf(&capture);
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
		FRAME_RATE = 102;

		InputVideoParameters::printPropertiesOf(&capture);
	}else {
		std::cerr<<"Error! Wrong number of arguments!"<<std::endl;
	}


	cv::Mat frame_input;
	cv::Mat frame_input_monochrome;
	cv::Mat frame_target;


	if(visual)
		cv::namedWindow("Video");


	/* prepare frame_counter */
	unsigned long long frame_counter = 0;

	/* prepare videoFrameBuffer */
	VideoFrameBuffer videoFrameBuffer(frame_counter, cv::Size(FRAME_HEIGHT, FRAME_WIDTH));
	videoFrameBuffer.setSequecenFrameSize(cv::Size(100,100));

	/* prepare buffer to hold mono chromized input frame */
	frame_input_monochrome =
		cv::Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC1);

	/* prepare buffer to hold target frame */
	double resize_factor =  pow(2.0, FRAME_RED_FAC);

	int frame_target_width = cvRound(FRAME_WIDTH/resize_factor);
	int frame_target_height = cvRound(FRAME_HEIGHT/resize_factor);

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
	std::cout<<"Initialize with "<<dd_positions.size()<<" DotDetectors."<<std::endl;

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
		wdd.copyFrame(frame_counter);

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
					if( (*it).DANCE_FRAME_END >= frame_counter-10)
					{
						cv::circle(frame_input, (*it).positions[0]*std::pow(2,FRAME_RED_FAC),
							Cir_radius, Cir_color_gre, Cir_thikness);
					}
				}
			}

			cv::imshow("Video", frame_input);
			cv::waitKey(10);
		}

		// finally increase frame_input counter	
		frame_counter++;

		// benchmark output
		if((frame_counter % 100) == 0)
		{
			std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;

			//std::cout<<"fps: "<< 100/sec.count() <<"("<< cvRound(sec.count()*1000)<<"ms)"<<std::endl;

			bench_res.push_back(100/sec.count());

			start = std::chrono::steady_clock::now();
		}
	}
	capture.release();

	double avg = 0;
	for(auto it=bench_res.begin(); it!=bench_res.end(); ++it)
	{
		std::cout<<"fps: "<<*it<<std::endl;
		avg += *it;
	}

	std::cout<<"average fps: "<<avg / bench_res.size()<<std::endl;
}
