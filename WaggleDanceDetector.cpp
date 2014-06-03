/*
* WaggleDanceDetector.cpp
*
*  Created on: 08.04.2014
*      Author: sam
*/
#include "stdafx.h"


namespace wdd
{
	std::string danceFile_path = "C:\\Users\\Alexander Rau\\WaggleDanceDetector\\dance.txt";
	FILE * danceFile_ptr;
	std::string signalFile_path = "C:\\Users\\Alexander Rau\\WaggleDanceDetector\\signal.txt";
	FILE * signalFile_ptr;

	WaggleDanceDetector::WaggleDanceDetector(
		std::vector<double> 	dd_freq_config,
		std::map<std::size_t,cv::Point2i> dd_pos_id2point_map,
		std::vector<double> 	frame_config,
		int 			wdd_fbuffer_size,
		std::vector<double> 	wdd_signal_dd_config,
		bool wdd_write_signal_file,
		bool 			wdd_verbose,
		VideoFrameBuffer * videoFrameBuffer_ptr
		)
	{
		WDD_VideoFrameBuffer_ptr = videoFrameBuffer_ptr;
		WDD_VERBOSE = wdd_verbose;
		WDD_WRITE_SIGNAL_FILE = wdd_write_signal_file;

		setSignalConfig(wdd_signal_dd_config);

		setFBufferConfig(wdd_fbuffer_size);

		setFrameConfig(frame_config);

		setFreqConfig(dd_freq_config);

		setPositions(dd_pos_id2point_map);

		initWDDSignalValues();

		initWDDDanceValues();

		fopen_s (&danceFile_ptr, danceFile_path.c_str() , "a+" );
		fopen_s (&signalFile_ptr, signalFile_path.c_str() , "a+" );		
	}

	WaggleDanceDetector::~WaggleDanceDetector()
	{
		fclose (danceFile_ptr);
		fclose (signalFile_ptr);
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
	}
	/*
	* Dependencies: NULL
	*/
	void WaggleDanceDetector::initWDDDanceValues()
	{
		WDD_DANCE = false;
		WDD_DANCE_MAX_POS_DIST = sqrt(2);
		WDD_DANCE_MAX_FRAME_GAP = 3;
		WDD_DANCE_MIN_CONSFRAMES = 20;
		WDD_DANCE_ID = 0;
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
		double wdd_signal_dd_min_potential =  wdd_signal_dd_config[2];

		/* TODO: do some sanity checks */

		/* assign passed values */
		WDD_SIGNAL_DD_MAXDISTANCE = wdd_signal_dd_maxdistance;
		WDD_SIGNAL_DD_MIN_CLUSTER_SIZE = wdd_signal_dd_min_cluster_size;
		WDD_SIGNAL_DD_MIN_POTENTIAL = wdd_signal_dd_min_potential;

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
		// either fill frame buffer
		if(WDD_FBUFFER_POS < WDD_FBUFFER_SIZE)
		{
			WDD_FBUFFER.push_back(cv::Mat(frame.clone()));
			WDD_FBUFFER_POS++;
		}
		// or replace oldest frame trough shift
		else
		{
			WDD_FBUFFER.pop_front();
			WDD_FBUFFER.push_back(cv::Mat(frame.clone()));
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

		// run detection on DotDetector layer
		execDetectionGetDDPotentials();

		// run detection on WaggleDanceDetector layer
		execDetectionGetWDDSignals();

		// run top level detection, concat over time
		if(WDD_SIGNAL)
			execDetectionConcatWDDSignals();

		execDetectionHousekeepWDDSignals();
	}
	void WaggleDanceDetector::execDetectionConcatWDDSignals()
	{
		// preallocate 
		double dist;
		DANCE d;		

		bool danceFound;
		DANCE * d_ptr = NULL;
		bool newPosition;

		std::vector<cv::Point2d> * pos_vec_ptr;
		// foreach WDD_SIGNAL
		for(std::size_t i=0; i < WDD_SIGNAL_ID2POINT_MAP.size(); i++)
		{
			danceFound = false; newPosition=false;
			// check distance against known Dances
			for (auto it_dances=WDD_UNIQ_DANCES.begin();it_dances!=WDD_UNIQ_DANCES.end(); ++it_dances)
			{
				pos_vec_ptr = &(*it_dances).positions;

				// ..and their associated points
				for(auto it_pt=(*pos_vec_ptr).begin(); it_pt!=(*pos_vec_ptr).end(); ++it_pt)
				{
					dist = cv::norm(WDD_SIGNAL_ID2POINT_MAP[i] - *it_pt);

					// wdd signal belongs to known dance
					if( dist < WDD_DANCE_MAX_POS_DIST)
					{
						danceFound = true;
						d_ptr = &(*it_dances);

						// update dance
						// check if it is a new dance position - continue to calc distance for all positions, get MIN(dist)
						for(auto it_pt2=it_pt+1; it_pt2!=(*pos_vec_ptr).end(); ++it_pt2)
							dist = MIN(dist, cv::norm(WDD_SIGNAL_ID2POINT_MAP[i] - *it_pt2));

						if(dist > 0)
							newPosition = true;

						(*it_dances).DANCE_FRAME_END = WDD_SIGNAL_FRAME_NR;
						
						if(WDD_VERBOSE)
							std::cout<<WDD_SIGNAL_FRAME_NR<<" - UPD: "<<(*it_dances).DANCE_UNIQE_ID<<" ["<<(*it_dances).DANCE_FRAME_START<<","<<(*it_dances).DANCE_FRAME_END<<"]"<<std::endl;
						// jump to WDD_SIGNAL loop
						break;break;
					}
				}
			}

			if(!danceFound)
			{
				// if reached here, new DANCE encountered!
				d.DANCE_UNIQE_ID = WDD_DANCE_ID++;
				d.DANCE_FRAME_START = WDD_SIGNAL_FRAME_NR;
				d.DANCE_FRAME_END =  WDD_SIGNAL_FRAME_NR;
				d.positions.push_back(WDD_SIGNAL_ID2POINT_MAP[i]);
				WDD_UNIQ_DANCES.push_back(d);
			}
			else if(newPosition)
			{
				d_ptr->positions.push_back(WDD_SIGNAL_ID2POINT_MAP[i]);
			}
		}

		
	}
	void WaggleDanceDetector::execDetectionHousekeepWDDSignals()
	{
		//house keeping of Dance Signals
		for (auto it_dances=WDD_UNIQ_DANCES.begin();it_dances!=WDD_UNIQ_DANCES.end(); )
		{
			// if Dance did not recieve new signal for over WDD_UNIQ_SIGID_MAX_GAP frames
			if(WDD_SIGNAL_FRAME_NR - (*it_dances).DANCE_FRAME_END > WDD_DANCE_MAX_FRAME_GAP)
			{
				// check for minimum number of consecutiv frames for a positive dance
				if(((*it_dances).DANCE_FRAME_END - (*it_dances).DANCE_FRAME_START +1 ) >= WDD_DANCE_MIN_CONSFRAMES)
				{
					execDetectionFinalizeDance(*it_dances);
				}
				if(WDD_VERBOSE)
					std::cout<<WDD_SIGNAL_FRAME_NR<<" - DEL: "<<(*it_dances).DANCE_UNIQE_ID<<" ["<<(*it_dances).DANCE_FRAME_START<<","<<(*it_dances).DANCE_FRAME_END<<"]"<<std::endl;

				it_dances = WDD_UNIQ_DANCES.erase(it_dances);
			}
			else
			{
				++it_dances;
			}
		}
	}
	void WaggleDanceDetector::execDetectionFinalizeDance(DANCE d)
	{
		WDD_DANCE = true;
		WDD_UNIQ_FINISH_DANCES.push_back(d);
		
		execDetectionWriteDanceFileLine(d);

		std::vector<cv::Mat> seq = WDD_VideoFrameBuffer_ptr->loadFrameSequenc(d.DANCE_FRAME_START,d.DANCE_FRAME_END, cv::Point_<int>(d.positions[0]));

		WaggleDanceOrientator::extractOrientationFromImageSequence(seq, d.DANCE_UNIQE_ID);

	}

	void WaggleDanceDetector::execDetectionWriteDanceFileLine(DANCE d)
	{
		fprintf(danceFile_ptr, "%I64u %I64u %I64u", d.DANCE_FRAME_START, d.DANCE_FRAME_END, d.DANCE_UNIQE_ID);
		for (auto it=d.positions.begin(); it!=d.positions.end(); ++it)
		{
			fprintf(danceFile_ptr, " %.5f %.5f %.5f",
				(*it).x*pow(2, FRAME_REDFAC), (*it).y*pow(2, FRAME_REDFAC), 0);
		}
		fprintf(danceFile_ptr, "\n");
	}

	void WaggleDanceDetector::execDetectionWriteSignalFileLine()
	{
		fprintf(signalFile_ptr, "%I64u", WDD_SIGNAL_FRAME_NR);
		for (auto it=WDD_SIGNAL_ID2POINT_MAP.begin(); it!=WDD_SIGNAL_ID2POINT_MAP.end(); ++it)
		{
			fprintf(signalFile_ptr, " %.5f %.5f",
				it->second.x*pow(2, FRAME_REDFAC), it->second.y*pow(2, FRAME_REDFAC));
		}
		fprintf(signalFile_ptr, "\n");
	}
	/*
	* for each defined DotDetector in DD_POS_ID2POINT_MAP retrieve the raw signal (typicaly uint8)
	* from framebuffer, normalize it to -1:1, get the frequency scores 
	*/
	void WaggleDanceDetector::execDetectionGetDDPotentials()
	{
		// TODO: use some memory inplace freaky stuff

		// reset GetDDPotentials relevant fields
		DD_SIGNAL_SCORES.fill(0);
		DD_SIGNAL_POTENTIALS.fill(0);
		DD_SIGNAL_BOOL.fill(0);

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
			unsigned char AMPLITUDE = maxV_uin8 - minV_uin8;

			// check for flat DotDetector signal
			if(!AMPLITUDE)
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

			DD_SIGNAL_POTENTIALS.at(j) = AMPLITUDE * linearSum;

			if(DD_SIGNAL_POTENTIALS.at(j) > WDD_SIGNAL_DD_MIN_POTENTIAL)
			{
				DD_SIGNAL_BOOL.at(j) = 1;
				//DEBUG
				//std::cout << "POSITIVE DD " <<std::endl;
			}
		}

		//std::cout<<"# of possible fast skipps: "<< fsCount<< std::endl;
		delete[] dd_signal_uin8;
	}

	void WaggleDanceDetector::execDetectionGetWDDSignals()
	{
		// reset GetWDDSignals relevant fields
		WDD_SIGNAL = 0;
		WDD_SIGNAL_NUMBER = 0;
		WDD_SIGNAL_ID2POINT_MAP.clear();

		// test for minimum number of potent DotDetectors
		if(arma::sum(DD_SIGNAL_BOOL) < WDD_SIGNAL_DD_MIN_CLUSTER_SIZE)
			return;


		// get ids (=index in vector) of positive DDs
		// WARNING! Heavly rely on fact that :
		// - DD_ids are [0; DD_POSITIONS_NUMBER-1]
		// - length(DD_SIGNAL_BOOL) == DD_POSITIONS_NUMBER
		arma::Col<arma::uword> pos_DD_IDs = arma::find(DD_SIGNAL_BOOL);

		// init cluster ids for the positive DDs
		arma::Col<arma::sword> pos_DD_CIDs(pos_DD_IDs.size());
		pos_DD_CIDs.fill(-1);

		// init working copy of positive DD ids
		//std::vector<arma::uword> pos_DD_IDs_wset (pos_DD_IDs.begin(), pos_DD_IDs.end());

		// init unique cluster ID 
		unsigned int clusterID = 0;

		// foreach positive DD
		for(std::size_t i=0; i < pos_DD_IDs.size(); i++)
		{
			// check if DD is missing a cluster ID
			if(pos_DD_CIDs.at(i) >= 0)
				continue;

			// assign unique cluster id
			pos_DD_CIDs.at(i) = clusterID++;

			// assign source id (root id)
			arma::Col<arma::uword> root_DD_id(std::vector<arma::uword>(1,pos_DD_IDs.at(i)));

			// select only unclustered DD as working set
			arma::Col<arma::uword> pos_DD_unclustered_idx = arma::find(pos_DD_CIDs == -1);

			// make a local copy of positive ids from working set
			arma::Col<arma::uword> loc_pos_DD_IDs = pos_DD_IDs.rows(pos_DD_unclustered_idx);

			// init loop
			arma::Col<arma::uword> neighbour_DD_ids = 
				getNeighbours(root_DD_id, arma::Col<arma::uword>(), loc_pos_DD_IDs);

			arma::Col<arma::uword> Lneighbour_DD_ids;

			// loop until no new neighbours are added
			while(Lneighbour_DD_ids.size() != neighbour_DD_ids.size())
			{
				// get new discovered elements as
				// {D} = {N_i} \ {N_i-1}
				arma::Col<arma::uword> D = neighbour_DD_ids;
				for(std::size_t j=0; j<Lneighbour_DD_ids.size(); j++)
				{
					//MATLAB:  D(D==m) = [];
					arma::uvec q1 = arma::find(D == Lneighbour_DD_ids.at(j), 1);
					if(q1.size() == 1)
						D.shed_row(q1.at(0));
				}

				// save last neighbours id list
				Lneighbour_DD_ids = neighbour_DD_ids;

				// remove discovered elements from search
				for(std::size_t j=0; j<neighbour_DD_ids.size(); j++)
				{
					//MATLAB: loc_pos_DD_IDs(loc_pos_DD_IDs==m) = [];
					arma::uvec q1 = arma::find(loc_pos_DD_IDs == neighbour_DD_ids.at(j), 1);
					if(q1.size() == 1)
						loc_pos_DD_IDs.shed_row(q1.at(0));
				}

				neighbour_DD_ids = getNeighbours(D, neighbour_DD_ids, loc_pos_DD_IDs);
			}

			// set CIDs of all neighbours
			for(std::size_t j=0; j<neighbour_DD_ids.size(); j++)
			{
				arma::uvec q1 = arma::find(pos_DD_IDs == neighbour_DD_ids.at(j), 1);

				if(q1.size() == 1)
					pos_DD_CIDs.at(q1.at(0)) = pos_DD_CIDs.at(i);
				else
				{
					printf("ERROR! ::execDetectionGetWDDSignals: Unexpected assertion failure!\n");
					exit(19);
				}
			}
		}

		// assertion test
		arma::uvec q1 = arma::find(pos_DD_CIDs == -1);
		if(q1.size() > 0)
		{
			printf("ERROR! ::execDetectionGetWDDSignals: Unclassified DD signals!\n");
			exit(19);
		}

		// analyze cluster sizes
		arma::uvec count_unqClusterIDs(clusterID);
		count_unqClusterIDs.fill(0);

		// get size of each cluster
		for(std::size_t i=0; i<pos_DD_CIDs.size(); i++)
		{
			count_unqClusterIDs.at(pos_DD_CIDs.at(i))++;
		}

		arma::uvec f_unqClusterIDs;
		for(std::size_t i=0; i<count_unqClusterIDs.size(); i++)
		{
			if(count_unqClusterIDs.at(i) >= WDD_SIGNAL_DD_MIN_CLUSTER_SIZE)
			{
				f_unqClusterIDs.insert_rows(f_unqClusterIDs.size(),1);
				f_unqClusterIDs.at(f_unqClusterIDs.size()-1) = i;
			}
		}

		// decide if there is at least one WDD Signal
		if(f_unqClusterIDs.is_empty())
			return;

		WDD_SIGNAL = true;

		// foreach remaining cluster calculate center position
		for(std::size_t i=0; i<f_unqClusterIDs.size(); i++)
		{
			// find vecotr positions in pos_DD_CIDS <=> pos_DD_IDs 
			// associated with cluster id
			arma::uvec idx = arma::find(pos_DD_CIDs == f_unqClusterIDs.at(i));

			cv::Point2d center(0,0);
			for(std::size_t j=0; j<idx.size(); j++)
			{
				center += static_cast<cv::Point_<double>>(DD_POS_ID2POINT_MAP.find(pos_DD_IDs.at(idx.at(j)))->second);
			}

			center *= 1/static_cast<double>(idx.size());

			WDD_SIGNAL_ID2POINT_MAP[WDD_SIGNAL_NUMBER++] = center;
		}

		// toggel signal file creation
		if(WDD_WRITE_SIGNAL_FILE)
		{
			execDetectionWriteSignalFileLine();
		}
	}

	arma::Col<arma::uword> WaggleDanceDetector::getNeighbours(
		arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs)
	{
		// anchor case
		if(sourceIDs.size() == 1)
		{
			cv::Point2i DD_pos = DD_POS_ID2POINT_MAP.find(sourceIDs.at(0))->second;

			for(std::size_t i=0; i< set_DD_IDs.size(); i++)
			{
				cv::Point2i DD_pos_other = DD_POS_ID2POINT_MAP.find(set_DD_IDs.at(i))->second;

				// if others DotDetectors distance is in bound, add its ID
				if(std::sqrt(cv::norm(DD_pos-DD_pos_other)) < WDD_SIGNAL_DD_MAXDISTANCE)
				{
					N.insert_rows(N.size(), 1);
					N.at(N.size()-1) = set_DD_IDs.at(i);
				}
			}
		}
		// first recursive level call
		else if(sourceIDs.size() > 1)
		{
			for(std::size_t i=0; i< sourceIDs.size(); i++)
			{
				// remove discoverd neighbours from set
				for(std::size_t j=0; j< N.size(); j++)
				{
					arma::uvec q1 = arma::find(set_DD_IDs == N.at(j), 1);
					if(q1.size() == 1)
						set_DD_IDs.shed_row(q1.at(0));
					else if(q1.size() > 1)
					{
						printf("ERROR! ::getNeighbours: Unexpected number of DD Ids found!:\n");
						exit(19);
					}
				}

				// check if some ids are left
				if(set_DD_IDs.is_empty())
					break;

				arma::Col<arma::uword> _sourceIDs;
				_sourceIDs.insert_rows(0,1);
				_sourceIDs.at(0) = sourceIDs.at(i);

				N = getNeighbours(_sourceIDs, N, set_DD_IDs);
			}
		}
		else
		{
			printf("ERROR! ::getNeighbours: Unexpected assertion failure!\n");
			exit(19);
		}

		return N;
	}
	bool WaggleDanceDetector::isWDDSignal()
	{
		return WDD_SIGNAL;
	}
	bool WaggleDanceDetector::isWDDDance()
	{
		return WDD_DANCE;
	}
	std::size_t WaggleDanceDetector::getWDDSignalNumber()
	{
		return WDD_SIGNAL_NUMBER;
	}
	const std::map<std::size_t,cv::Point2d> * WaggleDanceDetector::getWDDSignalId2PointMap()
	{
		return &WDD_SIGNAL_ID2POINT_MAP;
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
		printf("[WDD_SIGNAL_DD_MIN_POTENTIAL] %.1f\n",WDD_SIGNAL_DD_MIN_POTENTIAL);
	}
	void WaggleDanceDetector::printPositionConfig()
	{
		printf("Printing position configuration:\n");
		printf("[dd_pos_id2point_map size] %d.\n", DD_POSITIONS_NUMBER);
	}

} /* namespace WaggleDanceDetector */
