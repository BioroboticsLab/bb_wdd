#pragma once

namespace wdd {
	class DotDetector
	{
	private:
		// save own ID
		std::size_t _UNIQUE_ID;

		// save extern pointer to DotDetectorLayer cv::Mat frame
		// (is expected to be at same location over runtime)
		uchar * _pixel_src_ptr;

		// save min, max, amplitude values
		uchar _MIN,_MAX, _AMPLITUDE;

		// save number of each possible pixel value currently in _DD_PX_VALS_RAW
		std::array<uchar,256> _UINT8_PX_VALS_COUNT;

		// save new min or max flag, old min/max gone flag
		bool _NEWMINMAX, _OLDMINGONE, _OLDMAXGONE, _NEWMINHERE, _NEWMAXHERE;

		// save position of next sample cos/sin value (=[0;frame_rate-1])
		std::size_t _sampPos;

		// container to save raw uint8 frame values of a single position
		std::array<uchar, WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;

		// container to save [-1:1] scaled values
		std::array<double, WDD_FBUFFER_SIZE> _DD_PX_VALS_NOR;	

		// container to save DD_FREQ_NUMBER x WDD_FBUFFER_SIZE sin/cos values
		double ** _DD_PX_VALS_SIN;
		double ** _DD_PX_VALS_COS;

		// container to save frequency scores, accumulated _DD_PX_VALS_SIN /COS
		//TODO: LINK SIZE TO MAIN.CPP
		std::array<double, 7> _DD_FREQ_SCORES, _ACC_SIN_VAL, _ACC_COS_VAL;

		void _copyInitialPixel(bool doDetection);
		void _copyPixel();		
		void _getInitialNewMinMax();
		void _getNewMinMax();
		void _execFullCalculation();
		void _execSingleCalculation();
		void _execDetection();
		double _normalizeValue(uchar u);
		void _nextSampPos();

	public:
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyPixel(bool doDetection);
		void copyPixelAndDetect();

		//arma::rowvec7 AUX_DD_FREQ_SCORES;
		//arma::Row<arma::uword> AUX_DEB_DD_RAW_BUFFERS;
		/*
		static 
		*/
		// save global position of next buffer write (=[0;WDD_FBUFFER_SIZE-1])
		static std::size_t _BUFF_POS;
		static void nextBuffPos();
	};
} /* namespace WaggleDanceDetector */