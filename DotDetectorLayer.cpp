#include "stdafx.h"
#include "DotDetectorLayer.h"
#include "WaggleDanceDetector.h"
#include "ppl.h"

namespace wdd {
	// is this how c++ works? -> Not really!
	std::vector<cv::Point2i>	DotDetectorLayer::positions;
	std::size_t					DotDetectorLayer::DD_NUMBER;
	double						DotDetectorLayer::DD_MIN_POTENTIAL;
	double *					DotDetectorLayer::DD_POTENTIALS;
	std::size_t					DotDetectorLayer::DD_SIGNALS_NUMBER;
	std::vector<unsigned int>	DotDetectorLayer::DD_SIGNALS_IDs;
	double 						DotDetectorLayer::DD_FREQ_MIN;
	double 						DotDetectorLayer::DD_FREQ_MAX;
	double 						DotDetectorLayer::DD_FREQ_STEP;
	std::size_t					DotDetectorLayer::FRAME_RATEi;
	double						DotDetectorLayer::FRAME_REDFAC;
	std::vector<double>			DotDetectorLayer::DD_FREQS;
	std::size_t					DotDetectorLayer::DD_FREQS_NUMBER;
	SAMP						DotDetectorLayer::SAMPLES[WDD_FRAME_RATE];
	cv::Mat *					DotDetectorLayer::frame_ptr;
	//DotDetector **				DotDetectorLayer::_DotDetectors;

	void DotDetectorLayer::init(std::vector<cv::Point2i> positions, cv::Mat * _frame_ptr, 
		std::vector<double> ddl_config)
	{
		DotDetectorLayer::_setDDLConfig(ddl_config);

		// save positions
		DotDetectorLayer::positions = positions;

		// get number of DotDetectors
		DotDetectorLayer::DD_NUMBER = DotDetectorLayer::positions.size();

		// allocate space for DotDetector potentials
		DotDetectorLayer::DD_POTENTIALS = new double[DotDetectorLayer::DD_NUMBER];
		
		// allocate DotDetector pointer array
		//DotDetectorLayer::_DotDetectors = new DotDetector * [DD_NUMBER];

		DotDetectorLayer::DD_SIGNALS_IDs.reserve(WDD_LAYER2_MAX_POS_DDS);

		unsigned int resX = static_cast<unsigned int>(_frame_ptr->rows);
		unsigned int resY = static_cast<unsigned int>(_frame_ptr->cols);

		frame_ptr = _frame_ptr;
		_dotDetector.init(resX, resY, WDD_FBUFFER_SIZE);
		/*
		// create DotDetectors, pass unique id and location of pixel
		for(std::size_t i=0; i<DD_NUMBER; i++)
		{
			DotDetectorLayer::_DotDetectors[i] = new DotDetector(i, &((*frame_ptr).at<uchar>(positions[i])));
		}
		*/


	}

	void DotDetectorLayer::release(void)
	{
		//for(std::size_t i=0; i<DotDetectorLayer::DD_NUMBER; i++)
		//	delete DotDetectorLayer::_DotDetectors[i];

		//delete DotDetectorLayer::_DotDetectors;
		delete DotDetectorLayer::DD_POTENTIALS;
	}

	// expect to be used as initial function to fill buffer and have only one single call
	// with doDetection = true
	void DotDetectorLayer::copyFrame(bool doDetection)
	{
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;
		DotDetectorLayer::DD_SIGNALS_IDs.clear();
		//concurrency::parallel_for(std::size_t(0), DotDetectorLayer::DD_NUMBER, [&](size_t i)
		//{
		//	DotDetectorLayer::_DotDetectors[i]->copyInitialPixel(doDetection);
		//});// end for-loop
		//for (unsigned int i(0); i < DotDetectorLayer::DD_NUMBER; ++i) {
		//	DotDetectorLayer::_DotDetectors[i]->copyInitialPixel(doDetection);
		//}
		_dotDetector.addNewFrame(frame_ptr);
		if (doDetection)
			_dotDetector.detectDots();
		//DotDetector::nextBuffPos();
	}

	void DotDetectorLayer::copyFrameAndDetect()
	{
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;
		DotDetectorLayer::DD_SIGNALS_IDs.clear();
		//unsigned __int64 t1 = GetRDTSC();

		//concurrency::parallel_for(std::size_t(0), DotDetectorLayer::DD_NUMBER, [&](size_t i)
		//{
		//	DotDetectorLayer::_DotDetectors[i]->copyPixelAndDetect();
		//});// end for-loop

		//DotDetector::nextBuffPos();
		_dotDetector.addNewFrame(frame_ptr);
		_dotDetector.detectDots();
	}

	/* Given a DotDetector frequency configuration consisting of min:step:max this
	* function calculates each single frequency value and triggers
	* createFreqSamples(), which creates the appropriate sinus and cosinus samples.
	* This function shall be designed in such a way that adjustments of frequency
	* configuration may be applied 'on the fly'*/
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, WDD_VERBOSE
	*/
	void DotDetectorLayer::_setDDLConfig(std::vector<double> ddl_config)
	{
		/* first check right number of arguments */
		if(ddl_config.size() != 6)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: wrong number of config arguments!"<<std::endl;
			exit(-19);
		}

		/* copy values from ddl_config*/
		double dd_freq_min = ddl_config[0];
		double dd_freq_max = ddl_config[1];
		double dd_freq_step = ddl_config[2];
		double dd_frame_rate = ddl_config[3];
		double dd_frame_redfac = ddl_config[4];
		double dd_min_potential = ddl_config[5];

		/* do some sanity checks */
		if(dd_freq_min > dd_freq_max)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_freq_min > dd_freq_max!"<<std::endl;
			exit(-20);
		}
		if((dd_freq_max-dd_freq_min)<dd_freq_step)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: (dd_freq_max-dd_freq_min)<dd_freq_step!"<<std::endl;
			exit(-20);
		}
		if(dd_frame_rate<=0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_frame_rate<=0!"<<std::endl;
			exit(-21);
		}
		
		if(dd_frame_redfac<0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_frame_redfac<0!"<<std::endl;
			exit(-22);
		}
		
		if(dd_min_potential<=0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_min_score<=0!"<<std::endl;
			exit(-23);
		}

		/* assign passed values */
		DotDetectorLayer::DD_FREQ_MIN = dd_freq_min;
		DotDetectorLayer::DD_FREQ_MAX = dd_freq_max;
		DotDetectorLayer::DD_FREQ_STEP = dd_freq_step;
		DotDetectorLayer::FRAME_RATEi = static_cast<std::size_t>(dd_frame_rate);
		DotDetectorLayer::FRAME_REDFAC = dd_frame_redfac;
		DotDetectorLayer::DD_MIN_POTENTIAL = dd_min_potential;

		/* invoke sinus and cosinus sample creation */
		_createFreqSamples();

		/* finally present output if verbose mode is on*/
		if(WaggleDanceDetector::WDD_VERBOSE)
		{
			DotDetectorLayer::printFreqConfig();
			DotDetectorLayer::printFreqSamples();
		}
	}
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, DD_FREQS_NUMBER, DD_FREQS
	* Frame_rate needs to be integer type
	*/
	void DotDetectorLayer::_createFreqSamples()
	{
		/* calculate frequencies */
		std::vector<double> dd_freqs;
		double dd_freq_cur = DotDetectorLayer::DD_FREQ_MIN;

		while(dd_freq_cur<=DotDetectorLayer::DD_FREQ_MAX)
		{
			dd_freqs.push_back(dd_freq_cur);
			dd_freq_cur += DotDetectorLayer::DD_FREQ_STEP;
		}

		/* assign frequencies */
		DotDetectorLayer::DD_FREQS_NUMBER = dd_freqs.size();
		//DotDetector::DD_FREQS_NUMBER = dd_freqs.size();
		DotDetectorLayer::DD_FREQS = dd_freqs;

		double step = 1.0 / WDD_FRAME_RATE;

		for(std::size_t s=0; s<WDD_FRAME_RATE; s++)
		{
			double fac = 2*CV_PI;
			for (unsigned int i(0); i < WDD_FREQ_NUMBER; ++i) {
				SAMPLES[s].cosines[i] = static_cast<float>(cos(fac * dd_freqs[i] * step * s));
				SAMPLES[s].sines[i] = static_cast<float>(sin(fac * dd_freqs[i] * step * s));
			}
		}
	}
	void DotDetectorLayer::printFreqConfig()
	{
		printf("Printing frequency configuration:\n");
		printf("[DD_FREQ_MIN] %.1f\n",DotDetectorLayer::DD_FREQ_MIN);
		printf("[DD_FREQ_MAX] %.1f\n",DotDetectorLayer::DD_FREQ_MAX);
		printf("[DD_FREQ_STEP] %.1f\n",DotDetectorLayer::DD_FREQ_STEP);
		for(std::size_t i=0; i< DotDetectorLayer::DD_FREQS_NUMBER; i++)
			printf("[FREQ[%lu]] %.1f Hz\n", i, DotDetectorLayer::DD_FREQS[i]);
	}
	void DotDetectorLayer::printFreqSamples()
	{
		printf("Printing frequency samples:\n");

		for (unsigned int i(0); i < WDD_FREQ_NUMBER; ++i) {
			printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[i]);
			for (std::size_t j = 0; j< DotDetectorLayer::FRAME_RATEi; j++)
				printf("%.3f ", SAMPLES[j].cosines[i]);
			printf("\n");

			printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[i]);
			for (std::size_t j = 0; j< DotDetectorLayer::FRAME_RATEi; j++)
				printf("%.3f ", SAMPLES[j].sines[i]);
			printf("\n");
		}
	}
} /* namespace WaggleDanceDetector */