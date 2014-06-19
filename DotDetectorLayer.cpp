#include "stdafx.h"
#include "DotDetectorLayer.h"
#include "WaggleDanceDetector.h"

namespace wdd {
	// is this how c++ works? 
	std::vector<cv::Point2i>	DotDetectorLayer::DD_POSITIONS;
	std::size_t					DotDetectorLayer::DD_NUMBER;
	double						DotDetectorLayer::DD_MIN_POTENTIAL;
	double *					DotDetectorLayer::DD_POTENTIALS;
	bool *						DotDetectorLayer::DD_SIGNALS;
	std::size_t					DotDetectorLayer::DD_SIGNALS_NUMBER;
	double 						DotDetectorLayer::DD_FREQ_MIN;
	double 						DotDetectorLayer::DD_FREQ_MAX;
	double 						DotDetectorLayer::DD_FREQ_STEP;
	std::size_t					DotDetectorLayer::FRAME_RATEi;
	double						DotDetectorLayer::FRAME_REDFAC;
	std::vector<double>			DotDetectorLayer::DD_FREQS;
	std::size_t					DotDetectorLayer::DD_FREQS_NUMBER;
	SAMP						DotDetectorLayer::SAMPLES[WDD_FRAME_RATE];

	//arma::Mat<float>::fixed<WDD_FRAME_RATE,WDD_FREQ_NUMBER> DotDetectorLayer::DD_FREQS_COSSAMPLES;
	//arma::Mat<float>::fixed<WDD_FRAME_RATE,WDD_FREQ_NUMBER> DotDetectorLayer::DD_FREQS_SINSAMPLES;
	DotDetector **				DotDetectorLayer::_DotDetectors;

	void DotDetectorLayer::init(std::vector<cv::Point2i> dd_positions, cv::Mat * frame_ptr, 
		std::vector<double> ddl_config)
	{
		DotDetectorLayer::_setDDLConfig(ddl_config);

		// save DD_POSITIONS
		DotDetectorLayer::DD_POSITIONS = dd_positions;

		// get number of DotDetectors
		DotDetectorLayer::DD_NUMBER = DotDetectorLayer::DD_POSITIONS.size();

		// allocate space for DotDetector potentials
		DotDetectorLayer::DD_POTENTIALS = new double[DotDetectorLayer::DD_NUMBER];

		// allocate space for DotDetector signals
		DotDetectorLayer::DD_SIGNALS = new bool[DotDetectorLayer::DD_NUMBER];	

		// allocate DotDetector pointer array
		DotDetectorLayer::_DotDetectors = new DotDetector * [DD_NUMBER];

		// create DotDetectors, pass unique id and location of pixel
		for(std::size_t i=0; i<DD_NUMBER; i++)
		{
			DotDetectorLayer::_DotDetectors[i] = new DotDetector(i, &((*frame_ptr).at<uchar>(DD_POSITIONS[i])));
			//std::cout<<sizeof(*DotDetectorLayer::_DotDetectors[i])<<std::endl;
			//std::cout<<&(*DotDetectorLayer::_DotDetectors[i])<<std::endl;

			//if(i==2)
			//exit(0);
		}

	}

	void DotDetectorLayer::release(void)
	{
		for(std::size_t i=0; i<DotDetectorLayer::DD_NUMBER; i++)
			delete DotDetectorLayer::_DotDetectors[i];

		delete DotDetectorLayer::_DotDetectors;
		delete DotDetectorLayer::DD_POTENTIALS;
		delete DotDetectorLayer::DD_SIGNALS;
	}

	void DotDetectorLayer::copyFrame(bool doDetection)
	{
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;
	
		for(std::size_t i=0; i<DotDetectorLayer::DD_NUMBER; i++)
			DotDetectorLayer::_DotDetectors[i]->copyInitialPixel(doDetection);

		DotDetector::nextBuffPos();
	}

	void DotDetectorLayer::copyFrameAndDetect()
	{
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;

		for(std::size_t i=0; i<DotDetectorLayer::DD_NUMBER; i++)
			DotDetectorLayer::_DotDetectors[i]->copyPixelAndDetect();
		/*
		extern std::vector<unsigned __int64> loop_bench_res_sing, loop_bench_avg;

		unsigned __int64 sum = 0;
		for(auto it=loop_bench_res_sing.begin(); it!=loop_bench_res_sing.end(); ++it)
			sum += *it;

		sum /= loop_bench_res_sing.size();

		loop_bench_avg.push_back(sum);
		*/
		DotDetector::nextBuffPos();
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
		if(dd_frame_redfac<=0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_frame_redfac<=0!"<<std::endl;
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

			SAMPLES[s].c0 = static_cast<float>(cos(fac * dd_freqs[0] * step * s));
			SAMPLES[s].s0 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c1 = static_cast<float>(cos(fac * dd_freqs[1] * step * s));
			SAMPLES[s].s1 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c2 = static_cast<float>(cos(fac * dd_freqs[2] * step * s));
			SAMPLES[s].s2 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c3 = static_cast<float>(cos(fac * dd_freqs[3] * step * s));
			SAMPLES[s].s3 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c4 = static_cast<float>(cos(fac * dd_freqs[4] * step * s));
			SAMPLES[s].s4 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c5 = static_cast<float>(cos(fac * dd_freqs[5] * step * s));
			SAMPLES[s].s5 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c6 = static_cast<float>(cos(fac * dd_freqs[6] * step * s));	
			SAMPLES[s].s6 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));
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

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[0]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c0);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[0]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s0);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[1]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c1);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[1]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s1);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[2]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c2);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[2]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s2);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[3]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c3);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[3]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s3);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[4]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c4);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[4]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s4);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[5]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c5);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[5]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s5);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[6]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c6);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[6]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s6);
		printf("\n");

	}
} /* namespace WaggleDanceDetector */