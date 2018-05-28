#pragma once
#include "VideoFrameBuffer.h"

#include <armadillo>

namespace wdd {
struct Dance {
    unsigned long long danceFrameStart;
    unsigned long long danceFrameEnd;

    std::size_t danceUniqueID;

    std::vector<cv::Point2d> positions;
    cv::Point2d positionLast;

    cv::Point2d orientUVec;
    cv::Point2d naiveOrientation;

    timeval rawTime;
};

class WaggleDanceDetector {
private:
    std::size_t _wddFbufferPos;

    // Layer 2
    // saves total detection signal, either 0 or 1
    bool _wddSignal;

    // saves number of detection signals
    std::size_t _wddSignalNumbers;

    // saves positions of detection signals as map from dd_id -> Point2d
    std::map<std::size_t, cv::Point2d> _wddSignalID2PointMap;

    // defines the maximum distance between two neighbor DD signals
    // TODO: calculate from bee size & DPI
    double _wddSignalDDMaxDistance;

    // minimum number of positive DotDetector to start detection and
    // minimum size of clusters for positive detection and
    std::size_t _wddSignalDDMinClusterSize;

    // Layer 3
    // TODO: move post processing (dance image extraction & orientation
    // determination) in own class
    // indicates that a dance is completed
    bool _wddDance;

    // unique dance id counter
    std::size_t _wddDanceID;

    // sequencial dance number counter
    std::size_t _wddDanceNumber;

    // maps to show top level WDD signals (=dances)
    std::vector<Dance> _wddUniqueDances;

    // map to save ALL dances
    std::vector<Dance> _wddUniqFinishDances;

    // defines the maximum distance between two positions of same signal
    // TODO: calculate from bee size & DPI
    double _wddDanceMaxPosDist;

    // defines maximum gap of frames for a signal to connect
    std::size_t _wddDanceMaxFrameGap;

    // defines minimum number of consecutive frames to form a final WD signal
    std::size_t _wddDanceMinConsFrames;

    // aux pointer
    // pointer to videoFrameBuffer (accessing history frames)
    VideoFrameBuffer* _wddVideoFrameBufferPtr;

    // develop options
    // if set to true, appends wdd dance to output file
    bool _wddWriteDanceFile;
    // if set to true, appends wdd signal to output file
    bool _wddWriteSignalFile;

    int _startFrameShift;
    int _endFrameShift;

    CamConf auxCC;

    const std::string _dancePath;

    FILE* _danceFilePtr = nullptr;
    FILE* _signalFilePtr = nullptr;

public:
    WaggleDanceDetector(
        std::vector<cv::Point2i> dd_positions,
        cv::Mat* frame_ptr,
        std::vector<double> ddl_config,
        std::vector<double> wdd_dance_config,
        VideoFrameBuffer* videoFrameBuffer_ptr,
        CamConf auxCC,
        bool WDD_WRITE_SIGNAL_FILE,
        bool WDD_WRITE_DANCE_FILE,
        int WDD_VERBOSE,
        std::string const& dancePath);
    ~WaggleDanceDetector();

    void copyInitialFrame(unsigned long long fid, bool doDetection);
    void copyFrame(unsigned long long fid);

    bool isWDDSignal();
    bool isWDDDance();
    std::size_t getWDDSignalNumber();
    const std::map<std::size_t, cv::Point2d>* getWDDSignalId2PointMap();
    const std::vector<Dance>* getWDDFinishedDancesVec();
    void printWDDDanceConfig();
    std::size_t getWDD_SIGNAL_DD_MIN_CLUSTER_SIZE();
    void setWDD_SIGNAL_DD_MIN_CLUSTER_SIZE(std::size_t val);
    // defines if verbose execution mode
    //TODO: check for remove as not used anymore
    static int WDD_VERBOSE;
    // saves unique #frame
    static unsigned long long WDD_SIGNAL_FRAME_NR;

private:
    void _initWDDSignalValues();
    void _initWDDDanceValues();
    void _initOutPutFiles();

    void _setWDDConfig(std::vector<double> wdd_config);

    void _execDetection();
    void _execDetectionGetDDPotentials();
    void _execDetectionGetWDDSignals();
    void _execDetectionConcatWDDSignals();
    void _execDetectionHousekeepWDDSignals();
    void _execDetectionFinalizeDance(Dance* dance);
    void _execDetectionWriteDanceFileLine(Dance* d_ptr);
    void _execDetectionWriteSignalFileLine();

    arma::Col<arma::uword> getNeighbours(arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs);
};

} /* namespace WaggleDanceDetector */
