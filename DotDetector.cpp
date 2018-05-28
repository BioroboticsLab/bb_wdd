#include "DotDetector.h"
#include "Config.h"
#include "DotDetectorLayer.h"
#include "opencv2/opencv.hpp"
//#include "WaggleDanceDetector.h"
#include "mutex"

namespace wdd {

std::size_t DotDetector::_BUFF_POS = 0;
static std::mutex vector_acces;

DotDetector::DotDetector(std::size_t _id, uchar* pixel_src_ptr)
    : id(_id)
    , aux_pixel_ptr(pixel_src_ptr)
    , _OLDMINGONE(false)
    , _OLDMAXGONE(false)
    , _NEWMINHERE(false)
    , _NEWMAXHERE(false)
    , _NEWMINMAX(false)
    , _MAX(255)
    , _MIN(0)
    , _AMPLITUDE(0)
    , _sampPos(0)
{
    rawPixels.fill(0);
    pixelHistogram.fill(0);
    scoresForFreq.fill(0);
}

DotDetector::~DotDetector(void)
{
}

// add pixel values until buffer is full (initial filling phase)
void DotDetector::copyInitialPixel(bool doDetection)
{
    // while initializing no deletetion (overwriting) occurs
    // expect _BUFF_POS have increasing values of [0;WDD_FBUFFER_SIZE-1]

    // save raw pixel value
    rawPixels[_BUFF_POS] = *aux_pixel_ptr;

    // check if it is last call to _copyInitialPixel, all rawPixels[] set
    if (doDetection) {
        _getInitialNewMinMax();

        if (_AMPLITUDE == 0) {
            DotDetectorLayer::DD_POTENTIALS[id] = 0;
            return;
        }

        _execFullCalculation();
        _execDetection();
    }
}

void DotDetector::copyPixelAndDetect()
{
    // handle oldest raw pixel (which will be overwritten by newest raw pixel)

    /*
		BY ROMAN:
		This part deals with the histogram when a new pixel is added to the
		ring buffer.
		It determines whether the min and max values are still valid and
		calculates the new ones, if neccessary.

		-> Amplitude refers to the width of the histogram, that is the span of
		   different brightness values
		*/
    // decrease count of old raw pixel value, and if it is removed..
    if ((--pixelHistogram[rawPixels[_BUFF_POS]]) == 0) {
        //..check if a former min/max is gone
        if (rawPixels[_BUFF_POS] == _MIN) {
            _OLDMINGONE = true;
            // set new min
            while (pixelHistogram[_MIN] == 0)
                _MIN++;
        } else if (rawPixels[_BUFF_POS] == _MAX) {
            _OLDMAXGONE = true;
            // set new max
            while (pixelHistogram[_MAX] == 0)
                _MAX--;
        }
    }

    // save new raw pixel value
    rawPixels[_BUFF_POS] = *aux_pixel_ptr;

    // increase count of new raw pixel value, and if it is added..
    if ((++pixelHistogram[rawPixels[_BUFF_POS]]) == 1) {
        //..check if former min/max is valid
        if (rawPixels[_BUFF_POS] < _MIN) {
            _NEWMINHERE = true;
            // set new min
            _MIN = rawPixels[_BUFF_POS];
        } else if (rawPixels[_BUFF_POS] > _MAX) {
            _NEWMAXHERE = true;
            // set new max
            _MAX = rawPixels[_BUFF_POS];
        }
    }

    if (_OLDMINGONE || _OLDMAXGONE || _NEWMINHERE || _NEWMAXHERE) {
        _AMPLITUDE = _MAX - _MIN;
        amplitude_inv = 1.0f / _AMPLITUDE;
        // set flags
        _OLDMINGONE = _OLDMAXGONE = _NEWMINHERE = _NEWMAXHERE = false;
        _NEWMINMAX = true;
    }

    // if amplitude is zero, no further detection needed
    if (_AMPLITUDE < 13) {
        DotDetectorLayer::DD_POTENTIALS[id] = 0;
        _NEWMINMAX = true;
        return;
    }

    // when new min/max encountered,..
    if (_NEWMINMAX) {
        _execFullCalculation();
        _NEWMINMAX = false;
    }
    // else only one new value needs to be transformed
    else {
        _execSingleCalculation();
    }

    // leave DotDetector in a state ready to calculcate FREQ_SCORES
    _execDetection();
}

/*
	private functions
	*/
inline void DotDetector::_getInitialNewMinMax()
{
    // get initial count of raw pixel values
    for (std::size_t i = 0; i < WDD_FBUFFER_SIZE; i++)
        pixelHistogram[rawPixels[i]]++;

    // move maximum border towards left
    while (pixelHistogram[_MAX] == 0)
        _MAX--;

    // move minimum border towards left
    while (pixelHistogram[_MIN] == 0)
        _MIN++;

    // finally get amplitude and save inversed one
    _AMPLITUDE = _MAX - _MIN;
    amplitude_inv = 1.0f / _AMPLITUDE;
}
inline void DotDetector::_execFullCalculation()
{
    // reset accumulators to zero
    memset(&_ACC_VAL, 0, sizeof(_ACC_VAL));

    // recalculate cos/sin values and set accumulators
    // get correct starting position in ring buffer
    std::size_t startPos = (_BUFF_POS + 1) % WDD_FBUFFER_SIZE;
    std::size_t currtPos;

    float px_val;
    for (std::size_t j = 0; j < WDD_FBUFFER_SIZE; j++) {
        // init loop vars
        currtPos = (j + startPos) % WDD_FBUFFER_SIZE;

        //_normalizeValue(rawPixels[currtPos], &projectedOnCosSin[currtPos].n);
        projectedOnCosSin[currtPos].n = normalizeValue(rawPixels[currtPos]);
        px_val = projectedOnCosSin[currtPos].n;

        for (unsigned int i(0); i < WDD_FREQ_NUMBER; ++i) {
            float cos = DotDetectorLayer::SAMPLES[j].cosines[i] * px_val;
            float sin = DotDetectorLayer::SAMPLES[j].sines[i] * px_val;
            projectedOnCosSin[currtPos].cosines[i] = cos;
            projectedOnCosSin[currtPos].sines[i] = sin;
            _ACC_VAL.cosines[i] += cos;
            _ACC_VAL.sines[i] += sin;
        }
    }
    _sampPos = WDD_FBUFFER_SIZE;
}

inline void DotDetector::_execSingleCalculation()
{
    //_normalizeValue(rawPixels[_BUFF_POS], &projectedOnCosSin[_BUFF_POS].n);
    projectedOnCosSin[_BUFF_POS].n = normalizeValue(rawPixels[_BUFF_POS]);

    float px_val = projectedOnCosSin[_BUFF_POS].n;
    for (unsigned int i(0); i < WDD_FREQ_NUMBER; ++i) {
        float cos = DotDetectorLayer::SAMPLES[_sampPos].cosines[i] * px_val;
        float sin = DotDetectorLayer::SAMPLES[_sampPos].sines[i] * px_val;
        _ACC_VAL.cosines[i] -= projectedOnCosSin[_BUFF_POS].cosines[i];
        _ACC_VAL.cosines[i] += cos;
        projectedOnCosSin[_BUFF_POS].cosines[i] = cos;
        _ACC_VAL.sines[i] -= projectedOnCosSin[_BUFF_POS].sines[i];
        _ACC_VAL.sines[i] += sin;
        projectedOnCosSin[_BUFF_POS].sines[i] = sin;
    }
    _nextSampPos();
}

inline void DotDetector::_execDetection()
{
    //Normalize Frequency Scores to Amount of Frames Analysed
    /*
		_ACC_VAL.c0 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c1 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c2 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c3 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c4 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c5 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c6 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s0 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s1 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s2 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s3 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s4 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s5 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s6 /= (double)WDD_FBUFFER_SIZE;
		*/
    // score_i = sinSum_i^2 + cosSum_i^2
    for (unsigned int i(0); i < WDD_FREQ_NUMBER; ++i) {
        scoresForFreq[i] = _ACC_VAL.cosines[i] * _ACC_VAL.cosines[i] + _ACC_VAL.sines[i] * _ACC_VAL.sines[i];
    }

    float buff;
    buff = scoresForFreq[0] + scoresForFreq[1] + scoresForFreq[2];
    for (int i = 3; i < WDD_FREQ_NUMBER; i++) {
        if (buff > 0.3 * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
            vector_acces.lock();
            DotDetectorLayer::DD_SIGNALS_NUMBER++;
            DotDetectorLayer::DD_SIGNALS_IDs.push_back(id);
            vector_acces.unlock();
            return;
        }
        buff += -scoresForFreq[i - 3] + scoresForFreq[i];
    }
    if (buff > 0.3 * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
        vector_acces.lock();
        DotDetectorLayer::DD_SIGNALS_NUMBER++;
        DotDetectorLayer::DD_SIGNALS_IDs.push_back(id);
        vector_acces.unlock();
        return;
    }
}

// alternative definition of _normalizeValue
float DotDetector::normalizeValue(const uchar v)
{
    float flt_v = static_cast<float>(v);
    return (2 * amplitude_inv * (flt_v - _MIN)) - 1;
}

// original function
inline void DotDetector::_normalizeValue(uchar u, float* f_ptr)
{
    *f_ptr = static_cast<float>(u);
    *f_ptr -= _MIN; //0.. MAX
    *f_ptr *= amplitude_inv; //0..1
    *f_ptr *= 2; //0..2
    *f_ptr -= 1; //-1 .. 1
}

inline void DotDetector::_nextSampPos()
{
    // values between [0;frame_rate-1]
    //_sampPos++;
    //_sampPos %= DotDetectorLayer::FRAME_RATEi;
    _sampPos = (++_sampPos < WDD_FRAME_RATE ? _sampPos : 0);
}

void DotDetector::nextBuffPos()
{
    DotDetector::_BUFF_POS++;
    DotDetector::_BUFF_POS %= WDD_FBUFFER_SIZE;
    //DotDetector::_BUFF_POS = ( ++DotDetector::_BUFF_POS < WDD_FBUFFER_SIZE ? DotDetector::_BUFF_POS : 0);
}
} /* namespace WaggleDanceDetector */
