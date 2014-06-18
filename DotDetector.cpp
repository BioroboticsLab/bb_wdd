#include "stdafx.h"
#include "DotDetector.h"
#include "DotDetectorLayer.h"

namespace wdd {

	std::size_t DotDetector::_BUFF_POS;
	std::size_t DotDetector::_NRCALL_EXECFULL=0;
	std::size_t DotDetector::_NRCALL_EXECSING=0;
	std::size_t DotDetector::_NRCALL_EXECSLEP=0;
	std::vector<uchar> DotDetector::_AMP_SAMPLES;

	/*
	public functions
	*/
	unsigned __int64 inline GetRDTSC() {
		__asm {
			; Flush the pipeline
				XOR eax, eax
				CPUID
				; Get RDTSC counter in edx:eax
				RDTSC
		}
	}

	DotDetector::DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr):
		// copy passed unique id 
		//(= position in arrays of DotDetectorLayer)
		_UNIQUE_ID(UNIQUE_ID),
		// copy passed pixel source pointer 
		//(= position in cv::Mat frame of DotDetectorLayer)
		aux_pixel_ptr(pixel_src_ptr),
		_OLDMINGONE(false),_OLDMAXGONE(false),_NEWMINHERE(false),_NEWMAXHERE(false), _NEWMINMAX(false),
		_MAX(255),_MIN(0),_AMPLITUDE(0),_sampPos(0)
	{
		_DD_PX_VALS_COS.fill(0);
		_DD_PX_VALS_SIN.fill(0);

		_DD_PX_VALS_RAW.fill(0);
		_DD_PX_VALS_NOR.fill(0);

		_UINT8_PX_VALS_COUNT.fill(0);

		_DD_FREQ_SCORES.fill(0);
		_ACC_SIN_VAL.fill(0);
		_ACC_COS_VAL.fill(0);

		/*		
		printf("aux_pixel_ptr: %d\n",
		offsetof(DotDetector, DotDetector::aux_pixel_ptr));
		printf("_UNIQUE_ID: %d\n",
		offsetof(DotDetector, DotDetector::_UNIQUE_ID));
		printf("_sampPos: %d\n",
		offsetof(DotDetector, DotDetector::_sampPos));
		printf("_MIN: %d\n",
		offsetof(DotDetector, DotDetector::_MIN));
		printf("_MAX: %d\n",
		offsetof(DotDetector, DotDetector::_MAX));
		printf("_AMPLITUDE: %d\n",
		offsetof(DotDetector, DotDetector::_AMPLITUDE));
		printf("_NEWMINMAX: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMINMAX));
		printf("_OLDMINGONE: %d\n",
		offsetof(DotDetector, DotDetector::_OLDMINGONE));
		printf("_OLDMAXGONE: %d\n",
		offsetof(DotDetector, DotDetector::_OLDMAXGONE));
		printf("_NEWMINHERE: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMINHERE));
		printf("_NEWMAXHERE: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMAXHERE));

		printf("_DD_PX_VALS_RAW: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_RAW));
		printf("_UINT8_PX_VALS_COUNT: %d\n",
		offsetof(DotDetector, DotDetector::_UINT8_PX_VALS_COUNT));
		printf("_DD_PX_VALS_NOR: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_NOR));
		printf("_DD_PX_VALS_SIN: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_SIN));
		printf("_DD_PX_VALS_COS: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_COS));
		printf("_DD_FREQ_SCORES: %d\n",
		offsetof(DotDetector, DotDetector::_DD_FREQ_SCORES));
		printf("_ACC_SIN_VAL: %d\n",
		offsetof(DotDetector, DotDetector::_ACC_SIN_VAL));
		printf("_ACC_COS_VAL: %d\n",
		offsetof(DotDetector, DotDetector::_ACC_COS_VAL));

		exit(0);
		*/		
	}

	DotDetector::~DotDetector(void)
	{
	}

	// add pixel values until buffer is full (initial filling phase)
	void DotDetector::copyInitialPixel(bool doDetection)
	{	
		// while initializing no deletetion (overwriting) occurs
		// expect _BUFF_POS have increasing values of [0;WDD_FBUFFER_SIZE-1]

		// save raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *aux_pixel_ptr;

		// increase count of pixel value
		_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]++;

		// check if it is last call to _copyInitialPixel, all _DD_PX_VALS_RAW[] set
		if(doDetection)
		{
			_getInitialNewMinMax();

			if(_AMPLITUDE == 0)
			{
				DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
				DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
				return;
			}

			_execFullCalculation();
			_execDetection();
		}
	}

	void DotDetector::copyPixelAndDetect()
	{
		// handle oldest raw pixel (which will be overwritten by newest raw pixel)

		// decrease count of old raw pixel value, and if it is removed..
		if((--_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]) == 0)
		{
			//..check if a former min/max is gone
			if(_DD_PX_VALS_RAW[_BUFF_POS ] == _MIN)
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
		_DD_PX_VALS_RAW[_BUFF_POS] = *aux_pixel_ptr;

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
			_AMPLITUDE_INV = 1.0f /_AMPLITUDE;
			// set flags
			_OLDMINGONE = _OLDMAXGONE = _NEWMINHERE = _NEWMAXHERE = false;
			_NEWMINMAX = true;
		}

		// if amplitude is zero, no further detection needed
		if(_AMPLITUDE < 13)
		{
			DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
			DotDetector::_NRCALL_EXECSLEP++;
			_NEWMINMAX = true;
			return;
		}

		// when new min/max encountered,..
		if(_NEWMINMAX)
		{
			DotDetector::_NRCALL_EXECFULL++;
			_execFullCalculation();
			_NEWMINMAX = false;
		}
		// else only one new value needs to be transformed
		else
		{
			DotDetector::_NRCALL_EXECSING++;
			_execSingleCalculation();
		}

		// leave DotDetector in a state ready to calculcate FREQ_SCORES
		_execDetection();
	}

	/*
	private functions
	*/
	inline void DotDetector::_getInitialNewMinMax()
	{
		while(_UINT8_PX_VALS_COUNT[_MAX] == 0)
			_MAX--;

		while(_UINT8_PX_VALS_COUNT[_MIN] == 0)
			_MIN++;

		_AMPLITUDE = _MAX - _MIN;
		_AMPLITUDE_INV = 1.0f /_AMPLITUDE;
	}
	inline void DotDetector::_execFullCalculation()
	{
		// calculate normalization for all values, new min/max set already
		//arma::Row<uchar> t = arma::Row<uchar>((uchar *)&_DD_PX_VALS_RAW.begin(), WDD_FBUFFER_SIZE, false, true);
		_DD_PX_VALS_NOR = arma::conv_to<arma::Row<float>>::from(arma::Row<uchar>((uchar *)&_DD_PX_VALS_RAW, WDD_FBUFFER_SIZE, false, true));

		//		_DD_PX_VALS_NOR = arma::conv_to<arma::Row<float>>::from(_DD_PX_VALS_RAW);

		//_DD_PX_VALS_NOR.print("_DD_PX_VALS_NOR");
		//_DD_PX_VALS_NOR = (((_DD_PX_VALS_NOR - _MIN) / _AMPLITUDE) * 2) -1;
		_DD_PX_VALS_NOR = _DD_PX_VALS_NOR - _MIN;
		_DD_PX_VALS_NOR = _DD_PX_VALS_NOR * _AMPLITUDE_INV;
		_DD_PX_VALS_NOR = _DD_PX_VALS_NOR *2;
		_DD_PX_VALS_NOR = _DD_PX_VALS_NOR -1;

		// reset accumulators to zero
		_ACC_COS_VAL.fill(0);
		_ACC_SIN_VAL.fill(0);

		// recalculate cos/sin values and set accumulators
		// get correct starting position in ring buffer
		std::size_t startPos = (_BUFF_POS+1) % WDD_FBUFFER_SIZE;
		std::size_t currtPos;

		//_sampPos = 0;
		/*
		unsigned __int64 t1, t2 = 0;
		t1 = GetRDTSC();
		*/
		for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
		{
			currtPos = (j + startPos) % WDD_FBUFFER_SIZE;
			//currtPos = (currtPos < WDD_FBUFFER_SIZE ? currtPos : 0);

			for(std::size_t freq_i=0; freq_i<DotDetectorLayer::DD_FREQS_NUMBER; freq_i++)
			{
				_DD_PX_VALS_COS.at(currtPos, freq_i) = 
					DotDetectorLayer::DD_FREQS_COSSAMPLES.at(_sampPos, freq_i) * _DD_PX_VALS_NOR.at(currtPos);

				_ACC_COS_VAL.at(freq_i) += _DD_PX_VALS_COS.at(currtPos, freq_i);

				_DD_PX_VALS_SIN.at(currtPos, freq_i) =
					DotDetectorLayer::DD_FREQS_SINSAMPLES.at(currtPos, freq_i) * _DD_PX_VALS_NOR.at(currtPos);

				_ACC_SIN_VAL.at(freq_i) += _DD_PX_VALS_SIN.at(currtPos, freq_i);
			}
			// next buffer position -> next sample position
			_nextSampPos();
		}


		/*
		for(std::size_t freq_i=0; freq_i<WDD_FREQ_NUMBER; freq_i++)
		{
		for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
		{
		currtPos = (j + startPos)% WDD_FBUFFER_SIZE;

		_DD_PX_VALS_COS.at(currtPos, freq_i) =
		DotDetectorLayer::DD_FREQS_COSSAMPLES.at(_sampPos,freq_i) * _DD_PX_VALS_NOR.at(currtPos);

		_ACC_COS_VAL.at(freq_i) += _DD_PX_VALS_COS.at(currtPos, freq_i);

		_DD_PX_VALS_SIN.at(currtPos, freq_i) =
		DotDetectorLayer::DD_FREQS_SINSAMPLES.at(_sampPos,freq_i) * _DD_PX_VALS_NOR.at(currtPos);

		_ACC_SIN_VAL.at(freq_i) += _DD_PX_VALS_SIN.at(currtPos, freq_i);

		// next buffer position -> next sample position
		_nextSampPos();
		}
		}
		*/
		/*
		t2 = GetRDTSC();
		printf("%ul ticks\n", t2-t1);

		if(_UNIQUE_ID == 100)
			exit(0);
			*/
	}

	inline void DotDetector::_execSingleCalculation()
	{
		_normalizeValue(_DD_PX_VALS_RAW[_BUFF_POS], &_DD_PX_VALS_NOR[_BUFF_POS]);

		for(std::size_t freq_i=0; freq_i<WDD_FREQ_NUMBER; freq_i++)
		{
			_ACC_COS_VAL.at(freq_i) -= _DD_PX_VALS_COS.at(_BUFF_POS, freq_i);

			_DD_PX_VALS_COS.at(_BUFF_POS, freq_i) = 
				DotDetectorLayer::DD_FREQS_COSSAMPLES.at(_sampPos, freq_i) * _DD_PX_VALS_NOR.at(_BUFF_POS);

			_ACC_COS_VAL.at(freq_i) += _DD_PX_VALS_COS.at(_BUFF_POS, freq_i);

			_ACC_SIN_VAL.at(freq_i) -= _DD_PX_VALS_SIN.at(_BUFF_POS, freq_i);

			_DD_PX_VALS_SIN.at(_BUFF_POS, freq_i) = 
				DotDetectorLayer::DD_FREQS_SINSAMPLES.at(_sampPos,freq_i) * _DD_PX_VALS_NOR.at(_BUFF_POS);
			_ACC_SIN_VAL[freq_i] += _DD_PX_VALS_SIN.at(_BUFF_POS, freq_i);
		}
		_nextSampPos();
	}

	inline void DotDetector::_execDetection()
	{
		for(std::size_t freq_i=0; freq_i<WDD_FREQ_NUMBER; freq_i++)
		{
			// score_i = sinSum_i^2 + cosSum_i^2
			_DD_FREQ_SCORES.at(freq_i) = 
				_ACC_SIN_VAL.at(freq_i)*_ACC_SIN_VAL.at(freq_i) +
				_ACC_COS_VAL.at(freq_i)*_ACC_COS_VAL(freq_i);
		}

		// sort values
		_DD_FREQ_SCORES = arma::sort(_DD_FREQ_SCORES);

		double potential = 0;

		for(std::size_t freq_i=0; freq_i<WDD_FREQ_NUMBER; freq_i++)
		{
			potential += (freq_i+1)*_DD_FREQ_SCORES[freq_i];
		}

		potential *= _AMPLITUDE;

		DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = potential;

		if(potential > DotDetectorLayer::DD_MIN_POTENTIAL)
		{
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = true;
			DotDetectorLayer::DD_SIGNALS_NUMBER++;
			_AMP_SAMPLES.push_back(_AMPLITUDE);
		}
		else
		{
			DotDetectorLayer::DD_SIGNALS[_UNIQUE_ID] = false;
		}
	}
	inline void DotDetector::_normalizeValue(uchar u, float * f_ptr)
	{
		*f_ptr = static_cast<float>(u);		
		*f_ptr -= _MIN;
		*f_ptr *= _AMPLITUDE_INV;
		*f_ptr *= 2;
		*f_ptr -= 1;
	}

	inline void DotDetector::_nextSampPos()
	{
		// values between [0;frame_rate-1]
		//_sampPos++;
		//_sampPos %= DotDetectorLayer::FRAME_RATEi;
		_sampPos = ( ++_sampPos < WDD_FRAME_RATE ? _sampPos : 0);
	}

	void DotDetector::nextBuffPos()
	{
		DotDetector::_BUFF_POS++;
		DotDetector::_BUFF_POS%= WDD_FBUFFER_SIZE;
		//DotDetector::_BUFF_POS = ( ++DotDetector::_BUFF_POS < WDD_FBUFFER_SIZE ? DotDetector::_BUFF_POS : 0);
	}
} /* namespace WaggleDanceDetector */