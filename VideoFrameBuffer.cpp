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
		
		int startFrameShift = WDD_FBUFFER_SIZE/2;
		int endFrameShift = WDD_FBUFFER_SIZE/2;

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

	cv::Mat * VideoFrameBuffer::getFrameByNumber(unsigned long long frame_nr)
	{
		unsigned int offset = static_cast<unsigned int>(CURRENT_FRAME_NR - frame_nr);
		//std::cout<<"DEBUG: VideoFrameBuffer::getFrameByNumber - STATUS: fnr:"<< frame_nr<<" cfnr:"<<CURRENT_FRAME_NR<<" offset:"<<offset<<std::endl;
		if( offset > MAX_FRAME_HISTORY)
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame "<<frame_nr<<" - history too small!"<<std::endl;
			return NULL;
		}
		else
		{
			offset = (NEXT_CELL_ID - offset) % MAX_FRAME_HISTORY;
			//debug only
			//std::cout<< "DEBUG: VideoFrameBuffer loaded frame "<<frame_nr<<std::endl;
			return &FRAME[offset];
		}
	}

	std::vector<cv::Mat> VideoFrameBuffer::loadFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC)
	{
		startFrame -= startFrameShift;
		endFrame -= endFrameShift;

		std::cout<<"HERE"<<std::endl;
		std::cout<<center<<std::endl;
		std::cout<<sequenceFramePointOffset<<std::endl;
		std::cout<<sequenceFrameSize<<std::endl;
		std::cout<<startFrame<<std::endl;
		std::cout<<endFrame<<std::endl;
		cv::Mat * frame_ptr;

		std::vector<cv::Mat> out;

		cv::Rect roi_rec((center*pow(2, FRAME_REDFAC)) - sequenceFramePointOffset, sequenceFrameSize);

		while(startFrame <=  endFrame)
		{
			frame_ptr = getFrameByNumber(startFrame);
			if(!frame_ptr->empty())
			{
				cv::Mat subframe_monochrome(*frame_ptr, roi_rec);
				out.push_back(subframe_monochrome.clone());
			}
			else
				std::cerr << "Error! VideoFrameBuffer::loadFrameSequenc frame "<< startFrame<< " empty!"<<std::endl;

			startFrame++;
		}

		return out;
	}
}