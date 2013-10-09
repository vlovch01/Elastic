#include "ElasticFile.h"
#include <strsafe.h>
#include <algorithm>
#include <list>
#include <iostream>

ElasticFile::ElasticFile(void) : m_filePos(0), m_fileSize(0), m_CurrentID( 0 ), m_BufferCommitSize( 0 ), m_dwSystemGranularity( 0 )
{
}


ElasticFile::~ElasticFile(void)
{
	updateDataFile();
}

HANDLE ElasticFile::FileOpen( std::wstring& FileName, OpenMode Mode )
{
	HANDLE hFile;
	DWORD dwMode = 0;
	DWORD dwOpen = 0;

	while( Mode )
	{
		switch( Mode )
		{
		case OpenMode::Append:
			dwOpen = OPEN_EXISTING;
			Mode = static_cast<OpenMode>( Mode & OpenMode::Append);
			break;
		case OpenMode::Create:
			dwOpen = CREATE_ALWAYS;
			Mode = static_cast<OpenMode>( Mode & OpenMode::Create );
			break;
		case OpenMode::CreateNew:
			dwOpen = CREATE_NEW;
			Mode = static_cast<OpenMode>( Mode & OpenMode::CreateNew );
			break;
		case OpenMode::Open:
			dwOpen = OPEN_EXISTING;
			Mode = static_cast<OpenMode>( Mode & OpenMode::Open );
			break;
		case OpenMode::OpenOrCreate:
			dwOpen = OPEN_EXISTING | CREATE_NEW;
			Mode = static_cast<OpenMode>( Mode & OpenMode::OpenOrCreate );
			break;
		case OpenMode::Truncate:
			dwOpen = TRUNCATE_EXISTING;
			Mode = static_cast<OpenMode>( Mode & OpenMode::Truncate );
			break;

		default:
			dwOpen = OPEN_EXISTING;
		}
		dwMode = dwMode | dwOpen;
	}

	hFile = ::CreateFile( FileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, dwMode, FILE_ATTRIBUTE_NORMAL, 0 ); 

	if( hFile == INVALID_HANDLE_VALUE )
	{
		return hFile;
	}

	m_fileHandle = hFile;
	chunk start;
	start.offset = 0;
	start.lenght = 0;
	LARGE_INTEGER largeInt;
	if( ::GetFileSizeEx( hFile, &largeInt ) )
	{
		start.lenght = largeInt.QuadPart;
		m_fileSize   = largeInt.QuadPart;
	}
	
	m_Changes.push_back( start );
	m_filePos = 0;

	 _SYSTEM_INFO sysInfo ={0};
	 ::GetSystemInfo( &sysInfo );
	 m_dwSystemGranularity = sysInfo.dwAllocationGranularity * 2;

	return hFile;
}

BOOL ElasticFile::FileSetCursor( HANDLE file, ULONG Offset, CursorMoveMode Mode )
{
	DWORD dwMode = FILE_CURRENT;
	switch( Mode )
	{
		case Begin:   dwMode = FILE_BEGIN; m_filePos = Offset; 
			break;
		case Current: dwMode = FILE_CURRENT; m_filePos += Offset;
			break;
		case End:    dwMode = FILE_END; m_filePos = m_fileSize - Offset;
			break;
	}

	LARGE_INTEGER largeInt;
	largeInt.QuadPart = Offset;
	if( ::SetFilePointerEx( file, largeInt, 0, dwMode ) )
	{
		if( m_filePos > m_fileSize )
		{
			//If the new cursor position is beyond the data which this file contains, the file must be extended up to the required size.
			m_Changes[ m_Changes.size() - 1].lenght += Offset;
			std::map<Chunk_ID, ustring>::iterator it = m_BufferMap.find( m_Changes[ m_Changes.size() - 1].ID );
			ustring str( ' ', Offset );
			it->second.append( str );
			m_BufferCommitSize += Offset;

			if( ::SetEndOfFile( file ) )
			{
				return TRUE;
			}
			else
			{
				ErrorExit(L"SetEndOfFile");
				return FALSE;
			}
		}
	}
	else
	{
		ErrorExit(L"SetFilePointerEx");
		return FALSE;
	}
}


__int64 ElasticFile::FileGetCursor( HANDLE file )
{
	LARGE_INTEGER li;
	LARGE_INTEGER old = {0};
	::SetFilePointerEx( file, old, &li, FILE_CURRENT );
	m_filePos = li.QuadPart;
	return m_filePos;
}

ULONG ElasticFile::FileRead( HANDLE file, PBYTE buffer, ULONG size )
{
	if( m_fileHandle != file )
	{
		return 0;
	}
	//Find where in file vector of chuncks is placed our currsor 
	//then find buffer from map
	unsigned int index    = findCursorPositionInVectorBuffer( m_filePos );
	ULONG bufferSize      = 0;
	__int64 startPosition = m_filePos;
	__int64 endPositon    = m_filePos + size;
	if( endPositon > m_fileSize )
	{
		endPositon = m_fileSize;
	}

	HANDLE hMemMap = ::CreateFileMappingW( file, 0, PAGE_READONLY, 0, 0, 0 );
	DWORD dwview = 0;
	BYTE *pbuf = NULL;

	while( startPosition <= endPositon )
	{
		std::map< Chunk_ID, ustring>::iterator it = m_BufferMap.find( m_Changes[index].ID );
		if( it != m_BufferMap.end() )
		{	//read from map buffer because we have changes
			unsigned int startCopy = startPosition - m_Changes[index].offset;
			memcpy( buffer + bufferSize, it->second.substr( startCopy ).c_str(), it->second.size() - 1 - startCopy );
			bufferSize += it->second.size() - startCopy;
		}
		else
		{
			if( m_Changes[index].lenght > 0 )
			{
				//Create file mappping and read memory - we don't have changes in this index
				//dwview = m_dwSystemGranularity;
				dwview = m_Changes[index].lenght;
				__int64 j = startPosition;
				{
					DWORD high = static_cast<DWORD>((j >> 32) & 0xFFFFFFFFul);
					DWORD low  = static_cast<DWORD>( j        & 0xFFFFFFFFul);
					if( j + dwview > endPositon )
					{
						dwview = static_cast<DWORD>(endPositon - j);//for last step when buffer is smaller then page size
					}
					pbuf = (BYTE*)::MapViewOfFile( hMemMap, FILE_MAP_READ, high, low, dwview );
					memcpy( buffer + bufferSize, pbuf, dwview );
					bufferSize += dwview + 1;
					::UnmapViewOfFile( pbuf );
				}
			}
			//else procede to next element in vector ( this one was truncated )
			++index;
		}
	}
	
	if( pbuf )
	{
		::UnmapViewOfFile( pbuf );
	}
	::CloseHandle( hMemMap );
	if( bufferSize != size )
	{
		std::cout<<" buffer size not equal :  bufferSize = "<<bufferSize<<" sent size = "<<size<<std::endl;
	}
	return bufferSize;
}

ULONG ElasticFile::FileWrite( HANDLE file, PBYTE buffer, ULONG size, bool overwrite )
{
	unsigned int index = findCursorPositionInVectorBuffer( m_filePos );
	__int64 startPosition = m_filePos;
	__int64 endPosition   = m_filePos + size;
	if( overwrite )
	{
		//then  we overwrite all buffer's between index and endPosition

	}
	else
	{
		//create new record in vector buf

	}
	return 0;
}

bool ElasticFile::FileTruncate( HANDLE file, ULONG cutSize )
{
	//TODO if m_Changes.size() == 1 then do truncation in place
	unsigned int index    = findCursorPositionInVectorBuffer( m_filePos );
	__int64 startPosition = m_filePos;
	__int64 endPosition   = m_filePos + cutSize;
	
	if( endPosition > m_fileSize )
	{
		endPosition = m_fileSize;
	}

	while( startPosition <= endPosition )
	{
		m_Changes[index].lenght = startPosition - m_Changes[index].offset;
		std::map< Chunk_ID, ustring>::iterator it = m_BufferMap.find( m_Changes[index].ID );
		if( it != m_BufferMap.end() )
		{
			if( m_Changes[index].lenght == 0 )
			{
				m_BufferMap.erase( it );
			}
			else
			{
				it->second.erase( m_Changes[index].lenght + 1);
			}
		}
		++index;
		startPosition = m_Changes[index].offset;
	}
	
	//compress two adicents elements from vector buffer if they are empty
	unsigned int j = index + 1;
	std::vector<chunk>::iterator it = m_Changes.begin();
	std::advance( it, j );
	while( j < m_Changes.size() )
	{
		if( m_Changes[index].lenght == 0 && m_Changes[j].lenght == 0 )
		{
			m_Changes[index].lenght += m_Changes[j].lenght;
			m_Changes.erase( it );
		}
		else
		{
			index = j;
			++j;
			++it;
		}
	}
	return TRUE;
}

bool ElasticFile::FileClose( HANDLE file )
{
	updateDataFile();
	return ::CloseHandle( file );
}

unsigned int ElasticFile::findCursorPositionInVectorBuffer( __int64 position )const
{
	unsigned int index = 0;
	for( ; index < m_Changes.size(); ++index)
	{
		if( m_Changes[index].offset < position && m_Changes[index].lenght > position )
		{
			break;
		}
	}
	return index;
}

inline bool ElasticFile::checkIfCommit()const
{
	return ( m_BufferCommitSize < 300 * 1024 * 1024 && m_Changes.size() < 1000 );//no more then 300MB and 1000 elements in vector buffer
}
void ElasticFile::updateDataFile()
{
	

}

void ElasticFile::ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
					dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
 //   ExitProcess(dw); 
}