#include "Monitor.h"
#include <string>
#include <iostream>
#include <stdlib.h>
#include <fltuser.h>
#include <assert.h>

Monitor::Monitor(const std::wstring& mDrive, const std::wstring& lDir)
{
	HRESULT hResult;
	driverInstalled = false;
	int turn = 0;
	communicationPort = INVALID_HANDLE_VALUE;

	monitoredDrive = mDrive;

	// Log Dir must in the format L"\\Logs\\"
	logDir = L"\\";
	logDir += lDir.c_str();
	logDir += L"\\";
	// Debug
	printf("logDir is %ls\n", logDir.c_str());

	// Unload previous instance of driver AND Load the new instance of driver
	FilterUnload(L"FileMonDriver");
	hResult = FilterLoad(L"FileMonDriver");
	
	// Connect to FileMonDriver sys
	if (!IS_ERROR( hResult )) {
		
		hResult = FilterConnectCommunicationPort( CAPTURE_FILEMON_PORT_NAME,
												0,
												NULL,
												0,
												NULL,
												&communicationPort );
		if (IS_ERROR( hResult )) {

			printf("FileMonitor: ERROR - Could not connect to filter: 0x%08x\n");
		} else {
			printf("Loaded filter driver: FileMonDriver\n");
			driverInstalled = true;
		}
	}
}

Monitor::~Monitor(void)
{
	stop();
	printf("Monitor stopped \n");
}

void 
Monitor::initialiseDosNameMap()
{
	UCHAR buffer[1024];
	PFILTER_VOLUME_BASIC_INFORMATION volumeBuffer = (PFILTER_VOLUME_BASIC_INFORMATION)buffer;
	HANDLE volumeIterator = INVALID_HANDLE_VALUE;
	ULONG volumeBytesReturned;
	WCHAR driveLetter[15];

	HRESULT hResult = FilterVolumeFindFirst( FilterVolumeBasicInformation,
										 volumeBuffer,
										 sizeof(buffer)-sizeof(WCHAR),   //save space to null terminate name
										 &volumeBytesReturned,
										 &volumeIterator );
	if (volumeIterator != INVALID_HANDLE_VALUE) {
		do {
			assert((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
			__analysis_assume((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
			volumeBuffer->FilterVolumeName[volumeBuffer->FilterVolumeNameLength/sizeof( WCHAR )] = UNICODE_NULL;
				
			if(SUCCEEDED( FilterGetDosName(volumeBuffer->FilterVolumeName,
				driveLetter,
				sizeof(driveLetter)/sizeof(WCHAR) )
				))
			{
				std::wstring dLetter = driveLetter;
				dosNameMap.insert(std::pair<std::wstring, std::wstring>(volumeBuffer->FilterVolumeName, dLetter));
				// Format of hashmap (Volume, DriverLetter)
			}
		} while (SUCCEEDED( hResult = FilterVolumeFindNext( volumeIterator,
															FilterVolumeBasicInformation,
															volumeBuffer,
															sizeof(buffer)-sizeof(WCHAR),    //save space to null terminate name
															&volumeBytesReturned ) ));
	}

	if (INVALID_HANDLE_VALUE != volumeIterator) {
		FilterVolumeFindClose( volumeIterator );
	}
}

// Convert smt from "E:\Logs\" -> "\Device\HarddiskVolume2\Logs\" and this will be passed to the kernel mode
std::wstring 
Monitor::convertFileObjectNameToDosName(std::wstring fileObjectName)
{
	stdext::hash_map<std::wstring, std::wstring>::iterator it;

	for(it = dosNameMap.begin(); it != dosNameMap.end(); it++)
	{
		size_t position = fileObjectName.rfind(it->second,0);
		if(position != std::wstring::npos)
		{
			// TBA: it->second.length() ?
			fileObjectName.replace(position, it->second.length(), it->first, 0, it->first.length());
			break;
		}
	}
	return fileObjectName;
}

// TBA
void
Monitor::setMonitorTempFiles(bool monitor)
{
	FILEMONITOR_MESSAGE command;
	FILEMONITOR_SETUP setupFileMonitor;
	HRESULT hResult;
	DWORD bytesReturned;
	std::wstring logDirDOSPath = L"";
	std::wstring logVolume = L"";

	if(monitor == true)
	{
		/*
		Create logDirPath from monitoredDrive, logDir 
		(monitoredDrive, logDir) = E:\Log
		*/
		// Final format logDirDOSPath "E:\\Log\"
		logDirDOSPath = monitoredDrive;
		logDirDOSPath += logDir.c_str();
		
		printf("Creating Log directory %ls\n", logDirDOSPath.c_str());

		CreateDirectory(logDirDOSPath.c_str(), NULL);

		// Translate logDirDOSPath to logDirPath L"E:\\Log\\" -> L"\\Device\HarddiskVolume2\Log\\"
		// Create the map of DoS driver name. 
		initialiseDosNameMap();
		
		//logDirPath = convertFileObjectNameToDosName(logDirDOSPath);
		// Look up logVolume give logDrive
		stdext::hash_map<std::wstring, std::wstring>::iterator it;
		
		for(it = dosNameMap.begin(); it != dosNameMap.end(); it++)
		{
			if (it->second == monitoredDrive)
			{
				printf("it->first is %ls\n", it->first.c_str());
			    logVolume = it->first;
				break;
			}
		}

		// Prepare out-parameter buf LogDrive. In fact the filter can *get* the parameter via this buf
		/*
		GetFullPathName(logVolume.c_str(), 1024, setupFileMonitor.wszLogVolume, NULL);
		setupFileMonitor.nLogVolumeSize = (UINT)wcslen(setupFileMonitor.wszLogVolume)*sizeof(wchar_t);
		*/
		int i=0;
		std::wstring::iterator pos_logVolume = logVolume.begin();
		while (pos_logVolume != logVolume.end())
		{
			setupFileMonitor.wszLogVolume[i] = *pos_logVolume;
			++pos_logVolume;
			++i;
		}

		setupFileMonitor.nLogVolumeSize = (UINT)wcslen(setupFileMonitor.wszLogVolume)*sizeof(wchar_t);

		printf("Log volume: %i -> %ls\n", setupFileMonitor.nLogVolumeSize, setupFileMonitor.wszLogVolume);

		// Prepare out-parameter buf LogDir. In fact the filter can *get* the parameter via this buf
		/*
		GetFullPathName(logDir.c_str(), 1024, setupFileMonitor.wszLogDirectory, NULL);
		setupFileMonitor.nLogDirectorySize = (UINT)wcslen(setupFileMonitor.wszLogDirectory)*sizeof(wchar_t);
		*/
		int j=0;
		std::wstring::iterator pos_logDir = logDir.begin();
		while (pos_logDir != logDir.end())
		{
			setupFileMonitor.wszLogDirectory[j] = *pos_logDir;
			++pos_logDir;
			++j;
		}

		setupFileMonitor.nLogDirectorySize = (UINT)wcslen(setupFileMonitor.wszLogDirectory)*sizeof(wchar_t);
		
		printf("Log directory: %i -> %ls\n", setupFileMonitor.nLogDirectorySize, setupFileMonitor.wszLogDirectory);
		
		// Setup the message sent to kernel-mode. Cann't be NULL
		command.Command = GetParameters;
		
		// Send the message to kernel-mode application

		hResult = FilterSendMessage( communicationPort,
								 &command,
								 sizeof( FILEMONITOR_COMMAND ),
								 &setupFileMonitor,
								 sizeof(FILEMONITOR_SETUP),
								 &bytesReturned );

		// Don't need to wait for the reply
	}
}

void
Monitor::start()
{
	printf("Monitor starting...\n");

	/* Send parameters to FileMonDriver */
	setMonitorTempFiles(true);

	/*
	wchar_t exListDriverPath[1024];
	GetFullPathName(L"FileMonitor.exl", 1024, exListDriverPath, NULL);
	Monitor::loadExclusionList(exListDriverPath);

	// Create a log directory exclusion which allows all writes in Captures log directory
	wchar_t logDirectory[1024];
	// Unconfigurable path2 logDir
	GetFullPathName(L"logs", 1024, logDirectory, NULL);
	std::wstring captureLogDirectory = logDirectory;
	Monitor::prepareStringForExclusion(captureLogDirectory);
	captureLogDirectory += L"\\\\.*";

	std::wstring captureExecutablePath = ProcessManager::getInstance()->getProcessPath(GetCurrentProcessId());
	Monitor::prepareStringForExclusion(captureExecutablePath); // Van: All file writes to this dir will not be copied
	
	// Exclude file events in the log directory
	// NOTE we exclude all processes because files are copied/delete/opene etc in the context of the calling process not Capture
	Monitor::addExclusion(L"+", L"write", L".*", captureLogDirectory, true);
	Monitor::addExclusion(L"+", L"create", L".*", captureLogDirectory, true);
	Monitor::addExclusion(L"+", L"delete", L".*", captureLogDirectory, true);
	Monitor::addExclusion(L"+", L"open", L".*", captureLogDirectory, true);
	Monitor::addExclusion(L"+", L"read", L".*", captureLogDirectory, true);

	// Exclude Captures temp directory
	wchar_t tempDirectory[1024];
	GetFullPathName(L"temp", 1024, tempDirectory, NULL);
	std::wstring captureTempDirectory = tempDirectory;
	Monitor::prepareStringForExclusion(captureTempDirectory);
	captureTempDirectory += L"\\\\.*";

	// Exclude file events in the log directory
	// NOTE we exclude all processes because files are copied/delete/openend etc in the context of the calling process not Capture
	Monitor::addExclusion(L"+", L"write", L".*", captureTempDirectory, true);
	Monitor::addExclusion(L"+", L"create", L".*", captureTempDirectory, true);
	Monitor::addExclusion(L"+", L"delete", L".*", captureTempDirectory, true);
	Monitor::addExclusion(L"+", L"open", L".*", captureTempDirectory, true);
	Monitor::addExclusion(L"+", L"read", L".*", captureTempDirectory, true);
	
	if(OptionsManager::getInstance()->getOption(L"log-system-events-file") != L"")
	{
		std::wstring loggerFile = Logger::getInstance()->getLogFullPath();
		Monitor::prepareStringForExclusion(loggerFile);
		Monitor::addExclusion(L"+", L"create", captureExecutablePath, loggerFile, true);
		Monitor::addExclusion(L"+", L"write", captureExecutablePath, loggerFile, true);
	}
	onFileExclusionReceivedConnection = EventController::getInstance()->connect_onServerEvent(L"file-exclusion", boost::bind(&FileMonitor::onFileExclusionReceived, this, _1));
	
	// Create the map of DoS driver name. 
	initialiseDosNameMap();
	*/
}

void
Monitor::stop()
{	
	printf("Monitor stopping...\n");
	//
	if(isDriverInstalled())
	{
		//
		printf("Driver is Installed \n");
		driverInstalled = false;
		//
		CloseHandle(communicationPort);
		printf("After CloseHandle \n");
		//
		FilterUnload(L"FileMonDriver");
		printf("After FilterUnload \n");
	}
	//
	printf("Done. Monitor stopping...\n");
}
