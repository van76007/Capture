#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <string>

#define TOTALBYTES    8192
#define BYTEINCREMENT 4096

void main()
{
    DWORD BufferSize = TOTALBYTES;
    DWORD cbData;
    DWORD dwRet;

    PPERF_DATA_BLOCK PerfData = (PPERF_DATA_BLOCK) malloc( BufferSize );
    cbData = BufferSize;
	
	std::wstring logDir = L"logs"; // TBA

	printf("\nRetrieving the data...");
	
	/*
    dwRet = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                             TEXT("Global"),
                             NULL,
                             NULL,
                             (LPBYTE) PerfData,
                             &cbData );
    while( dwRet == ERROR_MORE_DATA )
    {
        // Get a buffer that is big enough.

        BufferSize += BYTEINCREMENT;
        PerfData = (PPERF_DATA_BLOCK) realloc( PerfData, BufferSize );
        cbData = BufferSize;

        printf(".");
        dwRet = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                         TEXT("Global"),
                         NULL,
                         NULL,
                         (LPBYTE) PerfData,
                         &cbData );
    }
    if( dwRet == ERROR_SUCCESS )
        printf("\n\nFinal buffer size is %d\n", BufferSize);
    else printf("\nRegQueryValueEx failed (%d)\n", dwRet);
	*/

	
	HKEY keyHandle;
    char rgValue [1024];
    char fnlRes [1024];
    DWORD size1;
    DWORD Type;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
        //TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),0,
		TEXT("SYSTEM\\CurrentControlSet\\Services\\Minispy"),0,
        KEY_QUERY_VALUE, &keyHandle) == ERROR_SUCCESS)
         {
            printf("\nGot the data...");
			 size1=1023;
			
            RegQueryValueEx( keyHandle, TEXT("LogFolder"), NULL, &Type, 
                //(LPBYTE)rgValue,&size1)
                (LPBYTE) PerfData, &cbData );

            printf("Value is: %s\n", PerfData);
			
         }     
    else printf("\nCouldn't access system information!");

        RegCloseKey(keyHandle);

}