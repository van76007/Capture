#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <string>

void _tmain(int argc, TCHAR* argv[])
{
   WIN32_FIND_DATA FileData; 
   HANDLE hSearch; 
   DWORD dwAttrs;   
   TCHAR szNewPath[MAX_PATH];   
 
   BOOL fFinished = FALSE; 

   if(argc != 2)
   {
      _tprintf(TEXT("Usage: %s <dir>\n"), argv[0]);
      return;
   }
 
// Create a new directory. 
	/*
   if (!CreateDirectory(argv[1], NULL)) 
   { 
      printf("CreateDirectory failed (%d)\n", GetLastError()); 
      return;
   } else
	   printf("CreateDirectory ok (%s)\n", argv[1]);
*/

 // Create a directory in a hard-code way
 std::wstring monitoredDrive = L"E:"; // TBA
 std::wstring monitoredVolume = L"\\Device\HarddiskVolume2"; // TBA
 std::wstring logDir = L"logDir"; // TBA
 std::wstring logDirPath = L"";
 WCHAR wszLogDirectory[1024];

 //GetFullPathName(logDir.c_str(), 1024, wszLogDirectory, NULL);
 CreateDirectory(logDir.c_str(), NULL);
 
 //
 std::wstring port = L"8080";
 int value = _wtoi(port.c_str());
 printf( "Function: _wtoi( \"%ls\" ) = %d\n", port.c_str(), value );

 // Combine the string the second way
 /*
 monitoredDrive += L"\\";
 monitoredDrive += logDir.c_str();
 */
 logDirPath = monitoredDrive; 
 logDirPath += L"\\";
 logDirPath += logDir.c_str();

 // With or without it is not important
 logDirPath += L"\\";
 
 // logDirPath = L"E:\\logDir"
 CreateDirectory(logDirPath.c_str(), NULL);
 GetFullPathName(logDirPath.c_str(), 1024, wszLogDirectory, NULL);
 
 //std::wstring logDirP = L"E:\\Logs";
 std::wstring logDirP = logDirPath;
 printf("logDirPath first is (%ls)\n", logDirP.c_str());
 
 
 size_t position = logDirP.rfind(monitoredDrive.c_str(),0);
 if(position != std::wstring::npos)
 {
	 logDirP.replace(position, monitoredDrive.length(), monitoredVolume.c_str(), 0, monitoredVolume.length());
	 printf("logDirPath is (%ls)\n", logDirP.c_str());
 } else
	printf("Wrong \n");

std::wstring logDirP1 = L"E:\\Logs";
std::wstring logDirP2 = L"E:\\Logs";

if (logDirP1 == logDirP2)
{
	printf("Equal \n");
} else
	printf("NotEqual \n");
// List all files under a directory


/*	   
// Start searching for text files in the current directory. 
 
   hSearch = FindFirstFile(TEXT("*.txt"), &FileData); 
   if (hSearch == INVALID_HANDLE_VALUE) 
   { 
      printf("No text files found.\n"); 
      return;
   } 
 
// Copy each .TXT file to the new directory 
// and change it to read only, if not already. 
 
   while (!fFinished) 
   { 
      StringCchPrintf(szNewPath, MAX_PATH, TEXT("%s\\%s"), argv[1], FileData.cFileName);

      if (CopyFile(FileData.cFileName, szNewPath, FALSE))
      { 
         dwAttrs = GetFileAttributes(FileData.cFileName); 
         if (dwAttrs==INVALID_FILE_ATTRIBUTES) return; 

         if (!(dwAttrs & FILE_ATTRIBUTE_READONLY)) 
         { 
            SetFileAttributes(szNewPath, 
                dwAttrs | FILE_ATTRIBUTE_READONLY); 
         } 
      } 
      else 
      { 
         printf("Could not copy file.\n"); 
         return;
      } 
 
      if (!FindNextFile(hSearch, &FileData)) 
      {
         if (GetLastError() == ERROR_NO_MORE_FILES) 
         { 
            _tprintf(TEXT("Copied *.txt to %s\n"), argv[1]); 
            fFinished = TRUE; 
         } 
         else 
         { 
            printf("Could not find next file.\n"); 
            return;
         } 
      }
   } 
 
// Close the search handle. 
 
   FindClose(hSearch);
   */
}
