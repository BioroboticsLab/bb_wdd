#include "stdafx.h"
#include "DotDetector.h"
#include "DotDetectorLayer.h"

namespace wdd {
	// is this how c++ works? 
	std::size_t DotDetector::_BUFF_POS;
	/*
	public functions
	*/
	DotDetector::DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr):
		// copy passed unique id 
		//(= position in arrays of DotDetectorLayer)
		_UNIQUE_ID(UNIQUE_ID),
		// copy passed pixel source pointer 
		//(= position in cv::Mat frame of DotDetectorLayer)
		_pixel_src_ptr(pixel_src_ptr)
	{
		_MAX = 0;
		_MIN = 255;
		_AMPLITUDE = 0;
		_NEWMINMAX = false;
		_sampPos = 0;

		_DD_PX_VALS_SIN = new double * [DotDetectorLayer::DD_FREQS_NUMBER];
		_DD_PX_VALS_COS = new double * [DotDetectorLayer::DD_FREQS_NUMBER];

		for(std::size_t i=0; i<DotDetectorLayer::DD_FREQS_NUMBER; i++)
		{
			_DD_PX_VALS_SIN[i] = new double [WDD_FBUFFER_SIZE];
			_DD_PX_VALS_COS[i] = new double [WDD_FBUFFER_SIZE];
		}
	}

	DotDetector::~DotDetector(void)
	{
		for(std::size_t i=0; i<DotDetectorLayer::DD_FREQS_NUMBER; i++)
		{
			delete _DD_PX_VALS_SIN[i];
			delete _DD_PX_VALS_COS[i];
		}
		delete _DD_PX_VALS_SIN;
		delete _DD_PX_VALS_COS;
	}

	void DotDetector::copyPixelAndDetect()
	{
		_copyPixel();
		_execDetection();
	}
	// add pixel values until buffer is full (initial filling phase)
	void DotDetector::copyPixel()
	{	
		_copyPixel();
	}

	/*
	private functions
	*/

	void DotDetector::_copyPixel()
	{
		// save raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *_pixel_src_ptr;

		// check new raw pixel value against former min/max
		_NEWMINMAX = false;

		if(_DD_PX_VALS_RAW[_BUFF_POS] > _MAX)
		{
			_MAX = _DD_PX_VALS_RAW[_BUFF_POS];
			_AMPLITUDE = _MAX - _MIN;
			_NEWMINMAX = true;
		}

		if(_DD_PX_VALS_RAW[_BUFF_POS] < _MIN)
		{
			_MIN = _DD_PX_VALS_RAW[_BUFF_POS];
			_AMPLITUDE = _MAX - _MIN;
			_NEWMINMAX = true;
		}

		// when new min/max encountered,..
		if(_NEWMINMAX)
		{		
			//  recalculate normalization for all values, new min/max set already
			for(std::size_t i=0; i<WDD_FBUFFER_SIZE; i++)
			{
				_DD_PX_VALS_NOR[i] = _normalizeValue(_DD_PX_VALS_RAW[i]);
			}

			// reset accumulators to zero
			for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
			{
				_ACC_COS_VAL[freq_i] = 0;
				_ACC_SIN_VAL[freq_i] = 0;
			}

			// recalculate cos/sin values and set accumulators
			for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
			{
				for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
				{
					_DD_PX_VALS_COS[freq_i][j] = 
						DotDetectorLayer::DD_FREQS_COSSAMPLES[freq_i][_sampPos] * 
						_DD_PX_VALS_NOR[j];
					_ACC_COS_VAL[freq_i] += _DD_PX_VALS_COS[freq_i][j];

					_DD_PX_VALS_SIN[freq_i][j] = 
						DotDetectorLayer::DD_FREQS_SINSAMPLES[freq_i][_sampPos] * 
						_DD_PX_VALS_NOR[j];				
					_ACC_SIN_VAL[freq_i] += _DD_PX_VALS_SIN[freq_i][j];
				}
				// next buffer position -> next sample position
				_nextSampPos();
			}
		}
		// else only one new value needs to be transformed
		else
		{
			_DD_PX_VALS_NOR[_BUFF_POS] = _normalizeValue(_DD_PX_VALS_RAW[_BUFF_POS]);

			for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
			{
				_ACC_COS_VAL[freq_i] -= _DD_PX_VALS_COS[freq_i][_BUFF_POS];

				_DD_PX_VALS_COS[freq_i][_BUFF_POS] = 
					DotDetectorLayer::DD_FREQS_COSSAMPLES[freq_i][_sampPos] * 
					_DD_PX_VALS_NOR[_BUFF_POS];

				_ACC_COS_VAL[freq_i] += _DD_PX_VALS_COS[freq_i][_BUFF_POS];

				_ACC_SIN_VAL[freq_i] -= _DD_PX_VALS_SIN[freq_i][_BUFF_POS];

				_DD_PX_VALS_SIN[freq_i][_BUFF_POS] = 
					DotDetectorLayer::DD_FREQS_SINSAMPLES[freq_i][_sampPos] * 
					_DD_PX_VALS_NOR[_BUFF_POS];
				_ACC_SIN_VAL[freq_i] += _DD_PX_VALS_SIN[freq_i][_BUFF_POS];
			}
			_nextSampPos();
		}
		// leave DotDetector in a state ready to calculcate FREQ_SCORES
	}
	// expect buffer to be filled with values up to DotDetector::_BUFF_POS
	void DotDetector::_execDetection()
	{
		// if amplitude is zero, no further detection needed
		if(_AMPLITUDE == 0)
		{
			DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
			return;
		}

		for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
		{
			// score_i = sinSum_i^2 + cosSum_i^2
			_DD_FREQ_SCORES[freq_i] = 
				_ACC_SIN_VAL[freq_i]*_ACC_SIN_VAL[freq_i] +
				_ACC_COS_VAL[freq_i]*_ACC_COS_VAL[freq_i];
		}

		// sort values
		std::sort(_DD_FREQ_SCORES.begin(),_DD_FREQ_SCORES.end());

		double potential = 0;

		for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
		{
			potential += (freq_i+1)*_DD_FREQ_SCORES[freq_i];
		}

		potential *= _AMPLITUDE;

		DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = potential;

		if(potential > DotDetectorLayer::DD_MIN_POTENTIAL)
		{
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = true;
			DotDetectorLayer::DD_SIGNALS_NUMBER++;
		}
		else
		{
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
		}
	}
	inline double DotDetector::_normalizeValue(uchar u)
	{
		return ((static_cast<double>(u) - _MIN) / _AMPLITUDE)*2-1;
	}

	inline void DotDetector::_nextSampPos()
	{
		// values between [0;frame_rate-1]
		_sampPos++;
		_sampPos %= DotDetectorLayer::FRAME_RATEi;
	}

	void DotDetector::nextBuffPos()
	{
		DotDetector::_BUFF_POS++;
		DotDetector::_BUFF_POS%= WDD_FBUFFER_SIZE;
	}
} /* namespace WaggleDanceDetector */