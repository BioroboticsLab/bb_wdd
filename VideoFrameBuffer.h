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
	public:
		VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size size);
		~VideoFrameBuffer(void);
		void addFrame(cv:: Mat * frame_ptr);
		cv::Mat * getFrameByNumber(unsigned long long frame_nr);
		std::vector<cv::Mat> loadFrameSequenc(unsigned long long sFrame, unsigned long long eFrame, cv::Point2i center);
	};
}

