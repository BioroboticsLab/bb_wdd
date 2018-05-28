#pragma once
#include <armadillo>
#include <opencv2/opencv.hpp>

namespace wdd {

class DotDetectorM {

public:
    /*
		The method adds a new frame by copying the image to the ring buffer at a
		specific position. The position will be updated and the memory will not
		be moved by this operation.

		@param: the new frame to be added
		*/
    static void addNewFrame(cv::Mat* _newFrame);
    static void detectDots();
    static void init(unsigned int _resX, unsigned int _resY, unsigned int _framesInBuffer);

private:
    static void executeDetection(cv::Mat const& _projectedSin, cv::Mat const& _projectedCos, uint16_t _id);

    /*
		Holds the video to be analysed as ring buffer. Any operations
		can then be efficiently done on the armadillo cube.
		*/
    static cv::Mat videoBuffer;

    /*
		The frame, whose turn is next in the buffer
		*/
    static unsigned int positionInBuffer;

    /*
		Are we still initializing the video buffer?
		No dot detection is performed then.
		*/
    static bool initializing;

    /*
		Resolution of our video buffer
		*/
    static unsigned int resolutionX, resolutionY, framesInBuffer;
};
}
