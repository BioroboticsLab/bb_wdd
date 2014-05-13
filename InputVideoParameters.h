/*
* InputVideoParameters.h
*
*  Created on: 14.04.2014
*      Author: sam
*/
#include "stdafx.h"

#ifndef INPUTVIDEOPARAMETERS_H_
#define INPUTVIDEOPARAMETERS_H_



namespace wdd
{
	/*
	* InputVideoParameters class wraps openCV VideoCapture methods i.o. to
	* guarantee int type for frame_width, frame_height and model parameters of
	* input. VideoParameters are read on Construction and read-only afterwards.
	*/
	class InputVideoParameters
	{
	private:
		cv::VideoCapture * _vc;
		int frame_width;
		int frame_height;
		double frame_rate;
		std::string frame_format;


	public:
		InputVideoParameters(cv::VideoCapture * vc);
		~InputVideoParameters();

		int getFrameWidth();
		int getFrameHeight();
		double getFrameRate();
		std::string getFrameFormat();
		void printProperties();

		static int getFrameWidthOf(cv::VideoCapture * vc);
		static int getFrameHeightOf(cv::VideoCapture * vc);
		static double getFrameRateOf(cv::VideoCapture * vc);
		static std::string getFrameFormatOf(cv::VideoCapture * vc);
		static void printPropertiesOf(cv::VideoCapture * vc);
	};

} /* namespace WaggleDanceDetector */

#endif /* INPUTVIDEOPARAMETERS_H_ */
