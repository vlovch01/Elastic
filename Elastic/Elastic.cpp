// Elastic.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "FTimer.h"
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
	FTimer fclock;

	std::cout<<"Input file Name : ";
	std::wcin>>strFileName;
	std::cout<<std::endl;
	std::cout<<"Input first pass number :";
	std::cin>>m;
	std::cout<<std::endl;
	std::cout<<"Input second pass number :";
	std::cin>>M;
	std::cout<<std::endl;
	std::cout<<"Input number of iterations :";
	std::cin>>N;

	srand( time( NULL ) );
	HANDLE hFile = elasticFile.FileOpen(strFileName, ElasticFile::Open );
	unsigned short randNumber = 0;

	elasticFile.FileSetCursor( hFile, 0, ElasticFile::End );
	__int64 fileSize = elasticFile.FileGetCursor( hFile );
	elasticFile.FileSetCursor( hFile, 0, ElasticFile::Begin );
	elasticFile.FileTruncate( hFile, fileSize / 4 );

	fclock.start();
	PBYTE pbyte = new BYTE[ m + 1];
	pbyte[ m ] ='\0';
	for( __int64 index = 1; index <= N; ++index )
	{
		for( __int64 j = 1; j <= m; ++j )
		{
			randNumber = rand() % alpha.size();
			pbyte[j - 1] = alpha[randNumber];
		}
		//std::cout<<pbyte<<std::endl;
		elasticFile.FileSetCursor( hFile, (index - 1) * m, ElasticFile::Begin ); 
		elasticFile.FileWrite( hFile, pbyte, m, false );
	}
	
	std::cout<<std::endl<<"Numbers "<<std::endl;
	delete []pbyte;

	fclock.endAndPrint(L"Inserted values ");
	fclock.start();
	pbyte = new BYTE[ M + 1];
	pbyte[M] = '\0';
	for( __int64 index = 1; index <= N; ++index )
	{
		
		for( __int64 j = 1; j <= M; ++j )
		{
			randNumber = rand() % number.size();
			pbyte[j - 1] = number[ randNumber ];
		}
		elasticFile.FileSetCursor( hFile, index * m + ( index - 1 ) * M, ElasticFile::Begin );
		elasticFile.FileWrite( hFile, pbyte, M, false );
		//std::cout<<pbyte<<std::endl;
	}
	delete []pbyte;

	fclock.endAndPrint(L"Inserstion of next values ");
	//elasticFile.FileSetCursor( hFile, 0, ElasticFile::Begin );
	//BYTE buffer[21] = "Hello World in File";//"0011223344556677";
	//elasticFile.FileWrite( hFile, buffer, 19, true );
	//elasticFile.FileSetCursor( hFile, 25, ElasticFile::Begin );
	//BYTE buff[21] = "**********";//"!!@@##$$%%^^&&";
	//elasticFile.FileWrite( hFile, buff, 10, true );
	//elasticFile.FileSetCursor( hFile, 0, ElasticFile::Begin );
	//BYTE retBuf[128] = "\0";
	//elasticFile.FileRead( hFile, retBuf, 40 );
	//std::cout<<retBuf<<std::endl;
//	elasticFile.FileSetCursor( hFile, 16 * 1024, ElasticFile::CursorMoveMode::End );
//	elasticFile.FileSetCursor( hFile, 3 * 1024 * 1024, ElasticFile::Begin );
	std::cout<<"DONE!!!"<<std::endl;
	_getch();

	return 0;
}

