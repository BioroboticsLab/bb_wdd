#include "WaggleDanceDetector.h"
#include "DotDetectorLayer.h"
#include "Util.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceExport.h"
#include "WaggleDanceOrientator.h"

#include <boost/filesystem.hpp>

namespace wdd {
unsigned long long WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
int WaggleDanceDetector::WDD_VERBOSE;

WaggleDanceDetector::WaggleDanceDetector(std::vector<cv::Point2i> dd_positions,
    cv::Mat* frame_ptr,
    std::vector<double> ddl_config,
    std::vector<double> wdd_config,
    VideoFrameBuffer* videoFrameBuffer_ptr,
    CamConf auxCC,
    bool wdd_write_signal_file,
    bool wdd_write_dance_file,
    int wdd_verbose, const std::string& dancePath)
    : _wddVideoFrameBufferPtr(videoFrameBuffer_ptr)
    , auxCC(auxCC)
    , _wddWriteDanceFile(wdd_write_dance_file)
    , _wddWriteSignalFile(wdd_write_signal_file)
    , _startFrameShift(WDD_FBUFFER_SIZE)
    , _endFrameShift(0)
    , _dancePath(dancePath)
{
    WaggleDanceDetector::WDD_VERBOSE = wdd_verbose;

    WaggleDanceExport::setArena(auxCC.arena);

    _initWDDSignalValues();

    _initWDDDanceValues();

    _initOutPutFiles();

    _setWDDConfig(wdd_config);

    DotDetectorLayer::init(dd_positions, frame_ptr, ddl_config);

    _wddFbufferPos = 0;
}

WaggleDanceDetector::~WaggleDanceDetector()
{
    if (_wddWriteDanceFile)
        fclose(_danceFilePtr);

    if (_wddWriteSignalFile)
        fclose(_signalFilePtr);
}
void WaggleDanceDetector::_initWDDSignalValues()
{
    _wddSignal = false;
    _wddSignalNumbers = 0;
}
void WaggleDanceDetector::_initWDDDanceValues()
{
    _wddDance = false;
    _wddDanceID = 0;
    _wddDanceNumber = 0;
}
void WaggleDanceDetector::_initOutPutFiles()
{
    if (_wddWriteDanceFile || _wddWriteSignalFile) {
        if (_wddWriteDanceFile) {
            boost::filesystem::path path(_dancePath);
            path /= DANCE_FILE_NAME;
            _danceFilePtr = fopen(path.c_str(), "a+");
        }
        if (_wddWriteSignalFile) {
            boost::filesystem::path path(_dancePath);
            path /= SIGNAL_FILE_NAME;
            _signalFilePtr = fopen(path.c_str(), "a+");
        }
    }
}
void WaggleDanceDetector::_setWDDConfig(std::vector<double> wdd_config)
{
    if (wdd_config.size() != 5) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wrong number of config arguments!" << std::endl;
        exit(-19);
    }

    /* copy values from ddl_config*/
    // Layer 2
    double wdd_signal_dd_maxdistance = wdd_config[0];
    double wdd_signal_min_cluster_size = wdd_config[1];
    // Layer 3
    double wdd_dance_max_position_distance = wdd_config[2];
    double wdd_dance_max_frame_gap = wdd_config[3];
    double wdd_dance_min_consframes = wdd_config[4];

    // do some sanity checks
    if (wdd_signal_dd_maxdistance < 0) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wdd_signal_max_cluster_dd_distance < 0!" << std::endl;
        exit(-20);
    }
    if (wdd_signal_min_cluster_size < 0) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wdd_signal_min_cluster_size < 0!" << std::endl;
        exit(-20);
    }
    if (wdd_dance_max_position_distance < 0) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_max_position_distance < 0!" << std::endl;
        exit(-21);
    }
    if (wdd_dance_max_frame_gap < 0) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_max_frame_gap < 0!" << std::endl;
        exit(-22);
    }
    if (wdd_dance_min_consframes < 0) {
        std::cerr << "Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_min_consframes < 0!" << std::endl;
        exit(-23);
    }

    _wddSignalDDMaxDistance = wdd_signal_dd_maxdistance;
    _wddSignalDDMinClusterSize = static_cast<std::size_t>(wdd_signal_min_cluster_size);

    _wddDanceMaxPosDist = wdd_dance_max_position_distance;
    _wddDanceMaxFrameGap = static_cast<std::size_t>(wdd_dance_max_frame_gap);
    _wddDanceMinConsFrames = static_cast<std::size_t>(wdd_dance_min_consframes);

    if (WDD_VERBOSE)
        printWDDDanceConfig();
}

/*
	* expect the frame to be in target format, i.e. resolution and grayscale,
	* enhanced etc.
	* dont run detection yet
	*/
void WaggleDanceDetector::copyInitialFrame(unsigned long long frame_nr, bool doDetection)
{
    WaggleDanceDetector::WDD_SIGNAL_FRAME_NR = frame_nr;
    DotDetectorLayer::copyFrame(doDetection);
}
/*
	* expect the frame to be in target format, i.e. resolution and grayscale,
	* enhanced etc.
	*/
void WaggleDanceDetector::copyFrame(unsigned long long frame_nr)
{
    WaggleDanceDetector::WDD_SIGNAL_FRAME_NR = frame_nr;
    _execDetection();
}

void WaggleDanceDetector::_execDetection()
{
    //
    // Layer 1
    //
    DotDetectorLayer::copyFrameAndDetect();
    if (DotDetectorLayer::DD_SIGNALS_NUMBER > WDD_LAYER2_MAX_POS_DDS) {
        printf("[WARNING] WDD LAYER 1 OVERFLOW! Drop frame %llu (DD_SIGNALS_NUMBER: %d)\n",
            WaggleDanceDetector::WDD_SIGNAL_FRAME_NR,
            DotDetectorLayer::DD_SIGNALS_NUMBER);
        return;
    }

    //
    // Layer 2
    //

    // run detection on WaggleDanceDetector layer
    _execDetectionGetWDDSignals();

    //
    // Layer 3
    //

    // run top level detection, concat over time
    if (_wddSignal) {
        _execDetectionConcatWDDSignals();
    }

    _execDetectionHousekeepWDDSignals();
}
void WaggleDanceDetector::_execDetectionConcatWDDSignals()
{
    // preallocate
    double dist;
    Dance d;

    bool oldDanceMatched;
    cv::Point2d wdd_signal_pos;
    std::vector<cv::Point2d>* dance_position_ptr;

    // foreach WDD_SIGNAL_ID2POINT_MAP
    for (std::size_t i = 0; i < _wddSignalID2PointMap.size(); i++) {
        oldDanceMatched = false;
        wdd_signal_pos = _wddSignalID2PointMap[i];

        // check distance against known Dances
        for (auto it_dances = _wddUniqueDances.begin(); it_dances != _wddUniqueDances.end(); ++it_dances) {
            dance_position_ptr = &(it_dances->positions);

            // ..and their associated points
            for (auto it_dancepositions = dance_position_ptr->begin(); it_dancepositions != dance_position_ptr->end(); ++it_dancepositions) {
                dist = cv::norm(wdd_signal_pos - *it_dancepositions);

                // wdd signal belongs to known dance
                if (dist < _wddDanceMaxPosDist) {
                    // set flag i.o. to leave loops safely
                    oldDanceMatched = true;

                    // check if position is linear or there is a gap
                    // if it is a consecutive detection, then frame_gap == 1
                    // else there is a gap inbetween layer 2 detections, therefore fill positions
                    unsigned int frame_gap = static_cast<unsigned int>(WDD_SIGNAL_FRAME_NR - it_dances->danceFrameEnd);
                    while (frame_gap > 1) {
                        it_dances->positions.push_back(cv::Point2d(-1.0, -1.0));
                        frame_gap--;
                    }

                    // add current position - assertion: possible gap filled
                    it_dances->positions.push_back(wdd_signal_pos);

                    // save current position as last position;
                    it_dances->positionLast = wdd_signal_pos;

                    // save current frame as last frame
                    it_dances->danceFrameEnd = WDD_SIGNAL_FRAME_NR;

                    if (WDD_VERBOSE > 1)
                        std::cout << WDD_SIGNAL_FRAME_NR << " - UPD: " << it_dances->danceUniqueID
                                  << " [" << it_dances->danceFrameStart << "," << it_dances->danceFrameEnd << "]\n";

                    // jump to WDD_SIGNAL_ID2POINT_MAP loop
                    break;
                }
            }
        }

        if (!oldDanceMatched) {
            // if reached here, NEW dance encountered!
            d.danceUniqueID = _wddDanceID++;
            d.danceFrameStart = WDD_SIGNAL_FRAME_NR;
            d.danceFrameEnd = WDD_SIGNAL_FRAME_NR;
            d.positions.push_back(wdd_signal_pos);
            d.positionLast = wdd_signal_pos;
            gettimeofday(&d.rawTime, nullptr);
            _wddUniqueDances.push_back(d);
        }
    }
}
void WaggleDanceDetector::_execDetectionHousekeepWDDSignals()
{
    //house keeping of Dance Signals
    for (auto it_dances = _wddUniqueDances.begin(); it_dances != _wddUniqueDances.end();) {
        // if Dance did not recieve new signal for over WDD_UNIQ_SIGID_MAX_GAP frames
        if ((WDD_SIGNAL_FRAME_NR - it_dances->danceFrameEnd) > _wddDanceMaxFrameGap) {
            // check for minimum number of consecutiv frames for a positive dance
            // and VFB_MAX_FRAME_HISTORY boundary
            if (((it_dances->danceFrameEnd - it_dances->danceFrameStart) > _wddDanceMinConsFrames) && ((it_dances->danceFrameEnd - it_dances->danceFrameStart) < VFB_MAX_FRAME_HISTORY)) {
                if (WDD_VERBOSE > 1)
                    std::cout << WDD_SIGNAL_FRAME_NR << " - EXEC: " << (*it_dances).danceUniqueID << " [" << (*it_dances).danceFrameStart << "," << (*it_dances).danceFrameEnd << "]" << std::endl;

                _execDetectionFinalizeDance(&(*it_dances));
            }
            if (WDD_VERBOSE > 1)
                std::cout << WDD_SIGNAL_FRAME_NR << " - DEL: " << (*it_dances).danceUniqueID << " [" << (*it_dances).danceFrameStart << "," << (*it_dances).danceFrameEnd << "]" << std::endl;

            it_dances = _wddUniqueDances.erase(it_dances);
        } else {
            ++it_dances;
        }
    }
}
void WaggleDanceDetector::_execDetectionFinalizeDance(Dance* d_ptr)
{
    // set flag
    _wddDance = true;

    // retrieve original frames of detected dance
    std::vector<cv::Mat> seq = _wddVideoFrameBufferPtr->loadCroppedFrameSequence(
        d_ptr->danceFrameStart - _startFrameShift,
        d_ptr->danceFrameEnd - _endFrameShift,
        cv::Point_<int>(d_ptr->positions[0]),
        DotDetectorLayer::FRAME_REDFAC);

    //if number of cropped frames zero, just drop the detection cause something went terribly wrong
    // regarding the frame numbers
    if (seq.empty())
        return;

    d_ptr->orientUVec = WaggleDanceOrientator::extractOrientationFromPositions(d_ptr->positions, d_ptr->positionLast);
    d_ptr->naiveOrientation = WaggleDanceOrientator::extractNaive(d_ptr->positions, d_ptr->positionLast);

    _wddUniqFinishDances.push_back(*d_ptr);
    if (_wddWriteDanceFile)
        _execDetectionWriteDanceFileLine(d_ptr);

    if (WDD_VERBOSE) {
        printf("Waggle dance #%d at:\t %.1f %.1f with orient %.1f (uvec: %.1f,%.1f)\n",
            _wddDanceNumber,
            d_ptr->positions[0].x * pow(2, DotDetectorLayer::FRAME_REDFAC),
            d_ptr->positions[0].y * pow(2, DotDetectorLayer::FRAME_REDFAC),
            uvecToDegree(d_ptr->orientUVec),
            d_ptr->orientUVec.x,
            d_ptr->orientUVec.y);
    }

    WaggleDanceExport::write(seq, d_ptr, auxCC.camId);

    _wddDanceNumber++;
}
/*
	*/
void WaggleDanceDetector::_execDetectionGetDDPotentials()
{

    //std::cout<<"DotDetectorLayer::DD_SIGNALS_NUMBER: "<<DotDetectorLayer::DD_SIGNALS_NUMBER<<std::endl;
}

void WaggleDanceDetector::_execDetectionGetWDDSignals()
{
    // reset GetWDDSignals relevant fields
    _wddSignal = false;
    _wddSignalNumbers = 0;
    _wddSignalID2PointMap.clear();

    // also reset DANCE FLAG
    _wddDance = false;

    // test for minimum number of potent DotDetectors
    if (DotDetectorLayer::DD_SIGNALS_NUMBER < _wddSignalDDMinClusterSize)
        return;

    // get ids (=index in vector) of positive DDs
    // WARNING! Heavly rely on fact that :
    // - DD_ids are [0; DD_POSITIONS_NUMBER-1]
    arma::Col<arma::uword> pos_DD_IDs(DotDetectorLayer::DD_SIGNALS_IDs);

    // init cluster ids for the positive DDs
    arma::Col<arma::sword> pos_DD_CIDs(pos_DD_IDs.size());
    pos_DD_CIDs.fill(-1);
    // init working copy of positive DD ids
    //std::vector<arma::uword> pos_DD_IDs_wset (pos_DD_IDs.begin(), pos_DD_IDs.end());

    // init unique cluster ID
    unsigned int clusterID = 0;
    // foreach positive DD
    for (std::size_t i = 0; i < pos_DD_IDs.size(); i++) {
        // check if DD is missing a cluster ID
        if (pos_DD_CIDs.at(i) >= 0)
            continue;

        // assign unique cluster id
        pos_DD_CIDs.at(i) = clusterID++;

        // assign source id (root id)
        arma::Col<arma::uword> root_DD_id(std::vector<arma::uword>(1, pos_DD_IDs.at(i)));

        // select only unclustered DD as working set
        arma::Col<arma::uword> pos_DD_unclustered_idx = arma::find(pos_DD_CIDs == -1);

        // make a local copy of positive ids from working set
        arma::Col<arma::uword> loc_pos_DD_IDs = pos_DD_IDs.rows(pos_DD_unclustered_idx);

        // init loop
        arma::Col<arma::uword> neighbour_DD_ids = getNeighbours(root_DD_id, arma::Col<arma::uword>(), loc_pos_DD_IDs);

        arma::Col<arma::uword> Lneighbour_DD_ids;

        // loop until no new neighbours are added
        while (Lneighbour_DD_ids.size() != neighbour_DD_ids.size()) {
            // get new discovered elements as
            // {D} = {N_i} \ {N_i-1}
            arma::Col<arma::uword> D = neighbour_DD_ids;
            for (std::size_t j = 0; j < Lneighbour_DD_ids.size(); j++) {
                //MATLAB:  D(D==m) = [];
                arma::uvec q1 = arma::find(D == Lneighbour_DD_ids.at(j), 1);
                if (q1.size() == 1)
                    D.shed_row(q1.at(0));
            }

            // save last neighbours id list
            Lneighbour_DD_ids = neighbour_DD_ids;

            // remove discovered elements from search
            for (std::size_t j = 0; j < neighbour_DD_ids.size(); j++) {
                //MATLAB: loc_pos_DD_IDs(loc_pos_DD_IDs==m) = [];
                arma::uvec q1 = arma::find(loc_pos_DD_IDs == neighbour_DD_ids.at(j), 1);
                if (q1.size() == 1)
                    loc_pos_DD_IDs.shed_row(q1.at(0));
            }

            neighbour_DD_ids = getNeighbours(D, neighbour_DD_ids, loc_pos_DD_IDs);
        }

        // set CIDs of all neighbours
        for (std::size_t j = 0; j < neighbour_DD_ids.size(); j++) {
            arma::uvec q1 = arma::find(pos_DD_IDs == neighbour_DD_ids.at(j), 1);

            if (q1.size() == 1)
                pos_DD_CIDs.at(q1.at(0)) = pos_DD_CIDs.at(i);
            else {
                printf("ERROR! ::execDetectionGetWDDSignals: Unexpected assertion failure!\n");
                exit(19);
            }
        }
    }
    // assertion test
    arma::uvec q1 = arma::find(pos_DD_CIDs == -1);
    if (q1.size() > 0) {
        printf("ERROR! ::execDetectionGetWDDSignals: Unclassified DD signals!\n");
        exit(19);
    }

    // analyze cluster sizes
    arma::uvec count_unqClusterIDs(clusterID);
    count_unqClusterIDs.fill(0);

    // get size of each cluster
    for (std::size_t i = 0; i < pos_DD_CIDs.size(); i++) {
        count_unqClusterIDs.at(pos_DD_CIDs.at(i))++;
    }

    arma::uvec f_unqClusterIDs;
    for (std::size_t i = 0; i < count_unqClusterIDs.size(); i++) {
        if (count_unqClusterIDs.at(i) >= _wddSignalDDMinClusterSize) {
            f_unqClusterIDs.insert_rows(f_unqClusterIDs.size(), 1);
            f_unqClusterIDs.at(f_unqClusterIDs.size() - 1) = i;
        }
    }
    // decide if there is at least one WDD Signal
    if (f_unqClusterIDs.is_empty())
        return;

    _wddSignal = true;

    // foreach remaining cluster calculate center position
    for (std::size_t i = 0; i < f_unqClusterIDs.size(); i++) {
        // find vecotr positions in pos_DD_CIDS <=> pos_DD_IDs
        // associated with cluster id
        arma::uvec idx = arma::find(pos_DD_CIDs == f_unqClusterIDs.at(i));

        cv::Point2d center(0, 0);
        for (std::size_t j = 0; j < idx.size(); j++) {
            center += static_cast<cv::Point_<double>>(DotDetectorLayer::positions[pos_DD_IDs.at(idx.at(j))]);
        }

        center *= 1 / static_cast<double>(idx.size());

        _wddSignalID2PointMap[_wddSignalNumbers++] = center;
    }
    // toggel signal file creation
    if (_wddWriteSignalFile) {
        _execDetectionWriteSignalFileLine();
    }

    //std::cout<<"WDD_SIGNAL_NUMBER: "<<WDD_SIGNAL_NUMBER<<std::endl;
}

arma::Col<arma::uword> WaggleDanceDetector::getNeighbours(
    arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs)
{
    // anchor case
    if (sourceIDs.size() == 1) {
        cv::Point2i DD_pos = DotDetectorLayer::positions[sourceIDs.at(0)];

        for (std::size_t i = 0; i < set_DD_IDs.size(); i++) {
            cv::Point2i DD_pos_other = DotDetectorLayer::positions[set_DD_IDs.at(i)];

            // if others DotDetectors distance is in bound, add its ID
            if (std::sqrt(cv::norm(DD_pos - DD_pos_other)) < _wddSignalDDMaxDistance) {
                N.insert_rows(N.size(), 1);
                N.at(N.size() - 1) = set_DD_IDs.at(i);
            }
        }
    }
    // first recursive level call
    else if (sourceIDs.size() > 1) {
        for (std::size_t i = 0; i < sourceIDs.size(); i++) {
            // remove discoverd neighbours from set
            for (std::size_t j = 0; j < N.size(); j++) {
                arma::uvec q1 = arma::find(set_DD_IDs == N.at(j), 1);
                if (q1.size() == 1)
                    set_DD_IDs.shed_row(q1.at(0));
                else if (q1.size() > 1) {
                    printf("ERROR! ::getNeighbours: Unexpected number of DD Ids found!:\n");
                    exit(19);
                }
            }

            // check if some ids are left
            if (set_DD_IDs.is_empty())
                break;

            arma::Col<arma::uword> _sourceIDs;
            _sourceIDs.insert_rows(0, 1);
            _sourceIDs.at(0) = sourceIDs.at(i);

            N = getNeighbours(_sourceIDs, N, set_DD_IDs);
        }
    } else {
        printf("ERROR! ::getNeighbours: Unexpected assertion failure!\n");
        exit(19);
    }

    return N;
}
void WaggleDanceDetector::_execDetectionWriteDanceFileLine(Dance* d_ptr)
{
    unsigned long long start = d_ptr->danceFrameStart;
    unsigned long long end = d_ptr->danceFrameEnd;

    unsigned long long numFrames = end - start + 1;

    if (numFrames == d_ptr->positions.size()) {
        auto it = d_ptr->positions.begin();
        while (start <= end) {
            fprintf(_danceFilePtr, "%d %.2f %.2f %.1f\n", start,
                it->x * pow(2, DotDetectorLayer::FRAME_REDFAC),
                it->y * pow(2, DotDetectorLayer::FRAME_REDFAC),
                uvecToDegree(d_ptr->orientUVec));
            start++;
            ++it;
        }
    } else {
        printf("Warning! Dance Id %d (%d - %d) has corrupted position size: %d (not matching numFrames: %d)\n",
            d_ptr->danceUniqueID, start, end, d_ptr->positions.size(), numFrames);
    }
}

void WaggleDanceDetector::_execDetectionWriteSignalFileLine()
{
    fprintf(_signalFilePtr, "%I64u", WDD_SIGNAL_FRAME_NR);
    for (auto it = _wddSignalID2PointMap.begin(); it != _wddSignalID2PointMap.end(); ++it) {
        fprintf(_signalFilePtr, " %.5f %.5f",
            it->second.x * pow(2, DotDetectorLayer::FRAME_REDFAC), it->second.y * pow(2, DotDetectorLayer::FRAME_REDFAC));
    }
    fprintf(_signalFilePtr, "\n");
}
bool WaggleDanceDetector::isWDDSignal()
{
    return _wddSignal;
}
bool WaggleDanceDetector::isWDDDance()
{
    return _wddDance;
}
std::size_t WaggleDanceDetector::getWDDSignalNumber()
{
    return _wddSignalNumbers;
}
const std::map<std::size_t, cv::Point2d>* WaggleDanceDetector::getWDDSignalId2PointMap()
{
    return &_wddSignalID2PointMap;
}
const std::vector<Dance>* WaggleDanceDetector::getWDDFinishedDancesVec()
{
    return &_wddUniqFinishDances;
}
void WaggleDanceDetector::printWDDDanceConfig()
{
    printf("Printing WDD signal configuration:\n");
    printf("[WDD_DANCE_MAX_POS_DIST] %.1f\n", _wddDanceMaxPosDist);
    printf("[WDD_DANCE_MAX_FRAME_GAP] %ul\n", _wddDanceMaxFrameGap);
    printf("[WDD_DANCE_MIN_CONSFRAMES] %ul\n", _wddDanceMinConsFrames);
}
std::size_t WaggleDanceDetector::getWDD_SIGNAL_DD_MIN_CLUSTER_SIZE()
{
    return _wddSignalDDMinClusterSize;
}
void WaggleDanceDetector::setWDD_SIGNAL_DD_MIN_CLUSTER_SIZE(std::size_t val)
{
    if (val > 0)
        _wddSignalDDMinClusterSize = val;
}
} /* namespace WaggleDanceDetector */
