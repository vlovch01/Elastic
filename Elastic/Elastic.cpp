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
	unsigned short randNumber = 0;

	elasticFile.FileSetCursor( hFile, 0, ElasticFile::End );
	__int64 fileSize = elasticFile.FileGetCursor( hFile );
	elasticFile.FileSetCursor( hFile, 0, ElasticFile::Begin );
	elasticFile.FileTruncate( hFile, fileSize / 4 );
	PBYTE pbyte = new BYTE[ m + 1];
	for( __int64 index = 1; index <= N; ++index )
	{
		for( __int64 j = 0; j < m; ++j )
		{
			randNumber = rand() % alpha.size();
			pbyte[j] = alpha[randNumber];
		}
		//std::cout<<strRand.c_str()<<std::endl;
		elasticFile.FileWrite( hFile, pbyte, m, false );
	}
	
	std::cout<<std::endl<<"Numbers "<<std::endl;
	delete []pbyte;

	pbyte = new BYTE[ M + 1];
	for( __int64 index = 1; index <= N; ++index )
	{
		
		for( __int64 j = 1; j <= M; ++j )
		{
			randNumber = rand() % number.size();
			pbyte[j-1] = number[ randNumber ];
		}
		elasticFile.FileSetCursor( hFile, index * m + ( index - 1 ) * M, ElasticFile::Begin );
		elasticFile.FileWrite( hFile, pbyte, M, false );
		//std::cout<<strRand.c_str()<<std::endl;
	}
	delete []pbyte;


	//elasticFile.FileSetCursor( hFile, 15, ElasticFile::Begin );
	//BYTE buffer[21] = "0011223344556677";
	//elasticFile.FileWrite( hFile, buffer, 16, true );
	//elasticFile.FileSetCursor( hFile, 5, ElasticFile::Begin );
	//BYTE buff[21] = "!!@@##$$%%^^&&";
	//elasticFile.FileWrite( hFile, buff, 14, true );
	//elasticFile.FileSetCursor( hFile, 16 * 1024, ElasticFile::CursorMoveMode::End );
	//elasticFile.FileSetCursor( hFile, 3 * 1024 * 1024, ElasticFile::Begin );
	std::cout<<"DONE!!!"<<std::endl;
	_getch();

	return 0;
}

