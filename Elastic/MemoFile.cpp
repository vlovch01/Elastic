#include "MemoFile.h"
#include <algorithm>
#include <functional>

MemoFile::MemoFile( __int64 startPos, __int64 size )
{
	slice first;
	first.length = size;
	first.offset = startPos;
	m_VecChanges.push_back( std::move( first ) );
}


MemoFile::~MemoFile(void)
{
}

__int64 MemoFile::read   ( __int64 startPos, PBYTE buffer, __int64 size )
{

	return 0;
}

__int64 MemoFile::insert  ( __int64 startPos, PBYTE buffer, __int64 size, bool overwrite )
{
	if( overwrite )
	{
		return insertOverWrite( startPos, buffer, size );
	}
	else
	{
		return insertNoOverWrite( startPos, buffer, size );
	}	
}

bool MemoFile::truncate( __int64 startPos, __int64 size )
{
	std::list<slice>::iterator it    = findPositionInVector( startPos );
	std::list<slice>::iterator itEnd = findPositionInVector( startPos + size );

	if( it != itEnd )
	{
		if( (*it).offset != startPos )
		{
			__int64 lastPos =  startPos - (*it).offset;
			
			if( !(*it).ustrbuffer.empty() )
			{
				(*it).ustrbuffer.erase( lastPos + 1, (*it).offset + (*it).length - startPos );
			}
			(*it).length = lastPos;
			++it;
		}

		if( !(*itEnd).ustrbuffer.empty() )
		{
			(*itEnd).ustrbuffer.erase( 0, startPos + size - (*it).offset );
		}
		(*itEnd).length = (*itEnd).offset + (*itEnd).length - startPos - size;
		(*itEnd).offset = startPos + size + 1;

		it = m_VecChanges.erase( it, itEnd );
	}
	else
	{
		//if we want to truncate inside one interval
		__int64 ulStart = 0;
		__int64 ulEnd   = (*it).length;
		if( (*it).offset != startPos )
		{
			ulStart = startPos - (*it).offset;
		}

		if( startPos + size < (*it).offset + (*it).length )
		{
			ulEnd = startPos + size - (*it).offset;
		}

		if( ulEnd - ulStart == (*it).length )
		{
			m_VecChanges.erase( it );
		}
		else
		{
			(*it).ustrbuffer.erase( ulStart + 1, (*it).length - ulStart - 1 );
			(*it).length = ulStart;
			if( ulEnd < (*it).length )
			{
				ustring ustrRest;
				slice Item;
				Item.length = (*it).length - ulEnd;
				if( !(*it).ustrbuffer.empty() )
				{
					ustrRest.append( (*it).ustrbuffer.substr( ulEnd, (*it).length - ulEnd ) );
				}
				

				Item.offset = startPos + size + 1;
				Item.ustrbuffer.swap( std::move( ustrRest ) );
				it = m_VecChanges.insert( ++it, Item );
			}
		}
	}

	++it;
	std::for_each( it, m_VecChanges.end(), [&size]( slice& obj )
	{
		obj.offset = obj.offset - size;
	});
	return false;
}


inline std::list<MemoFile::slice>::iterator MemoFile::findPositionInVector( __int64 startPos )
{
	std::function<bool( const slice&, __int64)> cmp = []( const slice& obj, __int64 val )
	{
		return !(obj.offset <= val && obj.offset + obj.length >= val); 
	};

	std::list<MemoFile::slice>::iterator it =  std::lower_bound( m_VecChanges.begin(), m_VecChanges.end(), startPos, cmp );

	if( it != m_VecChanges.end() && cmp( *it, startPos ) )
	{
		return it;
	}

	if( m_VecChanges.size() == 1 && it == m_VecChanges.end() )
	{
		return --it;
	}
	//if( m_VecChanges.size() > 1 && it != m_VecChanges.end() )
	//{
	//	++it;
	//}
	return it;
}

__int64 MemoFile::insertOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	std::list<slice>::iterator it    = findPositionInVector( startPos );

	if( it == m_VecChanges.end() )
	{
		return 0;
	}

		std::list<slice>::iterator itEnd;
		if( ((*it).offset + (*it).length >= startPos + size && (*it).offset <= startPos ) || (  m_VecChanges.size() == 1 ) )
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
			restBuff.append( ( (*itEnd).ustrbuffer.substr( startPos + size - (*itEnd).offset, (*itEnd).offset + (*itEnd).length - startPos - size) ) );
		}

			if( (*it).offset == startPos )
			{
				(*it).length = size - 1;		
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
				Item.length = size - 1;
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
				if( itEnd != m_VecChanges.end() )
				{
					slice Item;
					Item.length = (*itEnd).offset + (*itEnd).length - ( startPos + size );
					Item.offset = startPos + size + 1;
					Item.ustrbuffer.swap( std::move( restBuff ) );
					m_VecChanges.insert( ++it, Item );
				}
			}

	return size;
}
__int64 MemoFile::insertNoOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	std::list<slice>::iterator it = findPositionInVector( startPos );
	if( it == m_VecChanges.end() )
	{
		return 0;
	}

	ustring strTemp( buffer );
	slice Item;
	Item.offset = startPos;
	Item.length = size;

	Item.ustrbuffer.swap( std::move( strTemp ) );
	ustring restBuff;
	if( !(*it).ustrbuffer.empty() )
	{
		restBuff.append( (*it).ustrbuffer.substr( startPos - (*it).offset, (*it).length - ( startPos - (*it).offset ) ) );//begin position to copy and number of chars to copy
	}

	if( (*it).offset < startPos )
	{
		(*it).length = startPos - (*it).offset;
		if( !(*it).ustrbuffer.empty() )
		{
			(*it).ustrbuffer.erase( startPos - (*it).offset + 1, restBuff.length() - 1 );
		}

		it = m_VecChanges.insert( ++it, Item );

		slice EndItem;
		EndItem.offset = startPos + size + 1;
		EndItem.length = restBuff.length() - 1;
		EndItem.ustrbuffer.swap( std::move( restBuff ) );
		it = m_VecChanges.insert( ++it, EndItem );
	}
	else
	{
		if( (*it).offset == startPos )
		{
			it = m_VecChanges.insert( it, Item );
		}
	}
	++it;
	std::for_each( it, m_VecChanges.end(), [&size]( slice& obj )
	{
		obj.offset = obj.offset + size + 1;
	});

	return size;
}

void MemoFile::inscrease( __int64  size )
{
	slice Item;
	if( m_VecChanges.size() > 0 )
	{
		std::list<slice>::iterator it = m_VecChanges.end();
		--it;
		Item.length = size;
		Item.offset = (*it).offset + (*it).length + 1;
		ustring strNew( size, ' ' );
		Item.ustrbuffer.swap( std::move( strNew ) );
		m_VecChanges.push_back( Item );
	}
	else
	{
		Item.offset = 0;
		Item.length = size;
		m_VecChanges.push_back( Item );
	}
}

void MemoFile::swapContainer( __int64 startPos, __int64 size )
{
	std::list<slice> temp;
	slice item;
	item.length = size;
	item.offset = startPos;
	temp.push_back( item );
	m_VecChanges.swap( std::move( temp ) );
	m_CommitSize = 0;
}