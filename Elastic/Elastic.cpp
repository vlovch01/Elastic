// Elastic.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "ElasticFile.h"
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <conio.h>

int _tmain(int argc, _TCHAR* argv[])
{
	__int64 m = 0;
	__int64 M = 0;
	__int64 N = 0;
	std::string alpha="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::string number = "0123456789";

	std::wstring strFileName(L"two.txt");
	ElasticFile elasticFile;

	std::cout<<"Input file Name : ";
	std::wcin>>strFileName;
	std::cout<<std::endl;
	std::cout<<"Input first pass number :";
	std::cin>>m;
	std::cout<<std::endl;
	std::cout<<"Input second pass number :";
	std::cin>>M;
	std::cout<<"Input number of iterations :";
	std::cin>>N;

	srand( time( NULL ) );
	HANDLE hFile = elasticFile.FileOpen(strFileName, ElasticFile::Create );
	
	//ElasticFile::ustring strRand;
	//for( __int64 index = 1; index <= N; ++index )
	//{
	//	for( __int64 j = 1; j <= m; ++j )
	//	{
	//		unsigned short randNumber = rand() % alpha.size();
	//		strRand.push_back( alpha[randNumber] );
	//	}
	//	//std::cout<<strRand.c_str()<<std::endl;
	//	elasticFile.FileWrite( hFile, static_cast<PBYTE>(const_cast<unsigned char*>(strRand.c_str() ) ), m, false );
	//	strRand.clear();
	//}
	//
	//std::cout<<std::endl<<"Numbers "<<std::endl;
	//strRand.clear();
	//for( __int64 index = 1; index <= N; ++index )
	//{
	//	
	//	for( __int64 j = 1; j <= M; ++j )
	//	{
	//		unsigned short randNumber = rand() % number.size();
	//		strRand.push_back( number[ randNumber ] );
	//	}
	//	elasticFile.FileSetCursor( hFile, index * m + ( index - 1 ) * M, ElasticFile::Begin );
	//	elasticFile.FileWrite( hFile, static_cast<PBYTE>( const_cast<unsigned char*>( strRand.c_str() ) ), M, false );
	//	//std::cout<<strRand.c_str()<<std::endl;
	//	strRand.clear();
	//}


	//elasticFile.FileSetCursor( hFile, 15, ElasticFile::Begin );
	//BYTE buffer[21] = "0011223344556677";
	//elasticFile.FileWrite( hFile, buffer, 16, true );
	//elasticFile.FileSetCursor( hFile, 5, ElasticFile::Begin );
	//BYTE buff[21] = "!!@@##$$%%^^&&";
	//elasticFile.FileWrite( hFile, buff, 14, true );
	//elasticFile.FileSetCursor( hFile, 16 * 1024, ElasticFile::CursorMoveMode::End );
	//elasticFile.FileSetCursor( hFile, 3 * 1024 * 1024, ElasticFile::Begin );

	SYSTEM_INFO sSysInfo;         // Useful information about the system

    GetSystemInfo(&sSysInfo);     // Initialize the structure.
	_tprintf (TEXT("This computer has page size %d.\n"), sSysInfo.dwPageSize);
	DWORD dwPageSize = sSysInfo.dwAllocationGranularity;
	std::vector<PBYTE> vPages;
	BYTE pByteMsg[] = "HELLO WORKD TRALA LAL ";

	for( unsigned long i = 0; i <= 1024 * 1024; ++i )
	{
		PBYTE pByte = (PBYTE)::VirtualAlloc( NULL, dwPageSize * 64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
		if( !pByte )
		{
			_tprintf(TEXT("Error! VirtualAlloc reserve failed with error code of %ld.\n"), GetLastError ());
			break;
		}
		else
		{
			//memcpy( pByte, pByteMsg, strlen( (char*)pByteMsg ) * sizeof( PBYTE ) );
			vPages.push_back( pByte );
		}
	}

	_getch();

	for( unsigned int i = 0; i <= vPages.size() - 1; ++i )
	{
		::VirtualFree( vPages[i], 0, MEM_FREE );
	}

	return 0;
}

