#pragma once
#include <list>
#include <xstring>
#include <Windows.h>

class MemoFile
{

public:
	struct slice
	{
		slice(): offset(0), length(0), pageID(0), buffer(NULL), overwrite( false ){}
		__int64 offset;
		__int64 length;
		unsigned int pageID;
		PBYTE buffer;
		bool overwrite;
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
	inline std::list<slice>::iterator findPositionInVector( __int64 startPos );
private:
	MemoFile( const MemoFile& );
	MemoFile& operator=( const MemoFile& );
	
	inline void tryCompresion( );
	inline void tryPageCompresion( );
	__int64 insertOverWrite( __int64 startPos, PBYTE buffer, __int64 size );
	__int64 insertNoOverWrite( __int64 startPost, PBYTE buffer, __int64 size );

	std::list<slice> m_VecChanges;
	bool			 m_Compress;
	bool			 m_PageCompress;
	__int64          m_CommitSize;
};

