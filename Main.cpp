#include "stdafx.h"

using namespace wdd;

bool fileExists (const std::string& name)
{
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

int main()
{
	int FRAME_RED_FAC = 4;

	bool verbose = false;
	bool visual = false;
	bool wdd_verbose = true;

	/*
	* prepare OpenCV VideoCapture
	*/
	cv::Mat frame_input;
	cv::Mat frame_input_monochrome;
	cv::Mat frame_target;
	cv::VideoCapture capture;

	if(visual)
		cv::namedWindow("Video");


	std::string videoFile = "C:\\Users\\Alexander Rau\\WaggleDanceDetector\\data\\test.avi";

	if(!fileExists(videoFile))
	{
		std::cout << "video path not ok! " << std::endl;

		exit(-200);
	}

	if(!capture.open(videoFile))
	{
		std::cout << "video not ok - check openCV install "
			"(https://help.ubuntu.com/community/OpenCV)" << std::endl;

		exit(-200);
	}

	/*
	* fetch video parameters
	*/
	InputVideoParameters vp(&capture);
	if(verbose)
	{
		vp.printProperties();
	}

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
	frame_config.push_back(vp.getFrameRate());
	frame_config.push_back((double)FRAME_RED_FAC);
	frame_config.push_back((double)frame_target_width);
	frame_config.push_back((double)frame_target_height);

	/* TODO: calculate positions from reduced frame size, full grid */
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


	int wdd_fbuffer_size = 32;

	std::vector<double> wdd_signal_dd_config;
	wdd_signal_dd_config.push_back(2.3);
	wdd_signal_dd_config.push_back(4);
	//wdd_signal_dd_config.push_back(17000);
		wdd_signal_dd_config.push_back(1700);

	WaggleDanceDetector wdd(
		dd_freq_config,
		dd_pos_id2point_map,
		frame_config,
		wdd_fbuffer_size,
		wdd_signal_dd_config,
		wdd_verbose);



	unsigned __int64 frame_counter = 0;

	while(capture.read(frame_input))
	{
		//convert BGR -> Gray
		cv::cvtColor(frame_input,frame_input_monochrome, CV_BGR2GRAY);

		// subsample
		cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
			0, 0, cv::INTER_LINEAR);

		if(verbose)
		{
			TypeToString::printType(frame_input);
			TypeToString::printType(frame_input_monochrome);
			TypeToString::printType(frame_target);
		}

		// feed WDD with tar_frame
		wdd.addFrame(frame_target, frame_counter, true);

		// output visually if enabled
		if(visual)
		{
			cv::imshow("Video", frame_input_monochrome);
			cv::waitKey(10);

		}
		// finally increase frame_input counter
		std::cout<<"Done frame#: "<<frame_counter<<std::endl;
		frame_counter++;
	}

}
