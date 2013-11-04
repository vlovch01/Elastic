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
	 void getPointerWithLength( __int64 len, unsigned int& pageID, PBYTE &start );
	 void deleteLength( unsigned int& pageId, PBYTE start, __int64 len );
	 inline __int64 getPageSize()const{ return m_pageSize; }
	 bool isEnoughSpace( __int64 len )const;
	 void resetManager();
	 __int64 getNumberOfPages(){ return m_numberOfPages; }
private:
	VirtualMemoManager( );
	inline void init();
	VirtualMemoManager( const VirtualMemoManager& );
	VirtualMemoManager& operator=( const VirtualMemoManager&);

private:
	__int64 m_pageSize;
	__int64 m_freeMem;
	__int64 m_numberOfPages;
	unsigned int m_currentPage;
	std::vector<PageInfo> m_VPages;
	std::list<Blanks> m_listBlanks;
};

