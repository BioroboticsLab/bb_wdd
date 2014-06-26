#pragma once
namespace wdd{	
	class CLEyeCameraCapture
	{
		CHAR _windowName[256];
		GUID _cameraGUID;
		CLEyeCameraInstance _cam;
		CLEyeCameraColorMode _mode;
		CLEyeCameraResolution _resolution;
		float _fps;
		HANDLE _hThread;
		bool _running;

		int _FRAME_WIDTH;
		int _FRAME_HEIGHT;
		bool _visual;
	public:
		CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps);

		bool StartCapture();
		void StopCapture();

		void setVisual(bool visual);

		void IncrementCameraParameter(int param);

		void DecrementCameraParameter(int param);

		void Run();

		static DWORD WINAPI CaptureThread(LPVOID instance);

	};
}