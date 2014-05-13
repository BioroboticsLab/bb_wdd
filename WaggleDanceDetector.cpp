/*
* WaggleDanceDetector.cpp
*
*  Created on: 08.04.2014
*      Author: sam
*/
#include "stdafx.h"


namespace wdd
{

	WaggleDanceDetector::WaggleDanceDetector(
		std::vector<double> 	dd_freq_config,
		std::map<std::size_t,cv::Point2i> dd_pos_id2point_map,
		std::vector<double> 	frame_config,
		int 			wdd_fbuffer_size,
		std::vector<double> 	wdd_signal_dd_config,
		bool 			wdd_verbose
		)
	{
		WDD_VERBOSE = wdd_verbose;

		setSignalConfig(wdd_signal_dd_config);

		setFBufferConfig(wdd_fbuffer_size);

		setFrameConfig(frame_config);

		setFreqConfig(dd_freq_config);

		setPositions(dd_pos_id2point_map);

		initWDDSignalValues();
	}

	WaggleDanceDetector::~WaggleDanceDetector()
	{
		// TODO Auto-generated destructor stub
	}
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, DD_FREQS_NUMBER, DD_FREQS
	*/
	void WaggleDanceDetector::createFreqSamples()
	{
		/* set abbreviation variables */
		int L = WDD_FBUFFER_SIZE;
		int sf = (int)FRAME_RATE;

		/* create sample time vector t */
		arma::rowvec t = arma::linspace<arma::rowvec>(0,(L-1)*(1.0/sf),WDD_FBUFFER_SIZE);

		//TODO: reuse same allocation if more then 1 call to function -
		// possible memory leak?
		DD_FREQS_COSSAMPLES = arma::mat(DD_FREQS_NUMBER, WDD_FBUFFER_SIZE);
		DD_FREQS_SINSAMPLES = arma::mat(DD_FREQS_NUMBER, WDD_FBUFFER_SIZE);

		for(std::size_t i=0; i<DD_FREQS_NUMBER;i++)
		{
			double f = DD_FREQS[i];
			DD_FREQS_COSSAMPLES.row(i) = cos(2*arma::datum::pi*f*t);
			DD_FREQS_SINSAMPLES.row(i) = sin(2*arma::datum::pi*f*t);
		}

	}
	/*
	* Dependencies: DD_POSITIONS_NUMBER, DD_FREQS_NUMBER
	*/
	void WaggleDanceDetector::initDDSignalValues()
	{
		// init 2d DD_SCORES
		DD_SIGNAL_SCORES =  arma::mat(DD_POSITIONS_NUMBER, DD_FREQS_NUMBER);
		DD_SIGNAL_SCORES.fill(0);

		DD_SIGNAL_POTENTIALS = arma::rowvec(DD_POSITIONS_NUMBER);
		DD_SIGNAL_POTENTIALS.fill(0);

		DD_SIGNAL_BOOL = arma::rowvec(DD_POSITIONS_NUMBER);
		DD_SIGNAL_BOOL.fill(0);
	}
	/*
	* Dependencies: NULL
	*/
	void WaggleDanceDetector::initWDDSignalValues()
	{
		WDD_SIGNAL = false;
		WDD_SIGNAL_NUMBER = 0;
		WDD_SIGNAL_POSITIONS = arma::Mat<double>(0,2);
	}
	/*
	* Dependencies: WDD_VERBOSE
	*/
	void WaggleDanceDetector::setFBufferConfig(int wdd_fbuffer_size)
	{
		/* do some sanity checks */
		if(wdd_fbuffer_size <= 0)
		{
			//TODO: throw exception..
		}
		/* assign passed value */
		WDD_FBUFFER_SIZE = wdd_fbuffer_size;

		/* assign implicit values */
		//TODO: rescue previous buffer values and position if call_nr > 1
		WDD_FBUFFER = std::deque<cv::Mat>(WDD_FBUFFER_SIZE);
		WDD_FBUFFER_POS = 0;

		/* finally present output if verbose mode is on*/
		if(WDD_VERBOSE)
		{
			printFBufferConfig();
		}
	}
	/* Given a DotDetector frame configuration consisting of rate, redfac, height,
	* width this function sets the appropriate attributes. This function shall be
	* designed in such a way that adjustments of frequency configuration may be
	* applied 'on the fly'*/
	/*
	* Dependencies: WDD_VERBOSE
	*/
	void WaggleDanceDetector::setFrameConfig(std::vector<double> dd_frame_config)
	{
		/* first check right number of arguments */
		if(dd_frame_config.size() != 4)
		{
			//TODO: throw exception..
		}

		/* copy values from dd_frame_config*/
		double frame_rate = dd_frame_config[0];
		double frame_redfac = dd_frame_config[1];
		int frame_height = (int) dd_frame_config[2];
		int frame_width = (int) dd_frame_config[3];

		/* TODO: do some sanity checks */

		/* assign passed values */
		FRAME_RATE = frame_rate;
		FRAME_REDFAC = frame_redfac;
		FRAME_HEIGHT = frame_height;
		FRAME_WIDTH = frame_width;

		/* finally present output if verbose mode is on*/
		if(WDD_VERBOSE)
		{
			printFrameConfig();
		}
	}
	/* Given a DotDetector frequency configuration consisting of min:step:max this
	* function calculates each single frequency value and triggers
	* createFreqSamples(), which creates the appropriate sinus and cosinus samples.
	* This function shall be designed in such a way that adjustments of frequency
	* configuration may be applied 'on the fly'*/
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, WDD_VERBOSE
	*/
	void WaggleDanceDetector::setFreqConfig(std::vector<double> dd_freq_config)
	{
		/* first check right number of arguments */
		if(dd_freq_config.size() != 3)
		{
			//TODO: throw exception..
		}

		/* copy values from dd_freq_config*/
		double dd_freq_min = dd_freq_config[0];
		double dd_freq_max = dd_freq_config[1];
		double dd_freq_step = dd_freq_config[2];

		/* do some sanity checks */
		if(dd_freq_min > dd_freq_max)
		{
			//TODO: throw exception..
		}
		if((dd_freq_max-dd_freq_min)<dd_freq_step)
		{
			//TODO: throw exception (warning?)..
		}

		/* assign passed values */
		DD_FREQ_MIN = dd_freq_min;
		DD_FREQ_MAX = dd_freq_max;
		DD_FREQ_STEP = dd_freq_step;

		/* calculate frequencies */
		std::vector<double> dd_freqs;
		double dd_freq_cur = dd_freq_min;

		while(dd_freq_cur<=dd_freq_max)
		{
			dd_freqs.push_back(dd_freq_cur);
			dd_freq_cur += dd_freq_step;
		}

		/* assign frequencies */
		DD_FREQS_NUMBER = dd_freqs.size();
		DD_FREQS = dd_freqs;

		/* invoke sinus and cosinus sample creation */
		createFreqSamples();

		/* finally present output if verbose mode is on*/
		if(WDD_VERBOSE)
		{
			printFreqConfig();
			printFreqSamples();
		}

	}
	/*
	* Dependencies: WDD_VERBOSE
	*/
	void WaggleDanceDetector::setSignalConfig(std::vector<double> wdd_signal_dd_config)
	{
		/* first check right number of arguments */
		if(wdd_signal_dd_config.size() != 3)
		{
			//TODO: throw exception..
		}
		/* copy values from dd_frame_config*/
		double wdd_signal_dd_maxdistance = wdd_signal_dd_config[0];
		int wdd_signal_dd_min_cluster_size = (int) wdd_signal_dd_config[1];
		double wdd_signal_dd_min_score =  wdd_signal_dd_config[2];

		/* TODO: do some sanity checks */

		/* assign passed values */
		WDD_SIGNAL_DD_MAXDISTANCE = wdd_signal_dd_maxdistance;
		WDD_SIGNAL_DD_MIN_CLUSTER_SIZE = wdd_signal_dd_min_cluster_size;
		WDD_SIGNAL_DD_MIN_SCORE = wdd_signal_dd_min_score;

		/* finally present output if verbose mode is on*/
		if(WDD_VERBOSE)
		{
			printSignalConfig();
		}
	}
	/* Given DotDetector positions as n-by-2 matrix this function accordingly sets
	* the attributes and triggers initDDScoreValues(), which initializes the
	* appropriate sizes for the attributes dependent on dd_positions. This function
	*  shall be designed in such a way that adjustments of frequency configuration
	*  may be applied 'on the fly'*/
	/*
	* Dependencies: WDD_VERBOSE
	*/
	void WaggleDanceDetector::setPositions(std::map<std::size_t,cv::Point2i> dd_pos_id2point_map)
	{
		if(dd_pos_id2point_map.size() < 1)
		{
			//TODO: Print warning..
		}

		/* assign passed values */
		DD_POS_ID2POINT_MAP = dd_pos_id2point_map;
		DD_POSITIONS_NUMBER = dd_pos_id2point_map.size();

		/* finally present output if verbose mode is on*/
		if(WDD_VERBOSE)
		{
			printPositionConfig();			
		}
		initDDSignalValues();
	}
	/*
	* expect the frame to be in target format, i.e. resolution and grayscale,
	* enhanced etc.
	*/
	void WaggleDanceDetector::addFrame(cv::Mat frame, unsigned long long frame_nr,
		bool imidiateDetect)
	{	
		//		std::cout<<"value: "<< frame <<std::endl<< "adress: " << &frame << std::endl;
		// either fill frame buffer
		if(WDD_FBUFFER_POS < WDD_FBUFFER_SIZE)
		{
			WDD_FBUFFER.push_back(cv::Mat(frame.clone()));
			WDD_FBUFFER_POS++;
			if(WDD_VERBOSE)
				std::cout<<"Fill frame buffer."<<std::endl;

		}
		// or replace oldest frame trough shift
		else
		{
			WDD_FBUFFER.pop_front();
			WDD_FBUFFER.push_back(cv::Mat(frame.clone()));
			if(WDD_VERBOSE)
				std::cout<<"Swap frame buffer."<<std::endl;
		}
		// pass frame number
		WDD_SIGNAL_FRAME_NR = frame_nr;

		if(imidiateDetect)
		{
			execDetection();
		}
	}
	void WaggleDanceDetector::execDetection()
	{
		// check buffer is completly filled
		if(WDD_FBUFFER_POS < WDD_FBUFFER_SIZE)
			return;

		// reset detection relevant fields
		DD_SIGNAL_BOOL.fill(0);
		DD_SIGNAL_POTENTIALS.fill(0);
		DD_SIGNAL_SCORES.fill(0);

		// run detection on DotDetector layer
		execDetectionDDScores();
	}
	/*
	* for each defined DotDetector in DD_POS_ID2POINT_MAP retrieve the raw signal (typicaly uint8)
	* from framebuffer, normalize it to -1:1, get the frequency scores 
	*/
	void WaggleDanceDetector::execDetectionDDScores()
	{
		// TODO: use some memory inplace freaky stuff

		// container to hold raw uint8 frame values
		unsigned char * dd_signal_uin8 = new unsigned char[WDD_FBUFFER_SIZE];
		// container to hold [-1:1] scaled values
		arma::Col<double> dd_signal_nor(WDD_FBUFFER_SIZE);
		// container to hold frequency scores
		arma::Row<double> dd_signal_score(DD_FREQS_NUMBER);

		// TODO: remove fsCount in release
		std::size_t fsCount = 0;

		for(std::size_t j=0; j<DD_POSITIONS_NUMBER; j++)
		{
			std::size_t c = 0;
			// copy DotDetector signal from frame buffer, newest frame has lowest index
			// holy shit.. a lot of performance to save here
			for(auto it=WDD_FBUFFER.rbegin(); c < WDD_FBUFFER_SIZE; ++it)
			{
				dd_signal_uin8[c] =(*it).at<unsigned char>(DD_POS_ID2POINT_MAP.at(j));
				c++;
			}
			unsigned char minV_uin8 = *std::min_element(dd_signal_uin8,dd_signal_uin8+WDD_FBUFFER_SIZE);
			unsigned char maxV_uin8 = *std::max_element(dd_signal_uin8,dd_signal_uin8+WDD_FBUFFER_SIZE);

			// check for flat DotDetector signal
			if(minV_uin8 == maxV_uin8)
			{
				fsCount++;
				dd_signal_score.fill(0);
				// write DotDetector signal score back to data store 
				// TODO: check if needed in release?!
				DD_SIGNAL_SCORES.row(j) = dd_signal_score;
				continue;
			}

			// cast & copy from uint8 to double
			for(std::size_t i=0; i<WDD_FBUFFER_SIZE; i++)
			{
				dd_signal_nor.at(i) = static_cast<double>(dd_signal_uin8[i]);
			}

			// normalize values to [-1:1]
			double minVNor = static_cast<double>(minV_uin8);
			dd_signal_nor = dd_signal_nor - minVNor;
			double maxVNor = arma::max(dd_signal_nor);

			dd_signal_nor = dd_signal_nor / maxVNor;
			dd_signal_nor = dd_signal_nor * 2;
			dd_signal_nor = dd_signal_nor - 1;

			for(std::size_t i=0; i< DD_FREQS_NUMBER; i++)
			{
				double armaSinVal = arma::as_scalar(DD_FREQS_SINSAMPLES.row(i) * dd_signal_nor);
				double armaCosVal = arma::as_scalar(DD_FREQS_COSSAMPLES.row(i) * dd_signal_nor);
				dd_signal_score.at(i) = armaSinVal*armaSinVal + armaCosVal*armaCosVal;
			}

			// write DotDetector signal score back to data store 
			// TODO: check if needed in release?!
			DD_SIGNAL_SCORES.row(j) = dd_signal_score;

			// sort scores in ascending order
			arma::Row<double> dd_signal_score_ASC = arma::sort(dd_signal_score);

			double linearSum = 0;
			for(std::size_t i=0; i<dd_signal_score_ASC.size(); i++)
			{
				linearSum += (i+1)* dd_signal_score_ASC.at(i);
				//if(WDD_VERBOSE)
				//std::cout<<"["<<i<<"]: "<<dd_signal_score.at(i)<<std::endl;
			}

			//if(WDD_VERBOSE)
			//std::cout<<"linearSum: "<< linearSum <<" - pos: "<<j<<std::endl;


		}

		std::cout<<"# of possible fast skipps: "<< fsCount<< std::endl;
		delete[] dd_signal_uin8;
	}

	void WaggleDanceDetector::printFBufferConfig()
	{
		printf("Printing frame buffer configuration:\n");
		printf("[WDD_FBUFFER_SIZE] %d\n",WDD_FBUFFER_SIZE);
		printf("[WDD_FBUFFER_POS] %d\n",WDD_FBUFFER_POS);
	}
	void WaggleDanceDetector::printFrameConfig()
	{
		printf("Printing frame configuration:\n");
		printf("[FRAME_RATE] %.1f\n",FRAME_RATE);
		printf("[FRAME_REDFAC] %.1f\n",FRAME_REDFAC);
		printf("[FRAME_HEIGHT] %d\n",FRAME_HEIGHT);
		printf("[FRAME_WIDTH] %d\n",FRAME_WIDTH);
	}
	void WaggleDanceDetector::printFreqConfig()
	{
		printf("Printing frequency configuration:\n");
		printf("[DD_FREQ_MIN] %.1f\n",DD_FREQ_MIN);
		printf("[DD_FREQ_MAX] %.1f\n",DD_FREQ_MAX);
		printf("[DD_FREQ_STEP] %.1f\n",DD_FREQ_STEP);
		for(std::size_t i=0; i< DD_FREQS_NUMBER; i++)
			printf("[FREQ[%d]] %.1f Hz\n", i, DD_FREQS[i]);
	}
	void WaggleDanceDetector::printFreqSamples()
	{
		printf("Printing frequency samples:\n");
		for(std::size_t i=0; i< DD_FREQS_NUMBER; i++)
		{
			printf("[%.1f Hz (cos)] ", DD_FREQS[i]);
			DD_FREQS_COSSAMPLES.row(i).raw_print(std::cout);
			printf("[%.1f Hz (sin)] ", DD_FREQS[i]);
			DD_FREQS_SINSAMPLES.row(i).raw_print(std::cout);
		}
	}
	void WaggleDanceDetector::printSignalConfig()
	{
		printf("Printing signal configuration:\n");
		printf("[WDD_SIGNAL_DD_MAXDISTANCE] %.1f\n",WDD_SIGNAL_DD_MAXDISTANCE);
		printf("[WDD_SIGNAL_DD_MIN_CLUSTER_SIZE] %d\n",
			WDD_SIGNAL_DD_MIN_CLUSTER_SIZE);
		printf("[WDD_SIGNAL_DD_MIN_SCORE] %.1f\n",WDD_SIGNAL_DD_MIN_SCORE);
	}
	void WaggleDanceDetector::printPositionConfig()
	{
		printf("Printing position configuration:\n");
		printf("[dd_pos_id2point_map size] %d.\n", DD_POSITIONS_NUMBER);
	}

} /* namespace WaggleDanceDetector */
