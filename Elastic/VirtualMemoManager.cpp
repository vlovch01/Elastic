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
			break;
		}
	}
}
