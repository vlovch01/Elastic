#include "MemoFile.h"
#include "VirtualMemoManager.h"
#include <algorithm>
#include <functional>
#include <assert.h>

MemoFile::MemoFile( __int64 startPos, __int64 size )
{
	if( size != 0 )
	{
		slice first;
		first.length = size;
		first.offset = startPos;
		m_VecChanges.push_back( std::move( first ) );
	}
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
	std::list<MemoFile::slice>::iterator it = m_VecChanges.end();
	if( !m_VecChanges.empty() )
	{
		--it;
		if( (*it).length +(*it).offset < startPos )
		{
			return m_VecChanges.end();
		}
	}

	std::function<bool( const slice&, __int64)> cmp = []( const slice& obj, __int64 val )
	{
		return obj.offset <= val; 
	};

	it =  std::lower_bound( m_VecChanges.begin(), m_VecChanges.end(), startPos, cmp );

	if( it != m_VecChanges.end() && cmp( *it, startPos ) )
	{
		return it;
	}

	//if( it != m_VecChanges.end() || ( m_VecChanges.size() == 1 && it == m_VecChanges.end() ) )
	//{
	//	return --it;
	//}
	
	return --it;
}

__int64 MemoFile::insertOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	unsigned int uiPageId = 0;
	PBYTE pByteBuffer = NULL;

	if( m_VecChanges.empty() )
	{
		assert( size > 0 );
		ustring strTemp( buffer );
		slice Item;
		Item.offset = startPos;
		Item.length = size;
		
		spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
		Item.pageID = uiPageId;
		Item.buffer = pByteBuffer;

		Item.ustrbuffer.swap( strTemp );
		m_VecChanges.push_back( Item );
		return size;
	}
	std::list<slice>::iterator it    = findPositionInVector( startPos );

	if( it == m_VecChanges.end() )
	{
		return 0;
	}

	std::list<slice>::iterator itEnd;
	itEnd = findPositionInVector( startPos + size );
	if( itEnd == m_VecChanges.end() )
	{
		--itEnd;
	}
	//update
	if( (*it).offset < (*itEnd).offset )
	{
		if( (*it).offset == startPos )
		{
			(*it).length = size;
			//(*it).ustrbuffer.assign( buffer );
			spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
			(*it).pageID = uiPageId;
			(*it).buffer = pByteBuffer;
			memcpy( pByteBuffer, buffer, size );
		}
		else
		{
			if( (*it).buffer != NULL )
			{
				//(*it).ustrbuffer.erase( startPos - (*it).offset, (*it).length );
				spMng->deleteLength( (*it).pageID, (*it).buffer + startPos - (*it).offset, (*it).length );
			}
			(*it).length = startPos - (*it).offset;
			slice Item;
			Item.ustrbuffer.assign( buffer );
			Item.length = size;
			Item.offset = startPos;
			spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
			Item.pageID = uiPageId;
			memcpy( pByteBuffer, buffer, size );
			Item.buffer = pByteBuffer;

			it = m_VecChanges.insert( ++it, Item );
		}
		++it;
		//for itEnd
		if( startPos + size <= (*itEnd).offset + (*itEnd).length )
		{
				if( (*itEnd).buffer != NULL )
				{
					//(*itEnd).ustrbuffer.erase( 0, startPos + size - (*itEnd).offset );
					(*itEnd).buffer += startPos + size - (*itEnd).offset + 1;
					spMng->deleteLength( (*it).pageID, (*it).buffer, startPos + size - (*itEnd).offset );
				}
				(*itEnd).length = (*itEnd).offset + (*it).length - startPos - size;
				(*itEnd).offset = startPos + size;
				m_VecChanges.erase( it, itEnd );
		}
	}
	else//case (*it).offset == (*itEnd).offset
	{
		__int64 i64size = (*itEnd).offset + (*itEnd).length - startPos - size;
		ustring restBuff;
		slice Item;
		if( startPos == (*it).offset ) 
		{
			if( (*it).buffer != NULL && size <= (*it).length )//startPos + size - (*it).offset
			{
				//restBuff.assign( (*it).ustrbuffer.substr( startPos + size - (*it).offset,  i64size ) );
				(*it).buffer += startPos - (*it).offset + 1;
				spMng->deleteLength( (*it).pageID, (*it).buffer, startPos - (*it).offset );
				
			}
			(*it).length = size;
			(*it).ustrbuffer.assign( buffer );
		}
		else
		{
			if( (*it).buffer != NULL && startPos + size - (*it).offset <= (*it).length )
			{
				//restBuff.assign( (*it).ustrbuffer.substr( startPos + size - (*it).offset,  i64size ) );
				//(*it).ustrbuffer.erase( startPos - (*it).offset, (*it).length );
				Item.buffer = (*it).buffer + startPos + size - (*it).offset + 1;
				spMng->deleteLength( (*it).pageID, (*it).buffer + startPos - (*it).offset, size );
			}
			(*it).length = startPos - (*it).offset;
			
			Item.ustrbuffer.assign( buffer );
			Item.length = size;
			Item.offset = startPos;
			Item.pageID = (*it).pageID;
			it = m_VecChanges.insert( ++it, Item );
		}
		//for itEnd
		if( Item.buffer != NULL )
		{
			slice Item;
			Item.ustrbuffer.swap( std::move( restBuff ) );
			Item.length = i64size;
			Item.offset = startPos + size;
			m_VecChanges.insert( ++it, Item );
		}
	}

	return size;
}
__int64 MemoFile::insertNoOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	ustring strTemp( buffer );
	slice Item;
	Item.offset = startPos;
	Item.length = size;

	if( m_VecChanges.empty() )
	{
		assert( size > 0 );
		Item.ustrbuffer.swap( strTemp );
		m_VecChanges.push_back( Item );
		return size;
	}
	std::list<slice>::iterator it = findPositionInVector( startPos );
	if( it == m_VecChanges.end() )
	{
		return 0;
	}

	Item.ustrbuffer.swap( std::move( strTemp ) );
	ustring restBuff;
	__int64 i64size = (*it).length - ( startPos - (*it).offset );
	if( !(*it).ustrbuffer.empty() )
	{
		restBuff.append( (*it).ustrbuffer.substr( startPos - (*it).offset, i64size ) );//begin position to copy and number of chars to copy
	}

	if( (*it).offset < startPos && (*it).offset + (*it).length > startPos )
	{
		(*it).length = startPos - (*it).offset;
		if( !(*it).ustrbuffer.empty() )
		{
			(*it).ustrbuffer.erase( startPos - (*it).offset, restBuff.length() );
		}

		it = m_VecChanges.insert( ++it, Item );

		slice EndItem;
		EndItem.offset = startPos + size;
		EndItem.length = i64size;
		EndItem.ustrbuffer.swap( std::move( restBuff ) );
		it = m_VecChanges.insert( ++it, EndItem );
	}
	else
	{
		if( (*it).offset == startPos )
		{
			it = m_VecChanges.insert( it, Item );
		}
		else
		{
			m_VecChanges.push_back( Item );
		}
	}
	++it;
	std::for_each( it, m_VecChanges.end(), [&size]( slice& obj )
	{
		obj.offset = obj.offset + size;
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
		Item.offset = (*it).offset + (*it).length;
		ustring strNew( size, ' ' );
		Item.ustrbuffer.swap( std::move( strNew ) );
		m_VecChanges.push_back( Item );
	}
	else
	{
		Item.offset = 0;
		Item.length = size;
		ustring strNew( size, ' ' );
		Item.ustrbuffer.swap( std::move( strNew ) );
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