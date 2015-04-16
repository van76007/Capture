#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

void ErrorHandler(LPTSTR lpszFunction);

int _tmain(int argc, TCHAR *argv[])
{
   WIN32_FIND_DATA ffd;
   LARGE_INTEGER filesize;
   FILETIME ftWrite;

   TCHAR szDir[MAX_PATH];
   size_t length_of_arg;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   //
   /*
   FILETIME ftCreate, ftAccess, ftWrite;
   SYSTEMTIME stUTC, stLocal;
   DWORD dwRet;

    // Convert the last-write time to local time.
    FileTimeToSystemTime(&ftWrite, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    dwRet = StringCchPrintf(lpszString, dwSize, 
        TEXT("%02d/%02d/%d  %02d:%02d"),
        stLocal.wMonth, stLocal.wDay, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute);
	*/
   
   // If the directory is not specified as a command-line argument,
   // print usage.

   if(argc != 2)
   {
      _tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
      return (-1);
   }

   // Check that the input path plus 2 is not longer than MAX_PATH.

   StringCchLength(argv[1], MAX_PATH, &length_of_arg);

   if (length_of_arg > (MAX_PATH - 2))
   {
      _tprintf(TEXT("\nDirectory path is too long.\n"));
      return (-1);
   }

   _tprintf(TEXT("\nTarget directory is %s\n\n"), argv[1]);

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   StringCchCopy(szDir, MAX_PATH, argv[1]);
   StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      ErrorHandler(TEXT("FindFirstFile"));
      return dwError;
   } 
   
   // List all the files in the directory with some info about them.

   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
		// List all dirs inside the directory
         _tprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
      }
      else
      {
         // List all files inside the directory
		 filesize.LowPart = ffd.nFileSizeLow;
         filesize.HighPart = ffd.nFileSizeHigh;
		 ftWrite = ffd.ftLastWriteTime;
		 //
         _tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   dwError = GetLastError();
   if (dwError != ERROR_NO_MORE_FILES) 
   {
      ErrorHandler(TEXT("FindFirstFile"));
   }

   FindClose(hFind);
   return dwError;
}


void ErrorHandler(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
