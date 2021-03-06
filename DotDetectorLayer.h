#pragma once
#include "DotDetector.h"
#include "DotDetectorMatrix.h"

namespace wdd {

class DotDetectorLayer {
public:
    // save sampled target sinus and cosinus signals as array of SAMP
    static Sample SAMPLES[WDD_FRAME_RATE];

    // saves positions of used DotDetectors
    static std::vector<cv::Point2i> positions;
    // saves number n of DotDetectors in positions
    static std::size_t DD_NUMBER;
    // defines the absolute score value a DD needs for signal
    static double DD_MIN_POTENTIAL;

    // saves potential of n = positions_NUMBER DotDetectors (potentials are
    // matched against WDD_SIGNAL_DD_MIN_SCORE - their calculation is part of
    // magic)
    static double* DD_POTENTIALS;
    // saves number of positive DDs
    static std::size_t DD_SIGNALS_NUMBER;
    // saves ids of positive DDs (interface to WDD Layer 2)
    //(a DotDetecotor has a positive signal if DD_SIGNAL_POTENTIALS[i] > WDD_SIGNAL_DD_MIN_SCORE)
    static std::vector<unsigned long long> DD_SIGNALS_IDs;

    // defines detection frequency lower border
    static double DD_FREQ_MIN;
    // defines detection frequency upper border
    static double DD_FREQ_MAX;
    // defines step size to use between upper and lower border
    static double DD_FREQ_STEP;
    // saves each target frequency, created with min,max,step
    static std::vector<double> DD_FREQS;
    // saves number of DD_FREQS
    static std::size_t DD_FREQS_NUMBER;

    // saves frame rate
    static std::size_t FRAME_RATEi;
    // saves frame reduction factor
    static double FRAME_REDFAC;

    static cv::Mat* frame_ptr;

    static void init(std::vector<cv::Point2i> positions, cv::Mat* aux_frame_ptr,
        std::vector<double> ddl_config);

    static void release();

    static void copyFrame(bool doDetection);
    static void copyFrameAndDetect();

    static void printFreqConfig();
    static void printFreqSamples();

private:
    static DotDetectorM _dotDetector;

    static void _setDDLConfig(std::vector<double> ddl_config);
    static void _createFreqSamples();
};

} /* namespace WaggleDanceDetector */
