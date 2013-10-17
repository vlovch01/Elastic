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


inline std::list<MemoFile::slice>::iterator MemoFile::findPositionInVector( __int64 startPos )
{
	return std::lower_bound( m_VecChanges.begin(), m_VecChanges.end(), startPos, []( const slice& obj, __int64 val )
		{
			return obj.offset >= val && obj.offset + obj.length < val; 
		});
}

ULONG MemoFile::insertOverWrite( __int64 startPos, PBYTE buffer, ULONG size )
{
	std::list<slice>::iterator it    = findPositionInVector( startPos );

	if( it == m_VecChanges.end() )
	{
		return 0;
	}

		std::list<slice>::iterator itEnd;
		if( (*it).offset + (*it).length >= startPos + size && (*it).offset <= startPos )
		{
			itEnd = it;
		}
		else
		{
			itEnd = findPositionInVector( startPos + size );
		}
	
		if( itEnd == m_VecChanges.end() )
		{
			--itEnd;
			size = (*itEnd).offset + (*itEnd).length - (*it).offset;
		}
		ustring strBuf( buffer );
		ustring restBuff;
		if( !(*itEnd).ustrbuffer.empty() )
		{
			( (*itEnd).ustrbuffer.substr( startPos + size - (*itEnd).offset, (*itEnd).offset + (*itEnd).length - startPos - size) );
		}

			if( (*it).offset == startPos )
			{
				(*it).length = size;		
				(*it).ustrbuffer.swap( std::move(strBuf) );
				//case 3 and 4
				++it; ++itEnd;
			}
			else
			{
				(*it).ustrbuffer.erase( startPos - (*it).offset, (*it).length );
				(*it).length = startPos - (*it).offset;

				slice Item;
				Item.offset = startPos;
				Item.length = size;
				Item.ustrbuffer.swap( std::move( strBuf ) );
				it = m_VecChanges.insert( ++it, Item );
			}

			/*m_VecChanges.erase( std::remove( it, itEnd, [&startPos, &size]( const slice &obj )
			{
				return obj.offset > startPos && obj.offset + obj.length <= startPos + size; 
			}), m_VecChanges.end() );*/

			m_VecChanges.erase( it, itEnd );

			if( it != itEnd )
			{ 
				(*itEnd).length = (*itEnd).offset + (*itEnd).length - ( startPos + size );
				(*itEnd).offset = startPos + size + 1;
				(*itEnd).ustrbuffer.swap( std::move( restBuff ) ); 
			}
			else
			{
				//we need to insert new Item 
				slice Item;
				Item.length = (*itEnd).offset + (*itEnd).length - ( startPos + size );
				Item.offset = startPos + size + 1;
				Item.ustrbuffer.swap( std::move( restBuff ) );
				m_VecChanges.insert( ++it, Item );
			}

	return size;
}
ULONG MemoFile::insertNoOverWrite( __int64 startPos, PBYTE buffer, ULONG size )
{
	std::list<slice>::iterator it = findPositionInVector( startPos );

	return 0;
}