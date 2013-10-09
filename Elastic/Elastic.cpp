// Elastic.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction) 
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
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}
int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hFile;

	 hFile = ::CreateFile( TEXT("D:\\Dev\\What.the.Bleep!.Down.the.Rabbit.Hole.mkv"), //CreateFile(TEXT("D:\\Video\\What.the.Bleep!.Down.the.Rabbit.Hole.mkv"), // open One.txt
		    GENERIC_READ | GENERIC_WRITE,             // open for reading
            0,                        // do not share
            NULL,                     // no security
            OPEN_ALWAYS ,            // existing file only
            FILE_ATTRIBUTE_NORMAL,    // normal file
            NULL);
	 // no attr. template

	 if (hFile == INVALID_HANDLE_VALUE)
	 {
		 printf("Could not open One.txt."); 
		 return 0;
	 }

	 LARGE_INTEGER lint2 = {0};
	 LARGE_INTEGER result;
	 lint2.QuadPart = 11595;
	 //if( ::SetFilePointerEx( hFile, lint2, &result, FILE_BEGIN ) )
	 //{
		// if( ::SetEndOfFile( hFile ) )
		// {
		//	 printf("File size increasead " );
		// }
		// else
		// {
		//	ErrorExit(L"SetEndOfFile ");
		// }
	 //}

	 HANDLE hMemMap = ::CreateFileMappingW( hFile, 0, PAGE_READWRITE, 0, 0, 0 );
	 LARGE_INTEGER lint = {0};
	 BOOL bval = ::GetFileSizeEx( hFile, &lint );

	 _SYSTEM_INFO sysInfo ={0};
	 ::GetSystemInfo( &sysInfo );
	 __int64 fileSize = lint.QuadPart;
	 DWORD dwView = sysInfo.dwAllocationGranularity *2;
	 //BYTE *pbuf = (BYTE*)::MapViewOfFile( hMemMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );

	 for( __int64 index = 0; index < fileSize; index += dwView )
	 {
		 DWORD high = static_cast<DWORD>((index >> 32) & 0xFFFFFFFFul);
		 DWORD low  = static_cast<DWORD>( index        & 0xFFFFFFFFul);
		 if( index + dwView > fileSize )
		 {
			 dwView = static_cast<DWORD>(fileSize - index);
		 }
		 BYTE *pbuf = (BYTE*)::MapViewOfFile( hMemMap, FILE_MAP_READ | FILE_MAP_WRITE, high, low, dwView );
	 }
	 printf(" Done \n" );
	 //pbuf[0] = '^';
	 //pbuf[1] = '&';
	 //pbuf[4095] = '^';
	 //pbuf[11594] = '#';
	// memcpy( pbuf + 11595, "HELLO WORD ", 4*1024 );
	 //::UnmapViewOfFile( pbuf );
	 ::CloseHandle( hMemMap );
	 ::CloseHandle(hFile);
	return 0;
}

