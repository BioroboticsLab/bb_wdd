#pragma once

namespace wdd {
	class DotDetector
	{
	private:
		// START BLOCK0 (0)
		// container to save raw uint8 frame values of a single position
		// 72 bytes (32 bytes raw, 40 bytes overhead)
		//arma::Row<uchar>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;
		// 32 bytes
		std::array<uchar,WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;
		// END BLOCK0 (31)

		// START BLOCK1 (32 % 16 == 0)
		// save extern pointer to DotDetectorLayer cv::Mat frame
		// (is expected to be at same location over runtime)
		// 4 bytes
		uchar * aux_pixel_ptr;
		//
		// save position of next sample cos/sin value (=[0;frame_rate-1])
		// 4 bytes
		std::size_t _sampPos;
		//
		// save own ID
		// 4 bytes
		std::size_t _UNIQUE_ID;
		//
		// save inverse amplitude for faster normalization
		// 4 bytes
		float _AMPLITUDE_INV;
		//
		// save min, max, amplitude values
		// 3 byte
		uchar _MIN,_MAX, _AMPLITUDE;
		//
		// save new min or max flag, old min/max gone flag
		// 5 byte
		bool _NEWMINMAX, _OLDMINGONE, _OLDMAXGONE, _NEWMINHERE, _NEWMAXHERE;
		// END BLOCK1 (24 bytes)
		char padding1[8];
		// END BLOCK1 (32 bytes) (63)

		// START BLOCK2 (64 % 16 == 0)
		// save number of each possible pixel value currently in _DD_PX_VALS_RAW
		// 256 bytes
		std::array<uchar,256> _UINT8_PX_VALS_COUNT;
		// END BLOCK2 (319)

		// START BLOCK3 (320 % 16 == 0)
		// container to save [-1:1] scaled values
		// 216 bytes (128 bytes raw, 88 bytes overhead)
		arma::Row<float>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_NOR;
		// END BLOCK3 (535)
		char padding2[8];
		// END BLOCK3 (543)

		// START BLOCK4 (544 % 16 == 0)
		// container to save DD_FREQ_NUMBER x WDD_FBUFFER_SIZE cos values
		// 984 bytes (896 bytes raw, 88 bytes overhead)
		arma::Mat<float>::fixed<WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER> _DD_PX_VALS_COS;
		// END BLOCK4 (1527)
		char padding3[8];
		// END BLOCK4 (1535)

		// START BLOCK5 (1536 % 16 == 0)
		// container to save DD_FREQ_NUMBER x WDD_FBUFFER_SIZE sin values
		// 984 bytes (896 bytes raw, 88 bytes overhead)
		arma::Mat<float>::fixed<WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER> _DD_PX_VALS_SIN;
		// END BLOCK4 (2519)
		char padding4[8];
		// END BLOCK5 (2527)

		// START BLOCK6 (2528 % 16 == 0)
		// container to save frequency scores
		// 92 bytes (28 bytes raw, 64 bytes overhead)
		arma::Row<float>::fixed<WDD_FREQ_NUMBER> _DD_FREQ_SCORES; 
		// END BLOCK6 (2619)
		char padding5[4];
		// END BLOCK6 (2623)

		// START BLOCK7 (2624 % 16 == 0)
		// container to save accumulated COS and scores
		// 92 bytes (28 bytes raw, 64 bytes overhead)
		arma::Row<float>::fixed<WDD_FREQ_NUMBER> _ACC_COS_VAL;
		// END BLOCK7 (2715)
		char padding6[4];
		// END BLOCK7 (2719)

		// START BLOCK8 (2720 % 16 == 0)
		// container to save accumulated SIN and scores
		// 92 bytes (28 bytes raw, 64 bytes overhead)
		arma::Row<float>::fixed<WDD_FREQ_NUMBER>_ACC_SIN_VAL;
		// END BLOCK8 (2811)
		char padding7[4];
		// END BLOCK8 (2815)

		void _getInitialNewMinMax();
		void _getNewMinMax();
		void _execFullCalculation();
		void _execSingleCalculation();
		void _execDetection();
//		float _normalizeValue(uchar u);
		void _normalizeValue(uchar u, float * f_ptr);
		void _nextSampPos();
	public:
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyInitialPixel(bool doDetection);
		void copyPixelAndDetect();

		//arma::rowvec7 AUX_DD_FREQ_SCORES;
		//arma::Row<arma::uword> AUX_DEB_DD_RAW_BUFFERS;
		/*
		static 
		*/
		// save global position of next buffer write (=[0;WDD_FBUFFER_SIZE-1])
		static std::size_t _BUFF_POS;

		//DEBUG 
		static std::size_t _NRCALL_EXECFULL;
		static std::size_t _NRCALL_EXECSING;
		static std::size_t _NRCALL_EXECSLEP;
		static std::vector<uchar> _AMP_SAMPLES;
		static void nextBuffPos();
	};
} /* namespace WaggleDanceDetector */