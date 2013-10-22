#pragma once
#include <WTypes.h>

class ErrorLoger
{
public:
	static void ErrorMessage( LPTSTR lpszFunction );
private:
	ErrorLoger(void);
	~ErrorLoger(void);
};

