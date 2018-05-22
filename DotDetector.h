#pragma once

#include <memory>

#include "Config.h"

namespace wdd {
	struct SAMP {
		float cosines[WDD_FREQ_NUMBER];
		float sines[WDD_FREQ_NUMBER];
		float n;
		char padding1[4];
	};

	class DotDetector
	{
	private:
		// START BLOCK0 (0)
		// container to save raw uint8 frame pixel values of a single position		
		// 32 byte
		std::array<uchar,WDD_FBUFFER_SIZE> rawPixels;
		// END BLOCK0 (31)

		// START BLOCK1 (32 % 16 == 0)
		// save extern pointer to DotDetectorLayer cv::Mat frame
		// (is expected to be at same location over runtime)
		// 4 byte
		uchar * aux_pixel_ptr;
		//
		// save position of next sample cos/sin value (=[0;frame_rate-1])
		// 4 byte
		std::size_t _sampPos;
		//
		// save own ID
		// 4 byte
		std::size_t id;
		//
		// save inverse amplitude for faster normalization
		// 4 byte
		float amplitude_inv;
		//
		// save min, max, amplitude values
		// 3 byte
		/*
		IMPORTANT NOTE BY ROMAN:
		Min and max refer to the histogram and specify the lowest
		(and highest respectively) brightness we have in our ring buffer.
		*/
		uchar _MIN,_MAX, _AMPLITUDE;
		//
		// save new min or max flag, old min/max gone flag
		// 5 byte
		bool _NEWMINMAX, _OLDMINGONE, _OLDMAXGONE, _NEWMINHERE, _NEWMAXHERE;
		// END BLOCK1 (24 byte)
		char padding1[8];
		// END BLOCK1 (63)

		// START BLOCK2 (64 % 16 == 0)
		// container to save accumulated COS/SIN per freq values
		// 64 byte
		SAMP _ACC_VAL;
		// END BLOCK2 (127)

		// START BLOCK3 (128 % 16 == 0)
		// save number of each possible pixel value currently in rawPixels
		// 256 byte
		std::array<uchar,256> pixelHistogram;
		// END BLOCK3 (319)

												// START BLOCK3 (320 % 16 == 0)
												// container to save [-1:1] scaled values
												// 216 bytes (128 bytes raw, 88 bytes overhead)
												//arma::Row<float>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_NOR;
												// END BLOCK3 (535)
												//char padding2[8];
												// END BLOCK3 (543)

		// START BLOCK4 (384 % 16 == 0)
		// container to save COS/SIN/NOR values of corresponding _DD_PX_VALS_NOR
		// 2048 byte
		SAMP projectedOnCosSin[WDD_FBUFFER_SIZE];
		// END BLOCK4 (2367)

		// START BLOCK5 (2432 % 16 == 0)
		// container to save frequency scores
		// 28 bytes
		std::array<float,WDD_FREQ_NUMBER> scoresForFreq; 
		// END BLOCK5 (2459)
		char padding5[36];
		// END BLOCK5 (2495)

		// TOTAL 39 cl a 64 byte = 2496  byte

		void _getInitialNewMinMax();
		void _getNewMinMax();
		void _execFullCalculation();
		void _execSingleCalculation();
		void _execDetection();
		void _normalizeValue(uchar u, float * f_ptr);
		float normalizeValue(const uchar u);
		void _nextSampPos();

	public:
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyInitialPixel(bool doDetection);
		void copyPixelAndDetect();

		// save global position of next buffer write (=[0;WDD_FBUFFER_SIZE-1])
		static std::size_t _BUFF_POS;

		static void nextBuffPos();
	};
} /* namespace WaggleDanceDetector */
