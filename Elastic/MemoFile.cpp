#include "MemoFile.h"
#include <algorithm>

MemoFile::MemoFile(void)
{
}


MemoFile::~MemoFile(void)
{
}

ULONG MemoFile::read   ( __int64 startPos, PBYTE buffer, ULONG size )
{

	return 0;
}

ULONG MemoFile::insert  ( __int64 startPos, PBYTE buffer, ULONG size, bool overwrite )
{
	std::vector<slice>::iterator it = findPositionInVector( startPos );
	return 0;
}

bool MemoFile::truncate( __int64 startPos, ULONG size )
{
	return false;
}


inline std::vector<MemoFile::slice>::iterator MemoFile::findPositionInVector( __int64 startPos )
{
	return std::lower_bound( m_VecChanges.begin(), m_VecChanges.end(), startPos, []( const slice& obj, __int64 val )
		{
			return obj.offset >= val && obj.offset + obj.length < val; 
		});
}