#include "FTimer.h"
#include <iostream>
#include <string>
#include <Windows.h>

FTimer::FTimer(void)
{
	::QueryPerformanceFrequency( &m_frequency );
}


FTimer::~FTimer(void)
{
}

void FTimer::start()
{
	::QueryPerformanceCounter( &m_begin );
}

void FTimer::endAndPrint( LPTSTR lpszText )
{
	LARGE_INTEGER stopTime;
	
	::QueryPerformanceCounter( &stopTime );
	std::wcout<<lpszText<<std::endl;
	double result = double( stopTime.QuadPart - m_begin.QuadPart ) / m_frequency.QuadPart;
	std::cout<<"Time in miliseconds: "<<result * 1000<<std::endl;
	std::cout<<"Time in seconds: "<<result<<std::endl;
	std::cout<<"Time in minutes: "<<result / 60<<std::endl;
}