/*
* WaggleDanceDetector.h
*
*  Created on: 08.04.2014
*      Author: sam
*/
#include "stdafx.h"

#ifndef WAGGLEDANCEDETECTOR_H_
#define WAGGLEDANCEDETECTOR_H_

namespace wdd
{
	struct DANCE{
		std::size_t DANCE_UNIQE_ID;

		unsigned long long DANCE_FRAME_START;
		unsigned long long DANCE_FRAME_END;

		std::vector<cv::Point2d> positions;
	};

	class WaggleDanceDetector
	{
		// defines detection frequency lower border
		double 	DD_FREQ_MIN;
		// defines detection frequency upper border
		double 	DD_FREQ_MAX;
		// defines step size to use between upper and lower border
		double 	DD_FREQ_STEP;

		// saves each target frequency, created with min,max,step
		std::vector<double> DD_FREQS;
		// saves each target frequency mean score
		// TODO: is it needed?
		double DD_FREQS_MSCORES[1];
		// saves number of DD_FREQS
		std::size_t DD_FREQS_NUMBER;

		// save sampled target sinus and cosinus signals as n-by-m matrix of n =
		// DD_FREQS_NUMBER with m = WDD_FBUFFER_SIZE sampled values
		arma::Mat<double> DD_FREQS_COSSAMPLES;
		arma::Mat<double> DD_FREQS_SINSAMPLES;

		// new defined positions of used DotDetectors: as map from dd_id -> Point2i
		std::map<std::size_t,cv::Point2i> DD_POS_ID2POINT_MAP;
		// saves number n of DotDetectors in DD_POS_ID2POINT_MAP
		std::size_t DD_POSITIONS_NUMBER;

		// saves (frequency) scores of DotDetectors as nxm matrix of n =
		// DD_POSITIONS_NUMBER, each with m = DD_FREQS_NUMBER values
		arma::Mat<double> DD_SIGNAL_SCORES;

		// saves potential of n = DD_POSITIONS_NUMBER DotDetectors (potentials are
		// matched against WDD_SIGNAL_DD_MIN_SCORE - their calculation is part of
		// magic)
		arma::rowvec 	DD_SIGNAL_POTENTIALS;

		// saves signal of n = DD_POSITIONS_NUMBER DotDetectors (a DotDetecotor has
		// a positive signal if DD_SIGNAL_POTENTIALS[i] > WDD_SIGNAL_DD_MIN_SCORE)
		arma::rowvec 	DD_SIGNAL_BOOL;

		// defines frame sample rate per second of used input
		double 	FRAME_RATE;
		// defines frame reduction factor used
		double 	FRAME_REDFAC;
		// defines frame height
		int 	FRAME_HEIGHT;
		// defines frame width
		int 	FRAME_WIDTH;

		// saves frame buffer as array of ptr to
		// n = WDD_FBUFFER_SIZE 2d arrays (TODO: frame) with DIM: HEIGHT x WIDTH
		std::deque<cv::Mat> WDD_FBUFFER;
		// defines frame buffer size
		std::size_t WDD_FBUFFER_SIZE;
		// saves last frame buffer writing position
		std::size_t WDD_FBUFFER_POS;

		// saves total detection signal, either 0 or 1
		bool WDD_SIGNAL;
		// saves number of detection signals
		std::size_t WDD_SIGNAL_NUMBER;
		// saves positions of detection signals as map from dd_id -> Point2d
		std::map<std::size_t,cv::Point2d> WDD_SIGNAL_ID2POINT_MAP;
		// saves unique #frame
		unsigned long long WDD_SIGNAL_FRAME_NR;

		// defines the maximum distance between two neighbor DD signals
		// TODO: calculate from bee size & DPI
		double WDD_SIGNAL_DD_MAXDISTANCE;
		// minimum number of positive DotDetector to start detection and
		// minimum size of clusters for positive detection and
		std::size_t WDD_SIGNAL_DD_MIN_CLUSTER_SIZE;
		// defines the absolute score value a DD needs for signal
		double WDD_SIGNAL_DD_MIN_POTENTIAL;

		// if set to true, appends wdd signal to output file
		bool WDD_WRITE_SIGNAL_FILE;
		// defines if verbose execution mode
		//TODO: check for remove as not used anymore
		bool 	WDD_VERBOSE;

		/* 
			post signal processing 
		*/
		// TODO: move post processing (dance image extraction & orientation
		// determination) in own class
		// indicates that a dance is completed
		bool WDD_DANCE;
		// defines the maximum distance between two positions of same signal
		// TODO: calculate from bee size & DPI
		double 	WDD_DANCE_MAX_POS_DIST;
		// defines maximum gap of frames for a signal to connect
		int 	WDD_DANCE_MAX_FRAME_GAP;
		// defines minimum number of consecutive frames to form a final WD signal
		int 	WDD_DANCE_MIN_CONSFRAMES;
		// unique dance id counter
		std::size_t WDD_DANCE_ID;

		// maps to show top level WDD signals (=dances)
		std::vector<DANCE> WDD_UNIQ_DANCES;
		// map to save ALL dances 
		std::vector<DANCE> WDD_UNIQ_FINISH_DANCES;
		// pointer to videoFrameBuffer (accessing history frames)
		VideoFrameBuffer * WDD_VideoFrameBuffer_ptr;

	public:
		WaggleDanceDetector(
			std::vector<double> dd_freq_config,
			std::map<std::size_t,cv::Point2i> dd_pos_id2point_map,
			std::vector<double> frame_config,
			int wdd_fbuffer_size,
			std::vector<double> wdd_signal_dd_config,
			bool wdd_write_signal_file,
			bool wdd_verbose,
			VideoFrameBuffer * videoFrameBuffer_ptr
			);
		~WaggleDanceDetector();
		void addFrame(cv::Mat f, unsigned long long fid, bool imidiateDetect);
		void execDetection();
		void printFBufferConfig();
		void printFreqConfig();
		void printFreqSamples();
		void printFrameConfig();
		void printSignalConfig();
		void printPositionConfig();
		bool isWDDSignal();
		bool isWDDDance();
		std::size_t getWDDSignalNumber();
		const std::map<std::size_t,cv::Point2d> * getWDDSignalId2PointMap();
	private:
		void createFreqSamples();
		void execDetectionGetDDPotentials();
		void execDetectionGetWDDSignals();
		void execDetectionConcatWDDSignals();
		void execDetectionHousekeepWDDSignals();
		void execDetectionFinalizeDance(DANCE d);
		void execDetectionWriteDanceFileLine(DANCE d);
		void execDetectionWriteSignalFileLine();
		void initDDSignalValues();
		void initWDDSignalValues();
		void initWDDDanceValues();
		arma::Col<arma::uword> getNeighbours(arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs);
		void setFBufferConfig(int wdd_fbuffer_size);
		void setFrameConfig(std::vector<double> dd_frame_config);
		void setFreqConfig(std::vector<double> dd_freq_config);
		void setSignalConfig(std::vector<double> wdd_signal_dd_config);
		void setPositions(std::map<std::size_t,cv::Point2i> dd_pos_id2point_map);


	};

} /* namespace WaggleDanceDetector */

#endif /* WAGGLEDANCEDETECTOR_H_ */
