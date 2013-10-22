#pragma once
#include <vector>
#include <Windows.h>

class VirtualMemoManager
{
public:
	struct PageInfo
	{
		PBYTE m_pBegin;
		PBYTE m_pCurrentPos;
	};
	static std::shared_ptr<VirtualMemoManager>& getInstance( );

	~VirtualMemoManager(void);
	 void getPointerWithLength( __int64 len, unsigned int& pageID, PBYTE start );
	 inline __int64 getPageSize()const{ return m_pageSize; }
private:
	VirtualMemoManager( );
	VirtualMemoManager( const VirtualMemoManager& );
	VirtualMemoManager& operator=( const VirtualMemoManager&);

private:
	__int64 m_pageSize;
	std::vector<PageInfo> m_VPages;
};

