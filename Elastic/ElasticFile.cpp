#include "ElasticFile.h"
#include <strsafe.h>
#include <algorithm>
#include <iostream>

ElasticFile::ElasticFile(void) : m_filePos(0), m_fileSize(0), m_BufferCommitSize( 0 ), m_dwSystemGranularity( 0 )
{
	_SYSTEM_INFO sysInfo ={0};
	::GetSystemInfo( &sysInfo );
	m_dwSystemGranularity = sysInfo.dwAllocationGranularity * 2;

}


ElasticFile::~ElasticFile(void)
{
	FileClose( m_fileHandle );
}

HANDLE ElasticFile::FileOpen( std::wstring& FileName, OpenMode Mode )
{
	HANDLE hFile;
	m_FileName.assign( FileName );
	DWORD dwOpen = 0;
	DWORD dwMode = 0;
	m_mode = Mode;

	if( !m_Changes.empty() )
	{
		std::vector<chunk>().swap( m_Changes );
	}

	while( Mode )
	{
		switch( Mode )
		{
		case Append:
			dwMode = OPEN_EXISTING;
			Mode = static_cast<OpenMode>( Mode & ~Append);
			break;
		case Create:
			dwMode = CREATE_ALWAYS;
			Mode = static_cast<OpenMode>( Mode & ~Create );
			break;
		case CreateNew:
			dwMode = CREATE_NEW;
			Mode = static_cast<OpenMode>( Mode & ~CreateNew );
			break;
		case Open:
			dwMode = OPEN_EXISTING;
			Mode = static_cast<OpenMode>( Mode & ~Open );
			break;
		case OpenOrCreate:
			dwMode = OPEN_EXISTING | CREATE_NEW;
			Mode = static_cast<OpenMode>( Mode & ~OpenOrCreate );
			break;
		case Truncate:
			dwMode = TRUNCATE_EXISTING;
			Mode = static_cast<OpenMode>( Mode & ~Truncate );
			break;

		default:
			dwMode = OPEN_EXISTING;
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
	start._chunkStatus = NotChanged;
	LARGE_INTEGER largeInt;
	if( ::GetFileSizeEx( hFile, &largeInt ) )
	{
		start.lenght = largeInt.QuadPart;
		m_fileSize   = largeInt.QuadPart;
	}
	
	m_Changes.push_back( start );
	m_filePos = 0;
	m_BufferCommitSize = 0;

	return hFile;
}

BOOL ElasticFile::FileSetCursor( HANDLE file, ULONG Offset, CursorMoveMode Mode )
{
	checkIfCommit();
	
	DWORD dwMode = FILE_CURRENT;
	switch( Mode )//set where cursor should be located after adding Offset 
	{
		case Begin:   dwMode = FILE_BEGIN; m_filePos = Offset; 
			break;
		case Current: dwMode = FILE_CURRENT; m_filePos += Offset;
			break;
		case End:    dwMode = FILE_END;
			if( m_fileSize - Offset > 0 ) 
			{
				 m_filePos = m_fileSize - Offset;
			}
			break;
	}//m_filePos changed

	LARGE_INTEGER largeInt;
	largeInt.QuadPart = Offset;
	if( ::SetFilePointerEx( file, largeInt, 0, dwMode ) )
	{
		if( m_filePos > m_fileSize )
		{
			//If the new cursor position is beyond the data which this file contains, the file must be extended up to the required size.
			m_Changes[ m_Changes.size() - 1].lenght += m_filePos - m_fileSize;
			ustring str( m_filePos - m_fileSize, ' ' );
			m_Changes[ m_Changes.size() - 1].usbuffer.append( str );
			m_BufferCommitSize += m_filePos - m_fileSize;

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
	return FALSE;
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

	checkIfCommit();
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
		//std::map< Chunk_ID, ustring>::iterator it = m_BufferMap.find( m_Changes[index].ID );
		if( !m_Changes[index].usbuffer.empty() )
		{	//read from map buffer because we have changes
			__int64 startCopy = startPosition - m_Changes[index].offset;
			memcpy( buffer + bufferSize, m_Changes[index].usbuffer.c_str() + startCopy, m_Changes[index].usbuffer.size() - 1 - startCopy );
			bufferSize += m_Changes[index].usbuffer.size() - startCopy;
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
	checkIfCommit();

	unsigned int index    = findCursorPositionInVectorBuffer( m_filePos );
	__int64 startPosition = m_filePos;
	__int64 endPosition   = m_filePos + size;
	ULONG ulwritten       = 0;
	ustring strToAdd;

	if( overwrite )
	{
		//then  we overwrite all buffer's between index and endPosition
		while( startPosition < endPosition && index <= m_Changes.size() - 1 )
		{
			if( m_Changes[index].lenght > 0 )
			{
				if( !m_Changes[index].usbuffer.empty() )
				{
					//because buffer is not empty we can do inplace changes(overwrite privious changes)
					strToAdd.assign( m_Changes[index].usbuffer.c_str(), startPosition - m_Changes[index].offset );//form the beginning of previos buff copy 
					__int64 numElToCopy = std::min( m_Changes[index].lenght - ( startPosition - m_Changes[index].offset ), endPosition - startPosition );
					
					strToAdd.append( buffer + ulwritten, numElToCopy );
					if( size - ulwritten < m_Changes[index].lenght )
					{
						//add the rest of the string
						strToAdd.append( m_Changes[index].usbuffer.c_str() + size - ulwritten, m_Changes[index].lenght - ( size - ulwritten ));
					}
					m_Changes[index].usbuffer.swap( strToAdd );
					ulwritten += numElToCopy;
					startPosition += numElToCopy;
					strToAdd.clear();
				}
				else
				{
					//this chunk was't updated yet so we add a new buffer
					chunk Item;
					Item._chunkStatus = Modified;
					Item.offset       = startPosition;
					Item.lenght       = std::min( m_Changes[index].offset + m_Changes[index].lenght - startPosition, endPosition - startPosition );

					strToAdd.assign( buffer + ulwritten, Item.lenght );
					ulwritten     += Item.lenght;
					startPosition += Item.lenght;

					std::vector<chunk>::iterator it = m_Changes.begin();
					if( index + 1 < m_Changes.size() )
					{
						std::advance( it, index + 1 );
						m_Changes.insert( it, Item );
					}
					else
					{
						m_Changes.push_back( Item );
					}

					if( endPosition - startPosition < m_Changes[index].lenght )
					{
					//then we should add another Item
						chunk rest;
						rest._chunkStatus  = Modified;
						rest.offset        = startPosition;
						rest.lenght        = m_Changes[index].lenght - endPosition - m_Changes[index].offset;
						//buffer we don't change
						if( index + 2 < m_Changes.size() )
						{
							it = m_Changes.begin();
							std::advance( it, index + 2 );
							m_Changes.insert( it, rest );
						}
						else
						{
							m_Changes.push_back( Item );
						}
						++index;
					}
					m_Changes[index].lenght = startPosition - m_Changes[index].offset;//change lenght to the current Item in vector buffer
					++index;//we just added one more item and we want to start from next
				}
			}
			++index;
		}
	}
	else
	{
		//create new record in vector buf with lenght = size and update next neighbor offset
		ulwritten = size;
		chunk Item;
		Item.lenght  = size;
		Item.offset  = startPosition;
		Item.usbuffer.assign( buffer );
		std::vector<chunk>::iterator it = m_Changes.begin();
		if( index + 1 < m_Changes.size() )
		{
			std::advance( it, index + 1 );
			it = m_Changes.insert( it, Item );
			++it;
			while( it != m_Changes.end() )
			{
				(*it).offset += Item.lenght;
				++it;
			}
		}
		else
		{
			m_Changes.push_back( Item );
		}
		__int64 numElem = startPosition - m_Changes[index].offset;
		if( !m_Changes[index].usbuffer.empty() && m_filePos + size != m_Changes[index].offset )
		{
			//if is not empty then we should split the index buf in two Items
			ustring strRest = m_Changes[index].usbuffer.substr( numElem, m_Changes[index].lenght );
			m_Changes[index].usbuffer.erase( numElem, m_Changes[index].lenght );
			chunk rest;
			rest.usbuffer.assign( strRest );
			rest.lenght = m_Changes[index].lenght  - numElem;
			rest.offset = Item.lenght + Item.offset;

			if( index + 2 < m_Changes.size() )
			{
				it = m_Changes.begin();
				std::advance( it, index + 2 );
				it = m_Changes.insert( it, rest );
			}
			else
			{
				m_Changes.push_back( rest );
			}
			m_Changes[index].lenght = numElem;
		}
		m_BufferCommitSize += ulwritten;
		m_fileSize         += ulwritten;
	}

	if( m_Changes.size() > 1 )
	{
		m_Changes.erase( std::remove_if( m_Changes.begin(), m_Changes.end(), 
			[]( const chunk& obj )
		{
			return obj.lenght == 0;
		}), m_Changes.end() );
	}
	
	return ulwritten;
}

bool ElasticFile::FileTruncate( HANDLE file, ULONG cutSize )
{
	checkIfCommit();
	//TODO if m_Changes.size() == 1 then do truncation in place
	unsigned int index    = findCursorPositionInVectorBuffer( m_filePos );
	unsigned int tempIndex = index;
	__int64 startPosition = m_filePos;
	__int64 endPosition   = m_filePos + cutSize;
	
	if( endPosition > m_fileSize )
	{
		endPosition = m_fileSize;
	}

	while( startPosition <= endPosition )
	{
		m_Changes[index].lenght = startPosition - m_Changes[index].offset;
	
		if( m_Changes[index].lenght == 0 )
		{
			m_Changes[index]._chunkStatus = Deleted;//mark to delete
		}
		else
		{
			if( !m_Changes[index].usbuffer.empty() )
			{
				m_Changes[index].usbuffer.erase( 0, m_Changes[index].lenght );
				m_BufferCommitSize -= m_Changes[index].lenght;
			}
		}
		++index;
		startPosition = m_Changes[index].offset;
	}
	
	//remove vector items with length == 0
	if( m_Changes.size() > 1 )
	{
		m_Changes.erase( std::remove_if( m_Changes.begin(), m_Changes.end(), 
			[]( const chunk& obj )
		{
			return obj.lenght == 0;
		}), m_Changes.end() );
	}
	return TRUE;
}

BOOL ElasticFile::FileClose( HANDLE file )
{
	BOOL value = FALSE;
	updateDataFile();
	if( file ) 
	{
		value = ::CloseHandle( file );
	}
	return value;
}

unsigned int ElasticFile::findCursorPositionInVectorBuffer( __int64 position )const
{
	unsigned int index = 0;
	for( ; index < m_Changes.size(); ++index)
	{
		if( m_Changes[index].offset <= position && m_Changes[index].offset + m_Changes[index].lenght >= position )
		{
			return index;
		}
	}

	return 0;
}

void ElasticFile::checkIfCommit()
{
	if ( m_BufferCommitSize > 1 * 1024 * 1024 || m_Changes.size() > 9 )//no more then 300MB and 1000 elements in vector buffer
	{
		updateDataFile();
		FileOpen( m_FileName, Open );//reopen file after update
	}
}
void ElasticFile::updateDataFile()
{
	if( m_Changes.size() <= 1 && m_Changes[0].lenght == 0 )
	{
		return;
	}

	HANDLE hTempFile;
	const unsigned int lenPath = 261;
	__int64 tempFileCursor = m_Changes[0].offset;

	TCHAR tempFileName[lenPath] = {'\0'};
	if( ::GetTempPath( lenPath, tempFileName ) )
	{
		if( ::GetTempFileName( tempFileName, TEXT("~TMP"), 0, tempFileName ) )
		{
			hTempFile = ::CreateFile( tempFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
			LARGE_INTEGER set;
			LARGE_INTEGER result;

			set.QuadPart = m_BufferCommitSize;
			if( ::SetFilePointerEx( hTempFile, set, &result, FILE_BEGIN ) )
			{
				::SetEndOfFile( hTempFile );
			}
		}
	}

	std::vector<chunk>::iterator it = m_Changes.begin();
	HANDLE hMemMap = ::CreateFileMappingW( m_fileHandle, 0, PAGE_READONLY, 0, 0, 0 );

	for( ; it != m_Changes.end(); ++it )
	{
		if( (*it).lenght > 0 )
		{
			if( (*it).usbuffer.empty() )
			{
				//read from original file
				DWORD dwview = (*it).lenght;
				__int64 j = (*it).offset;
				{
					DWORD high = static_cast<DWORD>((j >> 32) & 0xFFFFFFFFul);
					DWORD low  = static_cast<DWORD>( j        & 0xFFFFFFFFul);
					if( j + dwview > m_fileSize )
					{
						dwview = static_cast<DWORD>(m_fileSize - j);//for last step when buffer is smaller then page size
					}
					BYTE *pbuf = (BYTE*)::MapViewOfFile( hMemMap, FILE_MAP_READ, high, low, dwview );
					
					tempFileCursor += writeToFile( hTempFile, pbuf, tempFileCursor, (*it).lenght );

					::UnmapViewOfFile( pbuf );
				}
			}
			else
			{
				//read from buffer
				tempFileCursor += writeToFile( hTempFile, static_cast<PBYTE>(const_cast<unsigned char*>((*it).usbuffer.c_str())), tempFileCursor, (*it).lenght );
			}
		}

	}
	
	::CloseHandle( hMemMap );
	::CloseHandle( hTempFile );
	
	TCHAR fileFullPath[MAX_PATH] = {'\0'};
	::GetFullPathName( m_FileName.c_str(), MAX_PATH, fileFullPath, NULL );
	::MoveFileEx( tempFileName, fileFullPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED );
}

DWORD ElasticFile::writeToFile( HANDLE hFile, PBYTE pBufer, __int64 offset, DWORD dwLength )
{
	LARGE_INTEGER lInt ={ 0 };
	lInt.QuadPart =  ( offset == 0 ? 0 : offset  );

	if( ::SetFilePointerEx( hFile, lInt, 0, FILE_BEGIN ) )
	{
		DWORD dwWritten = 0;
		if( ::WriteFile( hFile, pBufer, dwLength, &dwWritten, 0 ) )
		{
			return dwWritten;
		}
		else
		{
			ErrorExit(L"WriteFile");
			return 0;
		}
	}
	else
	{
		ErrorExit(L"SetFilePointerEx");
		return 0;
	}
	return 0;
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