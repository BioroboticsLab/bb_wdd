#include "stdafx.h"
#include "VideoFrameBuffer.h"

namespace wdd
{
	VideoFrameBuffer::VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size size)
	{
		CURRENT_FRAME_NR = current_frame_nr;
		MAX_FRAME_HISTORY = 600;
		FRAME = new cv::Mat[MAX_FRAME_HISTORY];
		NEXT_CELL_ID = 0;

		startFrameShift = cvRound( WDD_FBUFFER_SIZE/2.0 );
		endFrameShift = cvRound(WDD_FBUFFER_SIZE/2.0);

		//debug only
		FRAME_NR = new unsigned long long[MAX_FRAME_HISTORY];

		//init memory
		for(std::size_t i=0; i<MAX_FRAME_HISTORY; i++)
		{
			*(FRAME+i) = cv::Mat(size, CV_8UC1);
		}
	}


	VideoFrameBuffer::~VideoFrameBuffer(void)
	{
	}
	void VideoFrameBuffer::setSequecenFrameSize(cv::Size size)
	{
		sequenceFrameSize = size;
		sequenceFramePointOffset = cv::Point2i(size.width/2,size.height/2);
	}
	void VideoFrameBuffer::addFrame(cv:: Mat * frame_ptr)
	{
		//std::cout<<"DEBUG: VideoFrameBuffer::addFrame - STATUS: "<< NEXT_CELL_ID<<"/"<<MAX_FRAME_HISTORY<<std::endl;

		*(FRAME + NEXT_CELL_ID) = frame_ptr->clone();
		//debug only
		*(FRAME_NR + NEXT_CELL_ID) = CURRENT_FRAME_NR;
		//WaggleDanceOrientator::showImage(FRAME + NEXT_CELL_ID);

		NEXT_CELL_ID = (++NEXT_CELL_ID) % MAX_FRAME_HISTORY;

		CURRENT_FRAME_NR++;	
	}

	cv::Mat * VideoFrameBuffer::getFrameByNumber(unsigned long long req_frame_nr)
	{
		if(CURRENT_FRAME_NR >= req_frame_nr)
		{
			unsigned int offset = static_cast<unsigned int>(CURRENT_FRAME_NR - req_frame_nr);

			if( offset <= MAX_FRAME_HISTORY)
			{
				offset = (NEXT_CELL_ID - offset) % MAX_FRAME_HISTORY;
				return &FRAME[offset];

			}
			else
			{
				std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame "<<req_frame_nr<<" - history too small!"<<std::endl;
				return NULL;
			}
		}
		else
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame "<<req_frame_nr<<" - history too small!"<<std::endl;
			return NULL;
		}
	}

	std::vector<cv::Mat> VideoFrameBuffer::loadFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC)
	{
		// safe frame bounds
		unsigned long long _startFrame = startFrame >= startFrameShift ? startFrame-startFrameShift : 0;
		const unsigned long long _endFrame = endFrame >= endFrameShift ? endFrame-endFrameShift : 0;

		const unsigned short _center_x = static_cast<int>(center.x*pow(2, FRAME_REDFAC));
		const unsigned short _center_y = static_cast<int>(center.y*pow(2, FRAME_REDFAC));

		int roi_rec_x = static_cast<int>(_center_x - sequenceFramePointOffset.x);
		int roi_rec_y = static_cast<int>(_center_y - sequenceFramePointOffset.y);

		// safe roi edge
		roi_rec_x = roi_rec_x < 0 ? 0 : roi_rec_x;
		roi_rec_y = roi_rec_y < 0 ? 0 : roi_rec_y;
		
		// set roi
		cv::Rect roi_rec(cv::Point2i(roi_rec_x,roi_rec_y), sequenceFrameSize);

		std::vector<cv::Mat> out;

		cv::Mat * frame_ptr;

		while(_startFrame <=  _endFrame)
		{
			frame_ptr = getFrameByNumber(_startFrame);
			if(!frame_ptr->empty())
			{
				cv::Mat subframe_monochrome(*frame_ptr, roi_rec);
				out.push_back(subframe_monochrome.clone());
			}
			else
				std::cerr << "Error! VideoFrameBuffer::loadFrameSequenc frame "<< _startFrame<< " empty!"<<std::endl;

			_startFrame++;
		}

		return out;
	}
}