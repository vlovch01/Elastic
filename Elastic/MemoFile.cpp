#include "MemoFile.h"
#include "VirtualMemoManager.h"
#include <algorithm>
#include <functional>
#include <assert.h>

MemoFile::MemoFile( __int64 startPos, __int64 size ) : m_Compress( false ), m_PageCompress( false ), m_CommitSize( 0 )
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
	__int64 val = 0;

	if( overwrite )
	{
		val = insertOverWrite( startPos, buffer, size );
	}
	else
	{
		val = insertNoOverWrite( startPos, buffer, size );
	}

	if( m_Compress )
	{
		tryCompresion();
	}

	if( m_PageCompress )
	{
		tryPageCompresion();
	}
	return val;
}

bool MemoFile::truncate( __int64 startPos, __int64 size )
{
	if( m_VecChanges.empty() && m_CommitSize == 0 )
	{
		return false;
	}
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	std::list<slice>::iterator it    = findPositionInVector( startPos );
	std::list<slice>::iterator itEnd = findPositionInVector( startPos + size );

	if( it != itEnd )
	{
		if( (*it).offset != startPos )
		{
			__int64 lastPos =  startPos - (*it).offset;
			
			if( (*it).buffer != NULL )
			{
				//(*it).ustrbuffer.erase( lastPos + 1, (*it).offset + (*it).length - startPos );
				spMng->deleteLength( (*it).pageID, (*it).buffer + lastPos, (*it).offset + (*it).length - startPos );
			}
			(*it).length = lastPos;
			++it;
		}

		if( (*itEnd).buffer != NULL )
		{
			//(*itEnd).ustrbuffer.erase( 0, startPos + size - (*it).offset );
			spMng->deleteLength( (*itEnd).pageID, (*itEnd).buffer, startPos + size - (*itEnd).offset );
			(*itEnd).buffer += startPos + size - (*itEnd).offset + 1;
		}
		(*itEnd).length = (*itEnd).offset + (*itEnd).length - startPos - size;
		(*itEnd).offset = startPos + size + 1;

		while( it != itEnd )
		{
			if( (*it).buffer )
			{
				spMng->deleteLength( (*it).pageID, (*it).buffer, (*it).length );
			}
			m_VecChanges.erase( it );
			++it;
		}
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

		if( startPos + size <= (*it).offset + (*it).length )
		{
			ulEnd = startPos + size - (*it).offset;
		}

		if( ulEnd - ulStart == (*it).length )
		{
			if( (*it).buffer )
			{
				spMng->deleteLength( (*it).pageID, (*it).buffer, (*it).length );
			}
			m_VecChanges.erase( it );
		}
		else
		{
//			(*it).ustrbuffer.erase( ulStart + 1, (*it).length - ulStart - 1 );
			if( ulEnd < (*it).length )
			{
//				ustring ustrRest;
				slice Item;
				Item.length = (*it).length - ulEnd;
				if( (*it).buffer != NULL )
				{
//					ustrRest.append( (*it).ustrbuffer.substr( ulEnd, (*it).length - ulEnd ) );
					spMng->deleteLength( (*it).pageID, (*it).buffer + ulStart, size );
					Item.buffer = (*it).buffer + ulStart + size + 1;
					Item.pageID = (*it).pageID;
				}
				(*it).length = ulStart;
				
				Item.offset = startPos + size;
//				Item.ustrbuffer.swap( std::move( ustrRest ) );
				it = m_VecChanges.insert( ++it, Item );
				if( ulStart == 0 )
				{
					--it;
					it = m_VecChanges.erase( it );
				}
			}
		}
	}

	++it;
	std::for_each( it, m_VecChanges.end(), [&size]( slice& obj )
	{
		obj.offset = obj.offset - size;
	});
	return true;
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
	
	return it == m_VecChanges.begin() ? it : --it;
}

__int64 MemoFile::insertOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	unsigned int uiPageId = 0;
	PBYTE pByteBuffer = NULL;

	if( m_VecChanges.empty() )
	{
		assert( size > 0 );
		slice Item;
		Item.offset = startPos;
		Item.length = size;
		Item.overwrite = true;

		m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
		Item.pageID = uiPageId;
		Item.buffer = pByteBuffer;
		memcpy( pByteBuffer, buffer, size );

		m_VecChanges.push_back( Item );
		return size;
	}
	std::list<slice>::iterator it    = findPositionInVector( startPos );

	if( it == m_VecChanges.end() )
	{
		return 0;
	}

	std::list<slice>::iterator itEnd = findPositionInVector( startPos + size );

	if( itEnd == m_VecChanges.end() )
	{
		--itEnd;
	}
	//update
	if( (*it).offset < (*itEnd).offset )
	{
		std::list<slice>::iterator itRemove = it;
		itRemove++;//clean memory between it and itEnd
		for( ; itRemove != itEnd; ++itRemove )
		{
			if( (*itRemove).buffer )
			{
				spMng->deleteLength( (*itRemove).pageID, (*itRemove).buffer, (*itRemove).length );
			}
		}

		if( (*it).offset == startPos )
		{
			(*it).length = size;
			if( (*it).buffer )
			{
				spMng->deleteLength( (*it).pageID, (*it).buffer, (*it).length );
			}
			m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
			(*it).pageID = uiPageId;
			(*it).buffer = pByteBuffer;
			(*it).overwrite = true;
			memcpy( pByteBuffer, buffer, size );
		}
		else
		{
			if( (*it).buffer != NULL )
			{
				spMng->deleteLength( (*it).pageID, (*it).buffer + startPos - (*it).offset, (*it).length );
			}
			(*it).length = startPos - (*it).offset;
			slice Item;
			Item.length = size;
			Item.offset = startPos;
			m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
			Item.pageID = uiPageId;
			memcpy( pByteBuffer, buffer, size );
			Item.buffer = pByteBuffer;
			Item.overwrite = true;

			it = m_VecChanges.insert( ++it, Item );
		}
		++it;
		//for itEnd
		if( startPos + size <= (*itEnd).offset + (*itEnd).length )
		{
				if( (*itEnd).buffer != NULL )
				{
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

		slice Item;
		if( startPos == (*it).offset ) 
		{
			if( (*it).buffer != NULL && size <= (*it).length )//startPos + size - (*it).offset
			{
				memcpy( (*it).buffer, buffer, size );
			}
			else
			{
				if( (*it).buffer != NULL )
				{
					spMng->deleteLength( (*it).pageID, (*it).buffer, (*it).length );// we will need bigger buff
				}
				m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
				memcpy( pByteBuffer, buffer, size );
				(*it).buffer = pByteBuffer;
				(*it).overwrite = true;
				if( size <= (*it).length )
				{
					Item.length = i64size;
					Item.offset  = (*it).offset + size;
					it = m_VecChanges.insert( ++it, Item );
					--it;
				}
				
				(*it).length = size;
			}
		}
		else
		{
			if( (*it).buffer != NULL )
			{
				if(	startPos + size - (*it).offset <= (*it).length )
				{
					memcpy( (*it).buffer + startPos - (*it).offset, buffer, size );
				}
				else//if to last element from vector we add buffer with size > (*it).lenght
				{
					spMng->deleteLength( (*it).pageID, (*it).buffer + startPos - (*it).offset + 1,  (*itEnd).offset + (*itEnd).length - startPos );// we will need bigger buff
					m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
					memcpy( pByteBuffer, buffer, size );
					(*it).length = startPos - (*it).offset;
					Item.buffer = pByteBuffer;
					Item.pageID = uiPageId;
					Item.length = size;
					Item.offset = startPos;
					Item.overwrite = true;
					it = m_VecChanges.insert( ++it, Item );
				}
			}
			else
			{
				m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
				Item.buffer = pByteBuffer;
				Item.pageID = uiPageId;
				Item.overwrite = true;
				memcpy( pByteBuffer, buffer, size );

				(*it).length = startPos - (*it).offset;

				Item.length = size;
				Item.offset = startPos;
				it = m_VecChanges.insert( ++it, Item );

				//for itEnd
				slice sItem;
				sItem.length = i64size;
				sItem.offset = startPos + size;
				m_VecChanges.insert( ++it, sItem );
			}
		}
	}

	return size;
}
__int64 MemoFile::insertNoOverWrite( __int64 startPos, PBYTE buffer, __int64 size )
{
	m_Compress = false;

	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	unsigned int uiPageId = 0;
	PBYTE pByteBuffer = NULL;
	m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
	slice Item;
	Item.offset = startPos;
	Item.length = size;
	Item.buffer = pByteBuffer;
	Item.pageID = uiPageId;
	memcpy( pByteBuffer, buffer, size );

	if( m_VecChanges.empty() )
	{
		assert( size > 0 );
		m_VecChanges.push_back( Item );
		return size;
	}
	std::list<slice>::iterator it = findPositionInVector( startPos );
	if( it == m_VecChanges.end() )
	{
		return 0;
	}

	__int64 i64size = (*it).length - ( startPos - (*it).offset );

	if( (*it).offset < startPos && (*it).offset + (*it).length > startPos )
	{
		(*it).length = startPos - (*it).offset;

		slice EndItem;
		EndItem.offset = startPos + size;
		EndItem.length = i64size;
		if( (*it).buffer != NULL )
		{
			EndItem.buffer = (*it).buffer + startPos - (*it).offset;
			EndItem.pageID = (*it).pageID;
		}
		it = m_VecChanges.insert( ++it, Item );
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
			++it;
			m_Compress = true;
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
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	unsigned int uiPageId = 0;
	PBYTE pByteBuffer = NULL;
	m_PageCompress = spMng->getPointerWithLength( size, uiPageId, pByteBuffer );
	Item.pageID = uiPageId;
	Item.buffer = pByteBuffer;
	memset( pByteBuffer, ' ', size );

	if( m_VecChanges.size() > 0 )
	{
		std::list<slice>::iterator it = m_VecChanges.end();
		--it;
		Item.length = size;
		Item.offset = (*it).offset + (*it).length;
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

void MemoFile::tryCompresion()
{
	std::list<slice>::iterator itComp = m_VecChanges.begin();
	std::list<slice>::iterator itNext = std::next( itComp );
	while( itNext != m_VecChanges.end() )
	{
		if( (*itNext).buffer && (*itComp).buffer && 
			(*itNext).buffer == (*itComp).buffer + (*itComp).length && (*itNext).pageID == (*itComp).pageID )
		{
			//do compresion 
			(*itComp).length += (*itNext).length;
			itNext = m_VecChanges.erase( itNext );
		}
		else
		{
			itComp = itNext;
			++itNext;
		}
	}
}

void MemoFile::tryPageCompresion( )
{
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	if( spMng->getNumberOfPages() - spMng->getCurrentPage() >= 2 )
	{
		__int64 i64EmptyPage = spMng->getNextEmptyPage();
	}
}