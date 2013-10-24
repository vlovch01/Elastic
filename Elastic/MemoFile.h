#pragma once
#include <list>
#include <xstring>
#include <Windows.h>

class MemoFile
{

public:
	typedef std::basic_string <unsigned char> ustring;
	struct slice
	{
		slice(): pageID(0), buffer(NULL){}
		__int64 offset;
		__int64 length;
		ustring ustrbuffer;
		unsigned int pageID;
		PBYTE buffer;
	};
public:
	MemoFile( __int64 startPos, __int64 size );
	~MemoFile(void);

	__int64 read    ( __int64 startPos, PBYTE buffer, __int64 size );
	__int64 insert  ( __int64 startPos, PBYTE buffer, __int64 size, bool overwrite );
	bool truncate   ( __int64 startPos, __int64 size );
	void inscrease  ( __int64  size );

	const std::list<slice>& getChanges()const { return m_VecChanges; }
	void swapContainer( __int64 startPos, __int64 size );
private:
	MemoFile( const MemoFile& );
	MemoFile& operator=( const MemoFile& );
	inline std::list<slice>::iterator findPositionInVector( __int64 startPos );

	__int64 insertOverWrite( __int64 startPos, PBYTE buffer, __int64 size );
	__int64 insertNoOverWrite( __int64 startPost, PBYTE buffer, __int64 size );

	std::list<slice> m_VecChanges;
	__int64 m_CommitSize;
};

