#include "stdafx.h"
#include "VideoFrameBuffer.h"

namespace wdd
{
	VideoFrameBuffer::VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size cachedFrameSize, cv::Size extractFrameSize):
		_BUFFER_POS(0),
		_CURRENT_FRAME_NR(current_frame_nr),
		_cachedFrameSize(cachedFrameSize),
		_extractFrameSize(extractFrameSize)
	{		
		if( (extractFrameSize.width > cachedFrameSize.width) || (extractFrameSize.height > cachedFrameSize.height))
		{
			std::cerr<< "Error! VideoFrameBuffer():: extractFrameSize can not be bigger then cachedFrameSize in any dimension!"<<std::endl;
		}
		_sequenceFramePointOffset = 
			cv::Point2i(extractFrameSize.width/2,extractFrameSize.height/2);

		//init memory
		for(std::size_t i=0; i<VFB_MAX_FRAME_HISTORY; i++)
		{
			_FRAME[i] = cv::Mat(cachedFrameSize, CV_8UC1);
		}
	}

	VideoFrameBuffer::~VideoFrameBuffer(void)
	{
	}	

	void VideoFrameBuffer::addFrame(cv:: Mat * frame_ptr)
	{		
		//std::cout<<"VideoFrameBuffer::addFrame::_BUFFER_POS: "<<_BUFFER_POS<<std::endl;
		_FRAME[_BUFFER_POS]= frame_ptr->clone();
		
		_BUFFER_POS = (++_BUFFER_POS) < VFB_MAX_FRAME_HISTORY ? _BUFFER_POS : 0;

		_CURRENT_FRAME_NR++;
	}

	cv::Mat * VideoFrameBuffer::getFrameByNumber(unsigned long long req_frame_nr)
	{
		int frameJump = static_cast<int>(_CURRENT_FRAME_NR - req_frame_nr);

		//std::cout<<"VideoFrameBuffer::getFrameByNumber::_CURRENT_FRAME_NR, req_frame_nr, frameJump: "<<_CURRENT_FRAME_NR<<", "<<req_frame_nr<<", "<<frameJump<<std::endl;

		// if req_frame_nr > CURRENT_FRAME_NR --> framesJump < 0
		if(frameJump <= 0)
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame from future "<<req_frame_nr<<" (CURRENT_FRAME_NR < req_frame_nr)!"<<std::endl;
			return NULL;
		}

		// assert: CURRENT_FRAME_NR >= req_frame_nr --> framesJump >= 0
		if( frameJump <= VFB_MAX_FRAME_HISTORY)
		{
			int framePosition = _BUFFER_POS - frameJump;

			framePosition = framePosition >= 0 ? framePosition : VFB_MAX_FRAME_HISTORY + framePosition;
			
			//std::cout<<"VideoFrameBuffer::getFrameByNumber::_BUFFER_POS: "<<framePosition<<std::endl;

			return &_FRAME[framePosition];
		}
		else
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame "<<req_frame_nr<<" - history too small!"<<std::endl;
			return NULL;
		}


	}

	std::vector<cv::Mat> VideoFrameBuffer::loadFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC)
	{
		//std::cout<<"VideoFrameBuffer::loadFrameSequenc::startFrame, endFrame: "<<startFrame<<", "<<endFrame<<std::endl;
		//std::cout<<"VideoFrameBuffer::_cachedFrameSize.width, _cachedFrameSize.height: "<<_cachedFrameSize.width<<", "<<_cachedFrameSize.height<<std::endl;
		//std::cout<<"VideoFrameBuffer::_extractFrameSize.width, _extractFrameSize.height: "<<_extractFrameSize.width<<", "<<_extractFrameSize.height<<std::endl;

		const unsigned short _center_x = static_cast<int>(center.x*pow(2, FRAME_REDFAC));
		const unsigned short _center_y = static_cast<int>(center.y*pow(2, FRAME_REDFAC));

		int roi_rec_x = static_cast<int>(_center_x - _sequenceFramePointOffset.x);
		int roi_rec_y = static_cast<int>(_center_y - _sequenceFramePointOffset.y);

		// safe roi edge - lower bounds check
		roi_rec_x = roi_rec_x < 0 ? 0 : roi_rec_x;
		roi_rec_y = roi_rec_y < 0 ? 0 : roi_rec_y;
		
		// safe roi edge - upper bounds check
		roi_rec_x = (roi_rec_x + _extractFrameSize.width) <= _cachedFrameSize.width ? roi_rec_x : _cachedFrameSize.width - _extractFrameSize.width;
		roi_rec_y = (roi_rec_y + _extractFrameSize.height) <= _cachedFrameSize.height ? roi_rec_y : _cachedFrameSize.height - _extractFrameSize.height;


		// set roi
		cv::Rect roi_rec(cv::Point2i(roi_rec_x,roi_rec_y), _extractFrameSize);
		
		std::vector<cv::Mat> out;

		cv::Mat * frame_ptr;

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