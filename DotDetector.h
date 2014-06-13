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

		// save new min or max flag
		bool _NEWMINMAX;

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

		void _copyPixel();
		void _execDetection();
		inline double _normalizeValue(uchar u);
		inline void _nextSampPos();

	public:
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyPixel();
		void copyPixelAndDetect();

		/*
		static 
		*/
		static std::size_t _BUFF_POS;
		static void nextBuffPos();
	};
} /* namespace WaggleDanceDetector */