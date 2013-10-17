#include "MemoFile.h"
#include <algorithm>


MemoFile::MemoFile( __int64 startPos, ULONG size )
{
	slice first;
	first.length = size;
	first.offset = startPos;
	m_VecChanges.push_back( std::move( first ) );
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
	if( overwrite )
	{
		insertOverWrite( startPos, buffer, size );
	}
	else
	{
		insertNoOverWrite( startPos, buffer, size );
	}

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

ULONG MemoFile::insertOverWrite( __int64 startPos, PBYTE buffer, ULONG size )
{
	std::vector<slice>::iterator it    = findPositionInVector( startPos );

	if( it == m_VecChanges.end() )
	{
		return 0;
	}

		std::vector<slice>::iterator itEnd;
		if( (*it).offset + (*it).length >= startPos + size && (*it).offset <= startPos )
		{
			itEnd = it;
		}
		else
		{
			itEnd = findPositionInVector( startPos + size );
		}

		ustring strBuf( buffer );

			if( (*it).offset == startPos )
			{
				(*it).length = size;
				(*it).ustrbuffer.swap( std::move(strBuf) );

				++it; ++itEnd;
				m_VecChanges.erase( std::remove( it, itEnd, [&startPos, &size]( const slice &obj )
					{
						return obj.offset > startPos && obj.offset + obj.length <= startPos + size; 
					}), m_VecChanges.end() );
			}
			else
			{
				(*it).ustrbuffer.erase( startPos - (*it).offset, (*it).length );
				(*it).length = startPos - (*it).offset;

				slice Item;
				Item.offset = startPos;
				Item.length = size;
				Item.ustrbuffer.swap( std::move( strBuf ) );

			}

			if( it != itEnd )
			{
				ustring strEnd( (*it).ustrbuffer[startPos + size - (*it).offset], (*it).ustrbuffer[(*it).length] ); 
				(*itEnd).length = (*itEnd).offset + (*itEnd).length - ( startPos + size + 1 );
				(*itEnd).offset = startPos + size + 1;
				(*itEnd).ustrbuffer.swap( std::move( strEnd ) );
			}

	return 0;
}
ULONG MemoFile::insertNoOverWrite( __int64 startPos, PBYTE buffer, ULONG size )
{
	std::vector<slice>::iterator it = findPositionInVector( startPos );
	return 0;
}