#include "Camera.h"

#include <stdexcept>

Camera::Camera(int cameraIndex, double frameRate, double frameWidth, double frameHeight)
    : _cap(cameraIndex)
{
    if (!_cap.isOpened()) {
        throw std::runtime_error("Camera device not opened successfully;");
    }

    _cap.set(CV_CAP_PROP_FPS, frameRate);
    _cap.set(CV_CAP_PROP_FRAME_WIDTH, frameWidth);
    _cap.set(CV_CAP_PROP_FRAME_HEIGHT, frameHeight);
    //_cap.set(CV_CAP_PROP_MONOCHROME, true);
}

void Camera::nextFrame()
{
    _cap >> _frame;
    cv::cvtColor(_frame, _frame, CV_BGR2GRAY);
}

cv::Mat* Camera::getFramePointer()
{
    return &_frame;
}

/*

VideoCapture cap(0); // open the default camera

if(!cap.isOpened())  // check if we succeeded
        return -1;


    // With webcam get(CV_CAP_PROP_FPS) does not work.
    // Let's see for ourselves.

    double fps = cap.get(CV_CAP_PROP_FPS);
    // If you do not care about backward compatibility
    // You can use the following instead for OpenCV 3
    // double fps = video.get(CAP_PROP_FPS);
    cout << "Frames per second using video.get(CV_CAP_PROP_FPS) : " << fps << endl;


    // Number of frames to capture
    int num_frames = 240;

    // Start and end times
    time_t start, end;

    // Variable for storing video frames
    Mat frame;

    cout << "Capturing " << num_frames << " frames" << endl ;

    // warmup
    for(int i = 0; i < num_frames; i++)
    {
        cap >> frame;
    }

    // Start time
    time(&start);

    // Grab a few frames
    for(int i = 0; i < num_frames; i++)
    {
        cap >> frame;
    }

    // End Time
    time(&end);

    // Time elapsed
    double seconds = difftime (end, start);
    cout << "Time taken : " << seconds << " seconds" << endl;

    // Calculate frames per second
    fps  = num_frames / seconds;
    cout << "Estimated frames per second : " << fps << endl;

    namedWindow("frame",1);
    for(;;)
    {
        Mat frame;
        cap >> frame; // get a new frame from camera
        cv::rotate(frame, frame, ROTATE_90_COUNTERCLOCKWISE);
        cv::resize(frame, frame, cv::Size(0, 0), 2, 2, INTER_LANCZOS4);
        imshow("frame", frame);
        waitKey(1);
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;

    */
