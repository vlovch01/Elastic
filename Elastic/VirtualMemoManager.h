#pragma once
#include <vector>
#include <list>
#include <Windows.h>

class VirtualMemoManager
{
public:
	struct PageInfo
	{
		PBYTE m_pBegin;
		PBYTE m_pCurrentPos;
	};

	struct Blanks
	{
		PBYTE m_pStart;
		PBYTE m_pEnd;
		unsigned int uiPageId;
	};
	static std::shared_ptr<VirtualMemoManager>& getInstance( );

	~VirtualMemoManager(void);
	 void getPointerWithLength( __int64 len, unsigned int& pageID, PBYTE start );
	 void deleteLength( unsigned int& pageId, PBYTE start, __int64 len );
	 inline __int64 getPageSize()const{ return m_pageSize; }
private:
	VirtualMemoManager( );
	VirtualMemoManager( const VirtualMemoManager& );
	VirtualMemoManager& operator=( const VirtualMemoManager&);

private:
	__int64 m_pageSize;
	std::vector<PageInfo> m_VPages;
	std::list<Blanks> m_listBlanks;
};

