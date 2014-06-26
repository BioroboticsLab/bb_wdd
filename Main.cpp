#include "stdafx.h"
#include "CLEyeCameraCapture.h"
#include "DotDetectorLayer.h"
#include "InputVideoParameters.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"
#include "WaggleDanceExport.h"

using namespace wdd;

double uvecToDegree(cv::Point2d in)
{
	if(_isnan(in.x) | _isnan(in.y))
		return std::numeric_limits<double>::quiet_NaN();
	/*
	% rotatet 90 degree counterclock wise - 0° is at top center
	R= [  0     1; -1     0];

	u = u * R;

	theta = atan2(u(2),u(1));
	ThetaInDegrees = (theta*180/pi);
	*/

	double theta = atan2(in.y,in.x);
	return theta * 180/CV_PI;
}
void getExeFullPath(char * out, std::size_t size)
{
	char BUFF[MAX_PATH];

	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		GetModuleFileName(hModule, BUFF, sizeof(BUFF)/sizeof(char)); 
		// remove WaggleDanceDetector.exe part (23 chars :P)
		_tcsncpy_s(out, size, BUFF, strlen(BUFF)-23-1);
	}
	else
	{
		std::cerr << "Error! Module handle is NULL - can not retrive exe path!" << std::endl ;
		exit(-2);
	}
}

bool fileExists (const std::string& file_name)
{
	struct stat buffer;
	return (stat (file_name.c_str(), &buffer) == 0);
}

bool dirExists(const char * dirPath)
{
	int result = PathIsDirectory((LPCTSTR)dirPath);

	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}

// saves to full path to executable
char _FULL_PATH_EXE[MAX_PATH]; 

int main(int nargs, char** argv)
{	
	// get the full path to executable 
	getExeFullPath(_FULL_PATH_EXE, sizeof(_FULL_PATH_EXE));
	
	//char videoFilename[MAXCHAR];
	// WaggleDanceExport initialization
	WaggleDanceExport::execRootExistChk();


	CLEyeCameraCapture *cam = NULL;
	// Query for number of connected cameras
	int numCams = CLEyeGetCameraCount();
	if(numCams == 0)
	{
		printf("Error No PS3Eye cameras detected\n");
		return -1;
	}
	char windowName[64];
	// Query unique camera uuid
	GUID guid = CLEyeGetCameraUUID(0);
	printf("Camera GUID: [%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x]\n", 
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	sprintf_s(windowName, "WaggleDanceDetector Window - CamID:");
	// Create camera capture object
	// Randomize resolution and color mode

	cam = new CLEyeCameraCapture(windowName, guid, CLEYE_MONO_RAW, CLEYE_QVGA, WDD_FRAME_RATE);

	printf("Starting capture\n");
	cam->StartCapture();

	printf("Use the following keys to change camera parameters:\n"
		"\t'g' - select gain parameter\n"
		"\t'e' - select exposure parameter\n"
		"\t'z' - select zoom parameter\n"
		"\t'r' - select rotation parameter\n"
		"\t'+' - increment selected parameter\n"
		"\t'-' - decrement selected parameter\n");
	// The <ESC> key will exit the program
	CLEyeCameraCapture *pCam = NULL;
	int param = -1, key;
	while((key = cvWaitKey(0)) != 0x1b)
	{
		switch(key)
		{
		case 'v':				printf("Toggle Visual\n");		pCam->setVisual(true);	break;
					case 'V':	printf("Toggle Visual\n");		pCam->setVisual(false);	break;
		//case 'g':	case 'G':	printf("Parameter Gain\n");		param = CLEYE_GAIN;		break;
		//case 'e':	case 'E':	printf("Parameter Exposure\n");	param = CLEYE_EXPOSURE;	break;
		case 'z':	case 'Z':	printf("Parameter Zoom\n");		param = CLEYE_ZOOM;		break;
		case 'r':	case 'R':	printf("Parameter Rotation\n");	param = CLEYE_ROTATION;	break;
		case '+':	if(cam)		cam->IncrementCameraParameter(param);					break;
		case '-':	if(cam)		cam->DecrementCameraParameter(param);					break;
		}
	}
	cam->StopCapture();
	delete cam;
	return 0;
}
