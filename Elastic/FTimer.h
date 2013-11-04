#pragma once
#include <WTypes.h>

class FTimer
{
public:
	FTimer(void);
	~FTimer(void);
	void start();
	void endAndPrint( LPTSTR lpszText );

private:
	FTimer( const FTimer& );
	FTimer& operator=( const FTimer& );
	FTimer( const FTimer&& );
	FTimer& operator=( const FTimer&& );
private:
	LARGE_INTEGER m_begin;
	LARGE_INTEGER m_frequency;
};

