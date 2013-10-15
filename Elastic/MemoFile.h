#pragma once
#include <vector>
#include <xstring>
#include <Windows.h>

class MemoFile
{

public:
	typedef std::basic_string <unsigned char> ustring;
	struct slice
	{
		__int64 offset;
		__int64 length;
		ustring ustrbuffer;
	};
public:
	MemoFile(void);
	~MemoFile(void);

	ULONG read   ( __int64 startPos, PBYTE buffer, ULONG size );
	ULONG insert  ( __int64 startPos, PBYTE buffer, ULONG size, bool overwrite );
	bool truncate( __int64 startPos, ULONG size );
private:
	MemoFile( const MemoFile& );
	MemoFile& operator=( const MemoFile& );
	inline std::vector<slice>::iterator findPositionInVector( __int64 startPos );

	std::vector<slice> m_VecChanges;
	__int64 m_CommitSize;
};

