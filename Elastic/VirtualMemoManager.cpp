#include "VirtualMemoManager.h"
#include "ErrorLoger.h"

const unsigned int OneKilo = 1024;

std::shared_ptr<VirtualMemoManager>& VirtualMemoManager::getInstance()
{
	static std::shared_ptr<VirtualMemoManager> instance;
	if( !instance )
	{
		instance.reset( new VirtualMemoManager() );
	}
	return instance;
}

VirtualMemoManager::VirtualMemoManager(  )
{
	_SYSTEM_INFO sysInfo ={0};
	::GetSystemInfo( &sysInfo );
 
	m_pageSize = sysInfo.dwAllocationGranularity * 64;
	for( unsigned long i = 0; i < OneKilo; ++i )
	{
		//For 32bit binary system will allocate 2GB but for 64bit 4gb
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
}


VirtualMemoManager::~VirtualMemoManager(void)
{
	std::vector<PageInfo>::iterator it = m_VPages.begin();
	for( ; it != m_VPages.end(); ++it )
	{
		if( (*it).m_pBegin )
		{
			BOOL value = ::VirtualFree( (*it).m_pBegin, 0, MEM_FREE );
			if( !value )
			{
				ErrorLoger::ErrorMessage(L"VirtualFree - faild on page dealocation");
			}
		}
	}
}

void VirtualMemoManager::getPointerWithLength(  __int64 len, unsigned int& pageID, PBYTE start )
{
	unsigned int index = 0;
	pageID = 0;
	start  = NULL;

	for( ; index < m_VPages.size(); ++index )
	{
		if( m_VPages[index].m_pBegin + m_pageSize - m_VPages[index].m_pCurrentPos >= len )
		{
			pageID = index;
			start  = m_VPages[index].m_pCurrentPos;
			m_VPages[index].m_pCurrentPos += len + 1;
			return;
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
			return;
		}
		else
		{
			if( (*it).m_pEnd - (*it).m_pStart < len )
			{
				pageID = (*it).uiPageId;
				start  = (*it).m_pStart;
				(*it).m_pStart += len + 1;
				return;
			}
		}
	}
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