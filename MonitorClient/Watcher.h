#pragma once
#include <string>
#include "Global.h"
#include "Thread.h"
#include "TFTPClient.h"
#include <map>

/*
   Class: Server

   An TFTP server to send the log files in log Dir.
   Algo: Periodically refresh the logDir. Send file one by one and delete the file after sending.
   Unclear: If w can open and read the file when the kernel filter is still writting to it.
           - How not to send the garbage symlinks
*/

// List all files under the logDir every 30000 ms or 30s
#define REFRESH_DIR_INTERVAL 3000
#define WAIT_STOPPING_OBJECT 1000
#define WAIT_KILLING_THREAD 5000

class Watcher : public Runnable
{
public:

	Watcher(const std::wstring& sAddress, int p, const std::wstring& mDrive, const std::wstring& lDir);
	~Watcher();

	// Open the socket and prepare 
	bool connectToServer(bool startTFTPClient);
	void start();
	void stop();
	void run();
	
	inline bool isWatcherRunning() { return watcherRunning; }
	
private:

	int RefreshDirectory(const std::wstring& lDirPath);
	// Associated class
	TFTPClient* tftpClient;
	
	//
	std::wstring serverAddress;
	int port;
	std::wstring monitoredDrive;
	std::wstring logDir;
	std::wstring logDirDOSPath;

	// The FileMap DSA containing name of files under Log Dir 
	//std::map<std::wstring, bool> fileMap;
	//std::map<std::wstring, bool>::iterator it;

	// Thread-specific parameters
	bool watcherRunning;
	Thread* watcherThread;
	HANDLE hStopRunning;

};