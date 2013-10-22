#include "ElasticFile.h"
#include "ErrorLoger.h"
#include "MemoFile.h"
#include "VirtualMemoManager.h"

#include <algorithm>
#include <iostream>

ElasticFile::ElasticFile(void) : m_filePos(0), m_fileSize(0), m_BufferCommitSize( 0 ), m_dwSystemGranularity( 0 ), m_reserved( 16 )
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
	//chunk start;
	//start.offset = 0;
	//start.lenght = 0;
	//start._chunkStatus = NotChanged;

	LARGE_INTEGER largeInt;
	if( ::GetFileSizeEx( hFile, &largeInt ) )
	{
		if( m_spMemoFile )
		{
			//swap container
			m_spMemoFile->swapContainer( 0, largeInt.QuadPart );
		}
		else
		{
			m_spMemoFile.reset( new MemoFile( 0, largeInt.QuadPart ) );
		}
		m_fileSize   = largeInt.QuadPart;
	}
	
	//m_Changes.push_back( start );
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
	/*		m_Changes[ m_Changes.size() - 1].lenght += m_filePos - m_fileSize;
			ustring str( m_filePos - m_fileSize, ' ' );
			m_Changes[ m_Changes.size() - 1].usbuffer.append( str );
			m_BufferCommitSize += m_filePos - m_fileSize;*/
			m_spMemoFile->inscrease( m_filePos - m_fileSize );
			m_BufferCommitSize += m_filePos - m_fileSize;
			m_fileSize         += m_filePos - m_fileSize;

			if( ::SetEndOfFile( file ) )
			{
				return TRUE;
			}
			else
			{
				ErrorLoger::ErrorMessage(L"SetEndOfFile");
				return FALSE;
			}
		}
	}
	else
	{
		ErrorLoger::ErrorMessage(L"SetFilePointerEx");
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
	unsigned int index    = 0;
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
	__int64 writen = m_spMemoFile->insert( m_filePos, buffer, size, overwrite );
	if( !overwrite )
	{
		m_BufferCommitSize += writen;
		m_fileSize         += writen;
	}
	else
	{
		m_BufferCommitSize = std::max( m_BufferCommitSize, writen );
		m_fileSize         = std::max( m_BufferCommitSize, writen );
	}

	return writen;
}

bool ElasticFile::FileTruncate( HANDLE file, ULONG cutSize )
{
	checkIfCommit();
		
	return m_spMemoFile->truncate( m_filePos, cutSize );
	
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

void ElasticFile::checkIfCommit()
{
	if ( m_BufferCommitSize > 100 * 1024 * 1024 || m_spMemoFile->getChanges().size() > m_reserved )//no more then 100MB and 65535 elements in vector buffer
	{
		updateDataFile();
		if( m_reserved < 65535 )
		{
			m_reserved *= 2;
		}
		FileOpen( m_FileName, Open );//reopen file after update
	}
}
void ElasticFile::updateDataFile()
{
	std::list<MemoFile::slice> listChanges = m_spMemoFile->getChanges();
	std::list<MemoFile::slice>::iterator it = listChanges.begin();
	std::list<MemoFile::slice>::iterator prevIt;

	if( listChanges.empty() )
	{
		return;
	}

	HANDLE hTempFile;
	__int64 tempFileCursor = (*it).offset;

	TCHAR tempFileName[MAX_PATH + 1] = {'\0'};
	if( ::GetTempPath( MAX_PATH, tempFileName ) )
	{
		if( ::GetTempFileName( tempFileName, TEXT("~TMP"), 0, tempFileName ) )
		{
			hTempFile = ::CreateFile( tempFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
			LARGE_INTEGER set;
			LARGE_INTEGER result;

			set.QuadPart = m_fileSize;
			if( ::SetFilePointerEx( hTempFile, set, &result, FILE_BEGIN ) )
			{
				::SetEndOfFile( hTempFile );
			}
		}
	}

	__int64 fileOffset = 0;
	//std::vector<chunk>::iterator it = m_Changes.begin();
//	//HANDLE hMemMap = ::CreateFileMappingW( m_fileHandle, 0, PAGE_READONLY, 0, 0, 0 );
	DWORD dwRead;
	LARGE_INTEGER li = {0};

	for( ; it != listChanges.end(); ++it )
	{
		if( (*it).length > 0 )
		{
			if( (*it).ustrbuffer.empty() )
			{
				//read from original file
				//DWORD dwview = (*it).length;
				//__int64 j = (*it).offset;
				{
					//DWORD high = static_cast<DWORD>((j >> 32) & 0xFFFFFFFFul);
					//DWORD low  = static_cast<DWORD>( j        & 0xFFFFFFFFul);
					//if( j + dwview > m_fileSize )
					//{
					//	dwview = static_cast<DWORD>(m_fileSize - j);//for last step when buffer is smaller then page size
					//}
					//BYTE *pbuf = (BYTE*)::MapViewOfFile( hMemMap, FILE_MAP_READ, high, low, 0 );
					if( it != listChanges.begin() )
					{
						prevIt = std::prev( it, 1 );
						if( !(*prevIt).ustrbuffer.empty() )
						{
							fileOffset += (*prevIt).length;
						}
					}

					BYTE *pbuf = new BYTE[ (*it).length + 1 ]; 
					li.QuadPart = (*it).offset - fileOffset;
					
					::SetFilePointerEx( m_fileHandle, li, 0, FILE_BEGIN );
					if ( ::ReadFile( m_fileHandle, pbuf, (*it).length, &dwRead, NULL ) )
					{
						tempFileCursor += writeToFile( hTempFile, pbuf, tempFileCursor, (*it).length );
					}
					else
					{
						ErrorLoger::ErrorMessage(L"ReadFile");
					}
					delete []pbuf;
					//::UnmapViewOfFile( pbuf );
				}
			}
			else
			{
				//read from buffer
				tempFileCursor += writeToFile( hTempFile, static_cast<PBYTE>(const_cast<unsigned char*>((*it).ustrbuffer.c_str())), tempFileCursor, (*it).length );
			}
		}

	}
	
//	::CloseHandle( hMemMap );
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
			ErrorLoger::ErrorMessage(L"WriteFile");
			return 0;
		}
	}
	else
	{
		ErrorLoger::ErrorMessage(L"SetFilePointerEx");
		return 0;
	}
	return 0;
}
