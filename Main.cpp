#include "stdafx.h"

using namespace wdd;

bool fileExists (const std::string& file_name)
{
	struct stat buffer;
	return (stat (file_name.c_str(), &buffer) == 0);
}

bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

std::vector<cv::Mat> loadFrameSequenc(unsigned long long sFrame, unsigned long long eFrame, cv::Point2i center, cv::VideoCapture * capture_ptr )
{
	cv::Mat frame;
	cv::Mat subframe_monochrome;
	unsigned long long save_currentFrameNumber = static_cast<unsigned long long>(capture_ptr->get(CV_CAP_PROP_POS_FRAMES));

	std::vector<cv::Mat> out;

	cv::Size size(100,100);

	cv::Rect roi_rec((center*pow(2, 4)) - cv::Point(50,50), size);

	//init loop
	capture_ptr->set(CV_CAP_PROP_POS_FRAMES , static_cast<double>(sFrame-15));

	while(capture_ptr->get(CV_CAP_PROP_POS_FRAMES) <=  eFrame-15)
	{
		if(capture_ptr->read(frame))
		{
			cv::Mat roi(frame, roi_rec);
			cv::cvtColor(roi,subframe_monochrome, CV_BGR2GRAY);
			out.push_back(subframe_monochrome.clone());

			//WaggleDanceOrientator::showImage(&out[out.size()-1]);
		}
		else
			std::cerr << "Error loading frame# "<< capture_ptr->get(CV_CAP_PROP_POS_FRAMES) << std::endl;
	}

	capture_ptr->set(CV_CAP_PROP_POS_FRAMES , static_cast<double>(save_currentFrameNumber));

	return out;
}

int main()
{
	/* default WDD parameter values */
	int FRAME_RATE = 100;
	int FRAME_RED_FAC = 4;
	int wdd_fbuffer_size = 32;
	double wdd_signal_dd_maxdistance = 2.3;
	int wdd_signal_dd_min_cluster_size = 6;
	double wdd_signal_dd_min_score = 16444;

	bool verbose = false;
	bool visual = false;
	bool wdd_verbose = false;
	bool wdd_write_signal_file = true;


	cv::Mat frame_input;
	cv::Mat frame_input_monochrome;
	cv::Mat frame_target;

	/*
	* prepare OpenCV VideoCapture
	*/
	cv::VideoCapture capture(0);
	if(!capture.isOpened())
	{
		std::cerr<<"Warning! No webcam detected, using file system as input!"<<std::endl;

		std::string videoFile = "C:\\Users\\Alexander Rau\\WaggleDanceDetector\\data\\22_08_2008-1244-SINGLE.raw.avi";

		if(!fileExists(videoFile))
		{
			std::cout << "video path not ok! " << std::endl;

			exit(-201);
		}

		if(!capture.open(videoFile))
		{
			std::cout << "video not ok - check openCV install "
				"(https://help.ubuntu.com/community/OpenCV)" << std::endl;

			exit(-202);
		}
	}

	if(visual)
		cv::namedWindow("Video");

	/*
	* fetch video parameters
	*/
	InputVideoParameters vp(&capture);
	if(verbose)
	{
		vp.printProperties();
	}

	/* prepare frame_counter */
	unsigned long long frame_counter = 0;

	/* prepare videoFrameBuffer */
	VideoFrameBuffer videoFrameBuffer(frame_counter, cv::Size(vp.getFrameHeight(), vp.getFrameWidth()));

	/* prepare buffer to hold mono chromized input frame */
	frame_input_monochrome =
		cv::Mat(vp.getFrameHeight(),vp.getFrameWidth(),CV_8UC1);

	/* prepare buffer to hold target frame */
	double resize_factor =  pow(2.0, FRAME_RED_FAC);
	int frame_target_height = cvRound(vp.getFrameHeight()/resize_factor);
	int frame_target_width = cvRound(vp.getFrameWidth()/resize_factor);
	frame_target = cv::Mat(frame_target_height, frame_target_width,CV_8UC1);

	/*
	* prepare WaggleDanceDetector
	*/
	std::vector<double> dd_freq_config;
	dd_freq_config.push_back(11);
	dd_freq_config.push_back(17);
	dd_freq_config.push_back(1);

	/* TODO: conversion from int to double to int ?! */
	std::vector<double> frame_config;
	frame_config.push_back((double)FRAME_RATE);
	frame_config.push_back((double)FRAME_RED_FAC);
	frame_config.push_back((double)frame_target_width);
	frame_config.push_back((double)frame_target_height);


	std::map<std::size_t,cv::Point2i> dd_pos_id2point_map;
	std::size_t dd_uniq_id = 0;

	for(int i=0; i<frame_target_width; i++)
	{
		for(int j=0; j<frame_target_height; j++)
		{
			// x (width), y(height)
			dd_pos_id2point_map.insert(
				std::pair<std::size_t, cv::Point2i>(
				dd_uniq_id++,cv::Point2i(i,j)));
		}
	}

	std::vector<double> wdd_signal_dd_config;
	wdd_signal_dd_config.push_back(wdd_signal_dd_maxdistance);
	wdd_signal_dd_config.push_back(wdd_signal_dd_min_cluster_size);
	wdd_signal_dd_config.push_back(wdd_signal_dd_min_score);

	WaggleDanceDetector wdd(
		dd_freq_config,
		dd_pos_id2point_map,
		frame_config,
		wdd_fbuffer_size,
		wdd_signal_dd_config,
		wdd_write_signal_file,
		wdd_verbose,
		&videoFrameBuffer);



	const std::map<std::size_t,cv::Point2d> * WDDSignalId2PointMap = wdd.getWDDSignalId2PointMap();
	int Cir_radius = 11;
	cv::Scalar Cir_color = cv::Scalar(124,255,0);
	//make the circle filled with value < 0
	int Cir_thikness = -1;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

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
		wdd.addFrame(frame_target, frame_counter, true);

		// output visually if enabled
		if(visual)
		{
			//check for WDD Signal
			if(wdd.isWDDSignal())
			{
				for(std::size_t i=0; i< wdd.getWDDSignalNumber(); i++)
				{
					cv::circle(frame_input, (*WDDSignalId2PointMap).find(i)->second*std::pow(2,FRAME_RED_FAC),
						Cir_radius, Cir_color, Cir_thikness);
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

			std::cout<<"fps: "<< 100/sec.count() <<"("<< cvRound(sec.count()*1000)<<"ms)"<<std::endl;

			start = std::chrono::steady_clock::now();
		}
	}
}
