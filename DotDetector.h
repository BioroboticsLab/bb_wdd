#pragma once

namespace wdd {
	struct SAMP {
		float c0,c1,c2,c3,c4,c5,c6;
		char padding0[4];
		float s0,s1,s2,s3,s4,s5,s6;
		char padding1[4];
	};

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
		// container to save COS/SIN values of corresponding _DD_PX_VALS_NOR
		// 2048 bytes
		SAMP _DD_PX_VALS_COSSIN[WDD_FBUFFER_SIZE];
		// END BLOCK4 (2591)

		// START BLOCK5 (2592 % 16 == 0)
		// container to save frequency scores
		// 92 bytes (28 bytes raw, 64 bytes overhead)
		std::array<float,WDD_FREQ_NUMBER> _DD_FREQ_SCORES; 
		// END BLOCK6 (2683)
		char padding5[4];
		// END BLOCK5 (2687)

		// START BLOCK6 (2688 % 16 == 0)
		// container to save accumulated COS/SIN
		// 64 bytes
		SAMP _ACC_VAL;
		// END BLOCK6 (2751)
		//2752 % 16 == 0
		//2752 % 32 == 0

		void _getInitialNewMinMax();
		void _getNewMinMax();
		void _execFullCalculation();
		void _execSingleCalculation();
		void _execDetection();
		//		float _normalizeValue(uchar u);
		void _normalizeValue(uchar u, float * f_ptr);
		void _nextSampPos();
	public:

#ifdef WDD_DDL_DEBUG_FULL
		arma::Row<float>::fixed<1> AUX_DD_POTENTIALS;
		arma::Row<float>::fixed<WDD_FREQ_NUMBER> AUX_DD_FREQ_SCORE;
		arma::Row<unsigned int>::fixed<WDD_FBUFFER_SIZE> AUX_DD_RAW_PX_VAL;
#endif
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyInitialPixel(bool doDetection);
		void copyPixelAndDetect();

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