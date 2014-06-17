#include "stdafx.h"
#include "DotDetector.h"
#include "DotDetectorLayer.h"

namespace wdd {
	
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
		_MAX = 255;
		_MIN = 0;
		_AMPLITUDE = 0;

		_UINT8_PX_VALS_COUNT.fill(0);

		_OLDMINGONE = _OLDMAXGONE = _NEWMINHERE = _NEWMAXHERE = _NEWMINMAX = false;

		_sampPos = 0;

		_DD_PX_VALS_SIN = new double * [DotDetectorLayer::DD_FREQS_NUMBER];
		_DD_PX_VALS_COS = new double * [DotDetectorLayer::DD_FREQS_NUMBER];

		for(std::size_t i=0; i<DotDetectorLayer::DD_FREQS_NUMBER; i++)
		{
			// init and set to zero
			_DD_PX_VALS_SIN[i] = new double [WDD_FBUFFER_SIZE];
			std::fill(_DD_PX_VALS_SIN[i], _DD_PX_VALS_SIN[i]+WDD_FBUFFER_SIZE, 0);

			_DD_PX_VALS_COS[i] = new double [WDD_FBUFFER_SIZE];
			std::fill(_DD_PX_VALS_COS[i], _DD_PX_VALS_COS[i]+WDD_FBUFFER_SIZE, 0);
		}

		_DD_PX_VALS_RAW.fill(0);

		//DEBUG ONLY
		//AUX_DEB_DD_RAW_BUFFERS = arma::Row<arma::uword>(WDD_FBUFFER_SIZE);
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
	}
	// add pixel values until buffer is full (initial filling phase)
	void DotDetector::copyPixel(bool doDetection)
	{	
		_copyInitialPixel(doDetection);
	}

	/*
	private functions
	*/
	inline void DotDetector::_copyInitialPixel(bool doDetection)
	{
		// while initializing no deletetion (overwriting) occurs
		// expect _BUFF_POS have increasing values of [0;WDD_FBUFFER_SIZE-1]

		// save raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *_pixel_src_ptr;

		// increase count of pixel value
		_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]++;

		// DEBUG
		//AUX_DEB_DD_RAW_BUFFERS[_BUFF_POS] = _DD_PX_VALS_RAW[_BUFF_POS];

		//_getNewMinMax();

		// check if it is last call to _copyInitialPixel, all _DD_PX_VALS_RAW[] set
		if(doDetection)
		{
			_getInitialNewMinMax();
			_execFullCalculation();
			_execDetection();
		}
	}
	void DotDetector::_copyPixel()
	{
		// handle oldest raw pixel (which will be overwritten by newest raw pixel)

		// decrease count of old raw pixel value, and if it is removed..
		if((--_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]) == 0)
		{
			//..check if a former min/max is gone
			if(_DD_PX_VALS_RAW[_BUFF_POS] == _MIN)
			{
				_OLDMINGONE = true;				
				// set new min
				while(_UINT8_PX_VALS_COUNT[_MIN] == 0)
					_MIN++;
			}
			else if(_DD_PX_VALS_RAW[_BUFF_POS] == _MAX)
			{
				_OLDMAXGONE = true;
				// set new max
				while(_UINT8_PX_VALS_COUNT[_MAX] == 0)
					_MAX--;
			}
		}

		// save new raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *_pixel_src_ptr;

		// increase count of new raw pixel value, and if it is added..
		if((++_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]) == 1)
		{
			//..check if former min/max is valid
			if(_DD_PX_VALS_RAW[_BUFF_POS] < _MIN)
			{
				_NEWMINHERE = true;
				// set new min
				_MIN = _DD_PX_VALS_RAW[_BUFF_POS];
			}
			else if(_DD_PX_VALS_RAW[_BUFF_POS] > _MAX)
			{
				_NEWMAXHERE = true;
				// set new max
				_MAX = _DD_PX_VALS_RAW[_BUFF_POS];
			}
		}

		// DEBUG
		//AUX_DEB_DD_RAW_BUFFERS[_BUFF_POS] = _DD_PX_VALS_RAW[_BUFF_POS];

		if(_OLDMINGONE | _OLDMAXGONE | _NEWMINHERE | _NEWMAXHERE)
		{
			_AMPLITUDE = _MAX - _MIN;

			// set flags
			_OLDMINGONE = _OLDMAXGONE = _NEWMINHERE = _NEWMAXHERE = false;
			_NEWMINMAX = true;
		}

		// if amplitude is zero, no further detection needed
		if(_AMPLITUDE == 0)
		{
			DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
			return;
		}

		// when new min/max encountered,..
		if(_NEWMINMAX)
		{
			_execFullCalculation();
			_NEWMINMAX = false;
		}
		// else only one new value needs to be transformed
		else
		{
			_execSingleCalculation();
		}

		// leave DotDetector in a state ready to calculcate FREQ_SCORES
		_execDetection();
	}
	inline void DotDetector::_getInitialNewMinMax()
	{
		while(_UINT8_PX_VALS_COUNT[_MAX] == 0)
			_MAX--;

		while(_UINT8_PX_VALS_COUNT[_MIN] == 0)
			_MIN++;

		_AMPLITUDE = _MAX - _MIN;
	}
	inline void DotDetector::_execFullCalculation()
	{
		// calculate normalization for all values, new min/max set already
		for(std::size_t i=0; i<WDD_FBUFFER_SIZE; i++)
			_DD_PX_VALS_NOR[i] = _normalizeValue(_DD_PX_VALS_RAW[i]);

		// reset accumulators to zero
		_ACC_COS_VAL.fill(0);
		_ACC_SIN_VAL.fill(0);

		// recalculate cos/sin values and set accumulators
		// get correct starting position in ring buffer
		std::size_t startPos = (_BUFF_POS+1) % WDD_FBUFFER_SIZE;
		std::size_t curetPos;

		for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
		{
			curetPos = (j + startPos)% WDD_FBUFFER_SIZE;

			for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
			{
				_DD_PX_VALS_COS[freq_i][curetPos] = 
					DotDetectorLayer::DD_FREQS_COSSAMPLES[freq_i][_sampPos] * 
					_DD_PX_VALS_NOR[curetPos];

				_ACC_COS_VAL[freq_i] += _DD_PX_VALS_COS[freq_i][curetPos];

				_DD_PX_VALS_SIN[freq_i][curetPos] = 
					DotDetectorLayer::DD_FREQS_SINSAMPLES[freq_i][_sampPos] * 
					_DD_PX_VALS_NOR[curetPos];

				_ACC_SIN_VAL[freq_i] += _DD_PX_VALS_SIN[freq_i][curetPos];
			}
			// next buffer position -> next sample position
			_nextSampPos();
		}

		/*
		for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
		{
		for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
		{
		curetPos = (j + startPos)% WDD_FBUFFER_SIZE;

		_DD_PX_VALS_COS[freq_i][curetPos] = 
		DotDetectorLayer::DD_FREQS_COSSAMPLES[freq_i][_sampPos] * _DD_PX_VALS_NOR[curetPos];
		_ACC_COS_VAL[freq_i] += _DD_PX_VALS_COS[freq_i][curetPos];

		_DD_PX_VALS_SIN[freq_i][curetPos] = 
		DotDetectorLayer::DD_FREQS_SINSAMPLES[freq_i][_sampPos] * _DD_PX_VALS_NOR[curetPos];				
		_ACC_SIN_VAL[freq_i] += _DD_PX_VALS_SIN[freq_i][curetPos];

		_nextSampPos();
		}
		}
		*/
	}

	inline void DotDetector::_execSingleCalculation()
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

	inline void DotDetector::_execDetection()
	{

		for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
		{
			// score_i = sinSum_i^2 + cosSum_i^2
			_DD_FREQ_SCORES[freq_i] = 
				_ACC_SIN_VAL[freq_i]*_ACC_SIN_VAL[freq_i] +
				_ACC_COS_VAL[freq_i]*_ACC_COS_VAL[freq_i];

			//DEBUG
			//AUX_DD_FREQ_SCORES.at(freq_i) = _DD_FREQ_SCORES[freq_i];
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