#pragma once

namespace wdd
{
	class VideoFrameBuffer
	{
		unsigned int MAX_FRAME_HISTORY;

		cv::Mat * FRAME;
		unsigned int NEXT_CELL_ID;
		unsigned long long CURRENT_FRAME_NR;

		//debug only
		unsigned long long * FRAME_NR;

		cv::Size sequenceFrameSize;
		cv::Point sequenceFramePointOffset;

		int startFrameShift;
		int endFrameShift;
	public:
		VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size size);
		~VideoFrameBuffer(void);

		void setSequecenFrameSize(cv::Size size);
		void addFrame(cv:: Mat * frame_ptr);
		cv::Mat * getFrameByNumber(unsigned long long frame_nr);
		std::vector<cv::Mat> loadFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC);
	};
} /* namespace WaggleDanceDetector */

