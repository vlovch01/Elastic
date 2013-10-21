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
	VirtualMemoManager(void);
	~VirtualMemoManager(void);
	 PBYTE getPointerWithLength( __int64 len, unsigned int& pageID );
	 	VirtualMemoManager( const VirtualMemoManager& ) = delete;
	VirtualMemoManager& operator=( const VirtualMemoManager&) = delete;
private:


private:
	__int64 m_pageSize;
	std::vector<PageInfo> m_VPages;
};

