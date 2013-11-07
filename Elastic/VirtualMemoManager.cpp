#include "VirtualMemoManager.h"
#include "ErrorLoger.h"

const unsigned long long OneAndHalfGb = 1.5 * 1024 * 1024 * 1024;

std::shared_ptr<VirtualMemoManager>& VirtualMemoManager::getInstance()
{
	static std::shared_ptr<VirtualMemoManager> instance;//on VS2012 and higher we should use std::scoped_ptr as a member variable
	if( !instance )
	{
		instance.reset( new VirtualMemoManager() );
	}
	return instance;
}

VirtualMemoManager::VirtualMemoManager(  ) : m_currentPage(0), m_numberOfPages( 0 )
{
	init();
}
void VirtualMemoManager::init()
{
	_SYSTEM_INFO sysInfo ={0};
	::GetSystemInfo( &sysInfo );
 
	m_pageSize = sysInfo.dwAllocationGranularity * 64;
	m_numberOfPages = OneAndHalfGb / m_pageSize;

	for( unsigned long i = 0; i < m_numberOfPages; ++i )
	{
		PBYTE pByte = (PBYTE)::VirtualAlloc( NULL, m_pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );//Page will have 4MB 
		if( !pByte )
		{
			break;
		}
		else
		{
			PageInfo pageInfo;
			pageInfo.m_pBegin      = pByte;
			pageInfo.m_pCurrentPos = pByte;
			m_VPages.push_back( pageInfo );
		}
	}
	m_freeMem = OneAndHalfGb;
}
void VirtualMemoManager::resetManager()
{
	for( unsigned int index = 0; index < m_VPages.size(); ++index )
	{
		cleanPage( index );
	}
	
	std::list<Blanks>().swap( m_listBlanks );
	m_freeMem = OneAndHalfGb;
	m_currentPage = 0;
}

void VirtualMemoManager::cleanPage( unsigned int id )
{
	::VirtualFree( (PVOID)m_VPages[id].m_pBegin, m_pageSize, MEM_DECOMMIT );
	m_VPages[id].m_pBegin      = (PBYTE)::VirtualAlloc( (PVOID)m_VPages[id].m_pBegin, m_pageSize, MEM_COMMIT, PAGE_READWRITE );
	m_VPages[id].m_pCurrentPos = m_VPages[id].m_pBegin;
}

VirtualMemoManager::~VirtualMemoManager(void)
{
	std::vector<PageInfo>::iterator it = m_VPages.begin();
	for( ; it != m_VPages.end(); ++it )
	{
		if( (*it).m_pBegin )
		{
			BOOL value = ::VirtualFree( (*it).m_pBegin, 0, MEM_RELEASE );
			if( !value )
			{
				ErrorLoger::ErrorMessage(L"VirtualFree - faild on page dealocation");
			}
		}
	}
}

bool VirtualMemoManager::getPointerWithLength(  __int64 len, unsigned int& pageID, PBYTE &start )
{
	unsigned int index = 0;
	pageID = 0;
	start  = NULL;
	bool bval = false;

	for( ; index < m_VPages.size(); ++index )
	{
		if( m_VPages[index].m_pBegin + m_pageSize - m_VPages[index].m_pCurrentPos >= len )
		{
			pageID = index;
			start  = m_VPages[index].m_pCurrentPos;
			m_VPages[index].m_pCurrentPos += len;
			m_freeMem -= len;
			m_currentPage = index;
			return bval;
		}
		if( m_currentPage == index )
		{
			//means page is full
			bval = true;
		}
	}

	std::list<Blanks>::iterator it = m_listBlanks.begin();
	for( ; it != m_listBlanks.end(); ++it )
	{
		if( (*it).m_pEnd - (*it).m_pStart == len )
		{
			pageID = (*it).uiPageId;
			start  = (*it).m_pStart;
			m_listBlanks.erase( it );
			return false;
		}
		else
		{
			if( (*it).m_pEnd - (*it).m_pStart < len )
			{
				pageID = (*it).uiPageId;
				start  = (*it).m_pStart;
				(*it).m_pStart += len + 1;
				return false;
			}
		}
	}
	return bval;
}

void VirtualMemoManager::deleteLength( unsigned int& pageId, PBYTE start, __int64 len )
{
	PBYTE pageStart = m_VPages[pageId].m_pBegin;
	PBYTE pageEnd   = m_pageSize + pageStart;
	if( start + len <= pageEnd )
	{
		Blanks black;
		black.m_pStart = start;
		black.m_pEnd   = start + len;
		black.uiPageId = pageId;
		m_listBlanks.push_back( black );
	}
}

bool VirtualMemoManager::isEnoughSpace( __int64 len )const
{
	unsigned int tempPage = m_currentPage;
	while( tempPage < m_VPages.size() )
	{
		if( m_VPages[tempPage].m_pBegin + m_pageSize - m_VPages[tempPage].m_pCurrentPos >= len )
		{
			return true;
		}
		++tempPage;
	}

	return false;
}

unsigned int VirtualMemoManager::getNextEmptyPage()const
{
	if( m_VPages[m_currentPage + 1].m_pBegin == m_VPages[m_currentPage + 1].m_pCurrentPos )
	{
		return m_currentPage + 1;
	}
	return m_currentPage + 2;
}

PBYTE VirtualMemoManager::getPageById( unsigned int id )const
{
	if( id >= 0 && id < m_VPages.size() )
	{
		return m_VPages[id].m_pBegin;
	}
	return NULL;
}

void VirtualMemoManager::swapPagesInContainer( unsigned int first, unsigned int second )
{
	cleanPage( second );
	std::swap( m_VPages[first], m_VPages[second] );
}

void VirtualMemoManager::makeFullPage( unsigned int id )
{
	m_VPages[id].m_pCurrentPos = m_VPages[id].m_pBegin + m_pageSize;
}