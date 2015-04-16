/*
 *	PROJECT: Capture
 *	FILE: Analyzer.cpp
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "Watcher.h"

/*
#include "ServerReceive.h"
#include <boost/bind.hpp>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
*/

Watcher::Watcher(const std::wstring& sAddress, int p, const std::wstring& mDrive, const std::wstring& lDir)
{
	// Initialize the local variables
	watcherRunning = false;

	// Get the parameters
	serverAddress = sAddress;
	port = p;
	monitoredDrive = mDrive;
	logDir = lDir;
	// Create the logDirDOSPath
	logDirDOSPath = mDrive;
	logDirDOSPath += L"\\";
	logDirDOSPath += logDir.c_str();

	// Initialize the TFTPClient daemon
	TFTPClient::Startup();
	tftpClient = new TFTPClient(sAddress, p);
}

Watcher::~Watcher()
{
	/*
	delete serverReceive;
	WSACleanup();
	disconnectFromServer();
	*/
	stop();
	
	/* Remove handle and depending objects */
	CloseHandle(hStopRunning);
	
	//
	delete tftpClient;
	TFTPClient::Cleanup();
}

bool
Watcher::connectToServer(bool startTFTPClient)
{
	printf("Watcher ALWAYS connected to Server \n");
	return true;
}

/*
Starting monitoring the changes in logDir
*/
void
Watcher::start()
{
	printf("Watcher starting ... \n");

	if(!isWatcherRunning())
	{
		watcherThread = new Thread(this);
		watcherThread->start("Watching_Log_Dir");
	}
}

void
Watcher::stop()
{
	printf("Watcher stops \n");
	
	// TBA
	if(isWatcherRunning())
	{
		watcherRunning = false;

		// Van: Tech to terminate threads timely
		WaitForSingleObject(hStopRunning, WAIT_STOPPING_OBJECT);
		
		printf("Watcher::stop() stopping thread.\n");
		watcherThread->stop();
		
		DWORD dwWaitResult;
		dwWaitResult = watcherThread->wait(WAIT_KILLING_THREAD);

		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            printf("Watcher::stop() stopped watcherThread.\n");
			break;
		case WAIT_TIMEOUT:
			printf("Watcher::stop() stopping watcherThread timed out. Attempting to terminate.\n");
			watcherThread->terminate();
			printf("Watcher::stop() terminated watcherThread.\n");
			break;
        // An error occurred
        default: 
            printf("Watcher stopping watcherThread failed (%d)\n", GetLastError());
		} 
		
		
		// Delete this thread
		delete watcherThread;
	}
}

/* Main loop */
void 
Watcher::run()
{
	printf("Watcher is running ...\n");
	
	//
	watcherRunning = true;

	DWORD refreshError = 0;
	

    printf("Watcher monitoring logDir %ls\n", logDirDOSPath.c_str());
	
	// MUST Sleep(REFRESH_DIR_INTERVAL) before refeshing the list of files under the logDir
	while(!watcherThread->shouldStop() && isWatcherRunning())
	{
		// Refresh the list of files under log Dir
		refreshError = RefreshDirectory(logDirDOSPath);
		
		// Sleep for some interval
		Sleep(REFRESH_DIR_INTERVAL);
	}

	/* Set handle of this thread */
	SetEvent(hStopRunning);

	printf("Watcher stopped running\n");
}

// List all the files under a directory
int
Watcher::RefreshDirectory(const std::wstring& lDirPath)
{
	printf("Watcher is refreshing logDir ...\n");

	// Must have this one to browse the files under a directory
	std::wstring logDirDOSPathStar = lDirPath;
	logDirDOSPathStar += L"\\*";

	std::wstring fileNamePrefix = lDirPath;
	fileNamePrefix += L"\\";

	std::wstring fileName;

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = -1;
	int sendOK = TFTP_FILE_SENT;

	printf("monitoring logDir %ls\n", lDirPath.c_str());
	hFind = FindFirstFile(logDirDOSPathStar.c_str(), &ffd);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
	  
	  printf("INVALID_HANDLE\n");
      return dwError;
	}
	
	do
   {
	  if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
		
      }
      else
      {
         // List all files inside the directory
		 
		 // Get Full path to a logFileName
		 printf("Add file %ls\n", (std::wstring(ffd.cFileName)).c_str());
		 /*
		 wchar_t* szLogFileName = new wchar_t[1024];
		 GetFullPathName(fileNamePrefix+std::wstring(ffd.cFileName).c_str(), 1024, szLogFileName, NULL);
		 */
		 /*
		 std::wstring text(L"hello");
		const wchar_t* text_ptr = text.c_str();
		std::vector<wchar_t> buffer(text_ptr, text_ptr + text.size() + 1);
		wchar_t* buffer_ptr = &buffer[0];
		*/
		

		 // Send the file
		 /*
		 fileName = fileNamePrefix+std::wstring(ffd.cFileName);
		 sendOK = tftpClient -> sendFile(fileName);
		 */
		 fileName = std::wstring(ffd.cFileName);
		 sendOK = tftpClient -> sendFile(fileNamePrefix+fileName, fileName);

		 if ( sendOK == TFTP_FILE_SENT )
		 {
			printf("Watcher deletes file %ls\n", (std::wstring(ffd.cFileName)).c_str());
			//DeleteFile(szLogFileName);
			DeleteFile((fileNamePrefix+fileName).c_str());
		 }

		 // Cleanup
		 //delete [] szLogFileName;

		 // Continue with the next file
      }
   } while (FindNextFile(hFind, &ffd) != 0);
   
   // Free up the resource 
   FindClose(hFind);
   return 0;
}