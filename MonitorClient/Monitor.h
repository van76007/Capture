#pragma once
#include "Global.h"
#include <string>
#include <hash_map>
#include <vector>
#include <list>

/*
   Class: Monitor

   Monitor file system
*/

/*
   Constants: Kernel Driver IOCTL Codes

   IOCTL_CAPTURE_START - Starts the kernel drivers monitor.
   IOCTL_CAPTURE_STOP - Stops the kernel drivers monitor.
*/
#define IOCTL_CAPTURE_START    CTL_CODE(0x00000022, 0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CAPTURE_STOP    CTL_CODE(0x00000022, 0x0806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define CAPTURE_FILEMON_PORT_NAME	L"\\CaptureFileMonitorPort"

/*
	Struct: FILEMONITOR_MESSAGE

	Contains a command to be sent through to the kernel driver

	Command - Actual command to be sent
*/

// TBA: May not need GetFileEvents 
typedef enum _FILEMONITOR_COMMAND {
    GetFileEvents,
	GetParameters
} FILEMONITOR_COMMAND;

//
typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;

typedef struct _FILEMONITOR_SETUP {
	UINT nLogVolumeSize;
	WCHAR wszLogVolume[1024];
	UINT nLogDirectorySize;
	WCHAR wszLogDirectory[1024];
} FILEMONITOR_SETUP, *PFILEMONITOR_SETUP;

class Monitor
{
public:
	Monitor(const std::wstring& mDrive, const std::wstring& lDir);
	~Monitor();
	
	inline bool isDriverInstalled() { return driverInstalled; }
	void setMonitorTempFiles(bool monitor);
	void start();
	void stop();

private:
	void initialiseDosNameMap();
    std::wstring convertFileObjectNameToDosName(std::wstring fileObjectName);
    
	HANDLE communicationPort;
	bool driverInstalled;
	std::wstring monitoredDrive; // TBA
	std::wstring logDir; // TBA
	stdext::hash_map<std::wstring, std::wstring> dosNameMap;
};