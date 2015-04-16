#include <iostream>
#include <tchar.h>
#include "Global.h"
#include "Watcher.h"
#include "Monitor.h"

/* Initialise static variables. These are all singletons */
/*
// Process manager which keeps track of opened processes and their paths
bool ProcessManager::instanceCreated = false;
ProcessManager* ProcessManager::processManager = NULL;

// Objects listen for internal events from this controller
bool EventController::instanceCreated = false;
EventController* EventController::pEventController = NULL;

// Logging capabilities that either writes to a file or console
bool Logger::instanceCreated = false;
Logger* Logger::logger = NULL;

// Global options available to all objects
bool OptionsManager::instanceCreated = false;
OptionsManager* OptionsManager::optionsManager = NULL;
*/

/* Global default parameters available to all objects */
std::wstring serverIp = L"192.168.64.132";
std::wstring serverPort = L"8080";
// Those parameters could be passed by the .inf file. But no longer read from the reg
// Decision: - Create logDir in the same driver as monitoredDrive.
//			 - Exclude all write events to this logDir

std::wstring monitoredDrive = L""; // TBA
std::wstring logDir = L"logs"; // TBA

class MonitorClient : public Runnable
{
public:
	/* Constructor */
	MonitorClient()
	{
		printf("MonitorClient initializing ...\n");
		hStopRunning = CreateEvent(NULL, FALSE, FALSE, NULL);

		/* Capture needs the SeDriverLoadPrivilege and SeDebugPrivilege to correctly run.
		   If it doesn't acquire these privileges it exits. DebugPrivilege is used in the
		   ProcessManager to open handles to existing process. DriverLoadPrivilege allows
		   Capture to load the kernel drivers. */
		if(!obtainDriverLoadPrivilege())
		{	
			printf("\n\nCould not acquire privileges. Make sure you are running Application with Administrator rights\n");
			exit(1);
		}
		
		/* Initialize depending objects */
		int serverPortInt = _wtoi(serverPort.c_str());
		//
		printf( "Convert server port string ( \"%ls\" ) = %d\n", serverPort.c_str(), serverPortInt );
		
		// Start monitoring the log Dir
		watcher = new Watcher(serverIp, serverPortInt, monitoredDrive, logDir);
		//watcher->start();
		// Start installing mini-filter
		monitor = new Monitor(monitoredDrive, logDir);
		// Feb 16.TBA: Why should not put here
		//monitor->start();
		
		/* Start running this thread */
		Thread* monitorClientThread = new Thread(this);
		monitorClientThread->start("MonitorClient");
		printf("MonitorClient initialized\n");
	}
	
	/* Destructor */
	~MonitorClient()
	{
		printf("MonitorClient ceasing ...\n");
		
		/* Remove handle and depending objects */
		CloseHandle(hStopRunning);
		//
		delete watcher;
		delete monitor;

		printf("MonitorClient ceased\n");
	}

	/* Get the driver load privilege so that the kernel drivers can be loaded */
	bool obtainDriverLoadPrivilege()
	{
		printf("MonitorClient::obtainDriverLoadPrivilege() start\n");
		HANDLE hToken;
		if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;
			LPCTSTR lpszPrivilege = L"SeLoadDriverPrivilege";

			if ( !LookupPrivilegeValue( 
					NULL,            // lookup privilege on local system
					lpszPrivilege,   // privilege to lookup 
					&luid ) )        // receives LUID of privilege
			{
				printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
				return false; 
			}

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			// Enable the privilege or disable all privileges.
			if ( !AdjustTokenPrivileges(
				hToken, 
				FALSE, 
				&tp, 
				sizeof(TOKEN_PRIVILEGES), 
				(PTOKEN_PRIVILEGES) NULL, 
				(PDWORD) NULL) )
			{ 
			  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
			  return false; 
			} 

			if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
			{	
				printf("The token does not have the specified privilege. \n");
				return false;
			} 
		}

		CloseHandle(hToken);
		printf("CaptureClient::obtainDriverLoadPrivilege() end3\n");
		return true;
	}

	/* Main loop */
	void run()
	{
		printf("MonitorClient running ...\n");
		
		try 
		{
			
			/* Attemp to connect to the Log server. Always true */
			if(watcher->connectToServer(true))
			{
				watcher->start();

				// Feb 16. Must put here
				monitor->start();
				/* Wait till a user presses a key then exit */
				
				if( getchar() == '\n')
				{
					monitor->stop();
					watcher->stop();
				}
			}
			
			/* Set handle of this thread */
			SetEvent(hStopRunning);
			printf("MonitotClient: After SetEvent(hStopRunning) \n");
		} catch (...) {
			printf("MonitotClient::run exception\n");	
			throw;
		}

		printf("MonitorClient stop running\n");
	}

	/* Handle of this thread */
	HANDLE hStopRunning;

private:
	Watcher* watcher;
	Monitor* monitor;	
};

int _tmain(int argc, WCHAR* argv[])
{
	/* Print some user's guide */
	printf("MonitorClient::main start\n");
	printf("PROJECT: Capture Malwares\n");
    printf("VERSION: 1.0\n");
    printf("DATE: Feb 10, 2009\n");
    printf("COPYRIGHT HOLDER: CSIS ApS, DK\n");
    printf("AUTHORS:\n");
    printf("\tDinh Van Vu (dvv@csis.dk)\n");
	printf("\tNumber of argument %d\n", argc);

	if (argc == 1)
	{
		printf("\n\nUsage: MonitorClient.exe [-h] [-d monitored drive] [-l log dir] [-a server address] [-p server port]\n");
		printf("\n  -h\t\tPrint this help message");
		printf("\n  -d drive\tDriver to be monitored");
		printf("\n  -a address\tAddress of the server the client connects up to");
		printf("\n  -p port\tPort the server user the client connects up to");
		printf("\n  -l\t\tCopy temp files into the log directory when they are modified or\n\t\tdeleted");
		printf("\n  -s\t\tStop the program");
		exit(1);
	}

	/* Get the parameters from the command line */
	for(int i = 1; i < argc; i++)
	{
		std::wstring option = argv[i];

		if(option == L"--help" || option == L"-h") {
			printf("\n\nUsage: MonitorClient.exe [-h] [-d monitored drive] [-l log dir] [-a server address] [-p server port]\n");
			printf("\n  -h\t\tPrint this help message");
			printf("\n  -d drive\tDriver to be monitored");
			printf("\n  -a address\tAddress of the server the client connects up to");
			printf("\n  -p port\tPort the server user the client connects up to");
			printf("\n  -l\t\tCopy temp files into the log directory when they are modified or\n\t\tdeleted");
			printf("\n  -s\t\tStop the program");
			exit(1);
		} else if(option == L"-a") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				serverIp = argv[++i];
				printf("Option: Connect to server ip: %ls\n", serverIp.c_str());
			}
		} else if(option == L"-p") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				serverPort = argv[++i];
				printf("Option: Connect to server port: %ls\n", serverPort.c_str());
			}
		} else if(option == L"-l") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				// logDirectory
				logDir = argv[++i];
				printf("Option: Logging temp files to %ls\n", logDir.c_str());
			}
		} else if(option == L"-d") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				// Name of Drive to be Monitored
				monitoredDrive = argv[++i];
				printf("Option: monitoredDrive is %ls\n", monitoredDrive.c_str());
			}
		}
	}

	/* Start the main program thread */
	MonitorClient* cp = new MonitorClient();
	WaitForSingleObject(cp->hStopRunning, INFINITE);

	/* Exit the main program thread */
	delete cp;
	printf("MonitorClient::main end\n");
	return 0;
}
