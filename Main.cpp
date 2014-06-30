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

void loadCamConfigFile()
{
	std::string line;
	std::ifstream camconfigfile("cams.config");
	if(camconfigfile.is_open())
	{
		while (getline(camconfigfile,line))
		{
			std::cout<<line<<std::endl;
		}
		camconfigfile.close();
	}
	else
	{
		std::cerr<<"Error! Can not open cams.config file!"<<std::endl;
		exit(111);
	}
}
inline std::string guidToString(GUID g){
	char buff[64];
	sprintf_s(buff, "[%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]",
		g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2],
		g.Data4[3], g.Data4[4], g.Data4[5],
		g.Data4[6], g.Data4[7]);
	return buff;
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


	CLEyeCameraCapture *pCam = NULL;
	GUID * _guids;

	// Query for number of connected cameras
	int numCams = CLEyeGetCameraCount();

	if(numCams == 0)
	{
		printf("Error No PS3Eye cameras detected\n");
		exit(-1);
	}
	else
	{
		_guids = new GUID [numCams];
	}
	printf("CamID    GUID                                   configured?\n");
	printf("***********************************************************\n");
	for(int i = 0; i < numCams; i++)
	{
		// Query & temporaly save unique camera uuid
		_guids[i] = CLEyeGetCameraUUID(i);
		printf("%d\t %s\tno\n", i, guidToString(_guids[i]));
	}
	printf("\n\n");
	std::string in;
	std::size_t camId;

	while(true){
		std::cout << " -> Please selet camera id to start:" <<std::endl;
		std::getline(std::cin, in);

		try {
			int i_dec = std::stoi (in,nullptr);
			if((0<= i_dec) && (i_dec <= numCams-1))
			{
				camId = static_cast<std::size_t>(i_dec);
				break;
			}
		}
		catch (const std::invalid_argument& ia) {
			std::cerr << "Invalid argument: " << ia.what() << '\n';
		}

	}

	char windowName[64];
	sprintf_s(windowName, "WaggleDanceDetector - CamID: %d", camId);
	pCam = new CLEyeCameraCapture(windowName, _guids[camId], CLEYE_MONO_RAW, CLEYE_QVGA, WDD_FRAME_RATE);
	printf("Starting WaggleDanceDetector - CamID: %d\n", camId);
	pCam->StartCapture();

	/*
	printf("Use the following keys to change camera parameters:\n"
	"\t'g' - select gain parameter\n"
	"\t'e' - select exposure parameter\n"
	"\t'z' - select zoom parameter\n"
	"\t'r' - select rotation parameter\n"
	"\t'+' - increment selected parameter\n"
	"\t'-' - decrement selected parameter\n");
	// The <ESC> key will exit the program
	CLEyeCameraCapture *pCam = NULL;
	*/
	int param = -1, key;

	while((key = cvWaitKey(0)) != 0x1b)
	{
		/*
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

		*/
	}
	pCam->StopCapture();
	delete pCam;
	delete _guids;
	return 0;
}
