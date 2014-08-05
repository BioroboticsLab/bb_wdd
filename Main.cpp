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
	% rotatet 90 degree counterclock wise - 0� is at top center
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

// saves loaded, modified camera configs
std::vector<CamConf> camConfs;

// saves the next unique camId
std::size_t nextUniqueCamID = 0;

// format of config file:
// <camId> <GUID> <Arena.p1> <Arena.p2> <Arena.p3> <Arena.p4>
void loadCamConfigFileReadLine(std::string line)
{
	char * delimiter = " ";
	std::size_t pos = 0;

	std::size_t tokenNumber = 0;

	std::size_t camId = NULL;
	char guid_str[64];
	std::array<cv::Point2i,4> arena;

	//copy & convert to char *
	char * string1 = _strdup(line.c_str());

	// parse the line
	char *token = NULL;
	char *next_token = NULL;
	token = strtok_s(string1, delimiter, &next_token);

	int arenaPointNumber = 0;
	while (token != NULL)
	{
		int px, py;

		// camId
		if(tokenNumber == 0)
		{
			camId = atoi(token);
		}
		// guid
		else if( tokenNumber == 1)
		{
			strcpy_s(guid_str, token);			
		}
		else
		{
			switch (tokenNumber % 2)
			{
				// arena.pi.x
			case 0:
				px = atoi(token);
				break;
				// arena.pi.y
			case 1:
				py = atoi(token);
				arena[arenaPointNumber++] = cv::Point2i(px,py);
				break;
			}

		}

		tokenNumber++;
		token = strtok_s( NULL, delimiter, &next_token);
	}
	free(string1);
	free(token);

	if(tokenNumber != 10)
		std::cerr<<"Warning! cams.config file contains corrupted line with total tokenNumber: "<<tokenNumber<<std::endl;

	struct CamConf c;
	c.camId = camId;
	strcpy_s(c.guid_str, guid_str);	
	c.arena = arena;
	c.configured = true;

	// save loaded CamConf  to global vector
	camConfs.push_back(c);

	// keep track of loaded camIds and alter nextUniqueCamID accordingly
	if(camId >= nextUniqueCamID)
		nextUniqueCamID = camId + 1;
}
char camConfPath[] = "\\cams.config";
void loadCamConfigFile()
{
	extern char _FULL_PATH_EXE[MAX_PATH];
	char BUFF[MAX_PATH];
	strcpy_s(BUFF ,MAX_PATH, _FULL_PATH_EXE);
	strcat_s(BUFF, MAX_PATH, camConfPath);

	if(!fileExists(BUFF))
	{
		// create empty file
		std::fstream f;
		f.open(BUFF, std::ios::out );
		f << std::flush;
		f.close();
	}

	std::string line;
	std::ifstream camconfigfile;
	camconfigfile.open(BUFF);

	if(camconfigfile.is_open())
	{
		while (getline(camconfigfile,line))
		{
			loadCamConfigFileReadLine(line);
		}
		camconfigfile.close();
	}
	else
	{
		std::cerr<<"Error! Can not open cams.config file!"<<std::endl;
		exit(111);
	}
}
void saveCamConfigFile()
{	
	extern char _FULL_PATH_EXE[MAX_PATH];
	char BUFF[MAX_PATH];
	strcpy_s(BUFF ,MAX_PATH, _FULL_PATH_EXE);
	strcat_s(BUFF, MAX_PATH, camConfPath);
	FILE * camConfFile_ptr;
	fopen_s (&camConfFile_ptr, BUFF, "w+");

	for(auto it=camConfs.begin(); it!=camConfs.end(); ++it)
	{
		if(it->configured)
		{
			fprintf_s(camConfFile_ptr,"%d %s ",it->camId,it->guid_str);
			for(unsigned i=0;i<4;i++)
				fprintf_s(camConfFile_ptr,"%d %d ", it->arena[i].x, it->arena[i].y);
			fprintf_s(camConfFile_ptr,"\n");
		}
	}
	fclose(camConfFile_ptr);
}
inline void guidToString(GUID g, char * buf){
	char _buf[64];
	sprintf_s(_buf, "[%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]",
		g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2],
		g.Data4[3], g.Data4[4], g.Data4[5],
		g.Data4[6], g.Data4[7]);

	strcpy_s(buf,64,_buf);	
}

// saves to full path to executable
char _FULL_PATH_EXE[MAX_PATH];

int main(int nargs, char** argv)
{	

	printf("WaggleDanceDetection Version %s - compiled at %s\n\n",
			"1.1", "05.08.2014");

	// get the full path to executable 
	getExeFullPath(_FULL_PATH_EXE, sizeof(_FULL_PATH_EXE));

	//char videoFilename[MAXCHAR];
	// WaggleDanceExport initialization
	WaggleDanceExport::execRootExistChk();

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

	// Query & temporaly save unique camera uuid
	for(int i = 0; i < numCams; i++)
	{
		_guids[i] = CLEyeGetCameraUUID(i);
	}

	// Load & store cams.config file
	loadCamConfigFile();

	// prepare container for camIds
	std::vector<std::size_t> camIdsLaunch;

	// merge data from file and current connected cameras
	// compare guids connected to guids loaded from file 
	// - if match, camera has camId and arena properties -> configured=true
	// - else it gets new camId -> configured=false
	for(int i = 0; i < numCams; i++)
	{		
		char guid_str_connectedCam[64];
		guidToString(_guids[i], guid_str_connectedCam);

		bool foundMatch = false;

		for(auto it=camConfs.begin(); it!=camConfs.end(); ++it)
		{
			if(strcmp(guid_str_connectedCam, it->guid_str) == 0)
			{
				foundMatch = true;
				camIdsLaunch.push_back(it->camId);
				break;
			}
		}
		// no match found -> new config
		// find next camId
		if(!foundMatch)
		{
			struct CamConf c;
			c.camId = nextUniqueCamID++;
			camIdsLaunch.push_back(c.camId);
			strcpy_s(c.guid_str, guid_str_connectedCam);
			c.arena[0] = cv::Point2i(0,0);
			c.arena[1] = cv::Point2i(639,0);
			c.arena[2] = cv::Point2i(639,479);
			c.arena[3] = cv::Point2i(0,479);
			c.configured = false;

			camConfs.push_back(c);
		}
	}

	// for all camIds pushed to launch retrieve information and push to user prompt
	printf("CamID    GUID                                   configured?\n");
	printf("***********************************************************\n");

	for(std::size_t i = 0; i < camIdsLaunch.size(); i++)
	{				
		std::size_t _camId = camIdsLaunch[i];
		CamConf * cc_ptr = NULL;
		for(auto it=camConfs.begin(); it!=camConfs.end(); ++it)
			if(it->camId == _camId)
				cc_ptr = &(*it);

		if(cc_ptr != NULL)
			printf("%d\t %s\t%s\n", cc_ptr->camId, cc_ptr->guid_str, cc_ptr->configured ? "true" : "false");
	}
	printf("\n\n");

	// retrieve users camId choice
	std::string in;
	std::size_t camIdUserSelect = NULL;
	bool camId_isSelected = false;
	CLEyeCameraCapture *pCam = NULL;

	while(!camId_isSelected){
		std::cout << " -> Please selet camera id to start:" <<std::endl;
		std::getline(std::cin, in);

		try {
			std::size_t i_dec = static_cast<std::size_t>(std::stoi (in,nullptr));
			for(auto it = camIdsLaunch.begin(); it!=camIdsLaunch.end(); ++it)
			{
				if(*it == i_dec)
				{
					std::cout<< " -> "<<i_dec<<" selected!"<<std::endl;
					camIdUserSelect = i_dec;
					camId_isSelected = true;
				}else{
					std::cout<<i_dec<<" unavailable!"<<std::endl;
				}
			}
		}
		catch (const std::invalid_argument& ia) {
			std::cerr << "Invalid argument: " << ia.what() << '\n';
		}
	}

	// retrieve guid from camConfs according to camId
	char windowName[64];
	for(auto it=camConfs.begin(); it!=camConfs.end(); ++it)
	{
		if(it->camId == camIdUserSelect)
		{
			sprintf_s(windowName, "WaggleDanceDetector - CamID: %d", camIdUserSelect);

			char guid_str_connectedCam[64];
			bool foundMatch = false;
			for (int i=0; i< numCams; i++)
			{
				guidToString(_guids[i], guid_str_connectedCam);

				if(strcmp(guid_str_connectedCam, it->guid_str) == 0)
				{
					foundMatch = true;
					pCam = new CLEyeCameraCapture(windowName, _guids[i], CLEYE_MONO_PROCESSED, CLEYE_QVGA, WDD_FRAME_RATE, *it);
					break;break;
				}
			}


		}
	}
	printf("Starting WaggleDanceDetector - CamID: %d\n", camIdUserSelect);

	const CamConf * cc_ptr = pCam->getCamConfPtr();

	if(!cc_ptr->configured){
		// run Setup mode first
		pCam->StartCapture();
		int param = -1, key;

		while((key = cvWaitKey(0)) != 0x1b)
		{}

		pCam->StopCapture();
		pCam->setSetupModeOn(false);

		// update camConfs
		for(auto it=camConfs.begin(); it!=camConfs.end(); ++it)
		{
			if(it->camId == cc_ptr->camId)
			{
				camConfs.erase(it);
				break;
			}
		}
		camConfs.push_back(*cc_ptr);
		saveCamConfigFile();
	}

	//write back camConfs file

	pCam->StartCapture();
	int param = -1, key;

	while((key = cvWaitKey(0)) != 0x1b)
	{}

	pCam->StopCapture();
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

	delete pCam;
	delete _guids;
	return 0;
}
