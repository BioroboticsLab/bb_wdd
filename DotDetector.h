#pragma once

namespace wdd {
	class DotDetector
	{
	private:
		// save extern pointer to DotDetectorLayer cv::Mat frame
		// (is expected to be at same location over runtime)
		// 4 bytes
		uchar * aux_pixel_ptr;

		// save position of next sample cos/sin value (=[0;frame_rate-1])
		// 4 byte
		std::size_t _sampPos;
		
		// save inverse amplitude for faster normalization
		float _AMPLITUDE_INV;

		// save min, max, amplitude values
		// 1 byte
		uchar _MIN,_MAX, _AMPLITUDE;

		// save new min or max flag, old min/max gone flag
		// 1 byte
		bool _NEWMINMAX, _OLDMINGONE, _OLDMAXGONE, _NEWMINHERE, _NEWMAXHERE;

		// container to save raw uint8 frame values of a single position
		// 72 bytes (32 bytes raw, 40 bytes overhead)
		arma::Row<uchar>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;
		//std::array<uchar,WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;

		// save number of each possible pixel value currently in _DD_PX_VALS_RAW
		// 256 bytes
		std::array<uchar,256> _UINT8_PX_VALS_COUNT;


		// container to save [-1:1] scaled values
		// 216 bytes (128 bytes raw, 88 bytes overhead)
		arma::Row<float>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_NOR;

		// container to save DD_FREQ_NUMBER x WDD_FBUFFER_SIZE sin/cos values
		// 984 bytes (896 bytes raw, 88 bytes overhead)
		arma::Mat<float>::fixed<WDD_FREQ_NUMBER,WDD_FBUFFER_SIZE> _DD_PX_VALS_SIN, _DD_PX_VALS_COS;

		// container to save frequency scores, accumulated _DD_PX_VALS_SIN /COS and scores
		// 92 bytes (28 bytes raw, 64 bytes overhead)
		arma::Row<float>::fixed<WDD_FREQ_NUMBER> _DD_FREQ_SCORES, _ACC_SIN_VAL, _ACC_COS_VAL;

		// save own ID
		// 4 bytes
		std::size_t _UNIQUE_ID;



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