#include "ElasticFile.h"
#include "ErrorLoger.h"
#include "MemoFile.h"
#include "VirtualMemoManager.h"

#include <algorithm>
#include <iostream>

const __int64 MAX_BUFF = 65536;

ElasticFile::ElasticFile(void) : m_filePos(0), m_fileSize(0), m_BufferCommitSize( 0 )
{
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
	m_BufferCommitSize = 0;

	return hFile;
}

BOOL ElasticFile::FileSetCursor( HANDLE &file, ULONG Offset, CursorMoveMode Mode )
{
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

	checkIfCommit( m_filePos - m_fileSize );
	LARGE_INTEGER largeInt;
	largeInt.QuadPart = Offset;
	if( ::SetFilePointerEx( file, largeInt, 0, dwMode ) )
	{
		if( m_filePos > m_fileSize )
		{
			//If the new cursor position is beyond the data which this file contains, the file must be extended up to the required size.
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
	return TRUE;
}


__int64 ElasticFile::FileGetCursor( HANDLE &file )
{
	LARGE_INTEGER li;
	LARGE_INTEGER old = {0};
	::SetFilePointerEx( file, old, &li, FILE_CURRENT );
	m_filePos = li.QuadPart;
	return m_filePos;
}

__int64 ElasticFile::FileRead( HANDLE &file, PBYTE buffer, ULONG size )
{
	if( file != m_fileHandle )
	{
		return 0;
	}

	std::list<MemoFile::slice> listChanges     = m_spMemoFile->getChanges();
	std::list<MemoFile::slice>::iterator it    = m_spMemoFile->findPositionInVector( m_filePos );
	std::list<MemoFile::slice>::iterator itEnd = m_spMemoFile->findPositionInVector( m_filePos + size );	
	__int64 i64Readen = 0;
	__int64 i64Len    = 0;
	LARGE_INTEGER li = { 0 };
	DWORD dwRead = 0;

	for( ; it != itEnd; ++it )
	{
		i64Len = std::min( size - i64Readen, (*it).length );
		if( (*it).buffer )
		{
			//read from memory
			memcpy( buffer + i64Readen, (*it).buffer, i64Len );
			i64Readen += i64Len;
		}
		else
		{
			//read from file
			li.QuadPart = (*it).offset;
			::SetFilePointerEx( m_fileHandle, li, 0, FILE_BEGIN );
			if( ::ReadFile( m_fileHandle, buffer + i64Readen, i64Len, &dwRead, NULL ) )
			{
				i64Readen += dwRead;
			}
			else
			{
				ErrorLoger::ErrorMessage(L"ReadFile");
			}
		}
	}

	li.QuadPart = m_filePos;
	::SetFilePointerEx( m_fileHandle, li, 0, FILE_BEGIN );
	return i64Readen;
}

ULONG ElasticFile::FileWrite( HANDLE &file, PBYTE buffer, ULONG size, bool overwrite )
{
	checkIfCommit( size );
	__int64 writen = m_spMemoFile->insert( m_filePos, buffer, size, overwrite );
	if( !overwrite )
	{
		m_BufferCommitSize += writen;
		m_fileSize         += writen;
	}
	else
	{
		m_BufferCommitSize = std::max( m_BufferCommitSize, writen );
		m_fileSize         = std::max( m_fileSize,         writen );
	}
	file = m_fileHandle;

	return writen;
}

bool ElasticFile::FileTruncate( HANDLE &file, ULONG cutSize )
{
	//checkIfCommit();
	if( cutSize == 0 )
	{
		return false;
	}

	bool value =  m_spMemoFile->truncate( m_filePos, cutSize );
	if( value )
	{
		m_fileSize -= cutSize;
	}
	saveToDisk();
	file = m_fileHandle;
	return value;
}

BOOL ElasticFile::FileClose( HANDLE &file )
{
	BOOL value = FALSE;
	updateDataFile();
	if( file ) 
	{
		value = ::CloseHandle( file );
	}
	return value;
}

void ElasticFile::checkIfCommit( __int64 len )
{
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	if ( !spMng->isEnoughSpace( len )  || m_spMemoFile->getChanges().size() > 8096 )
	{
		saveToDisk();
	}
}

void ElasticFile::saveToDisk()
{
	std::shared_ptr<VirtualMemoManager> spMng = VirtualMemoManager::getInstance();
	updateDataFile();

	FileOpen( m_FileName, Open );//reopen file after update
	spMng->resetManager();
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
	__int64 tempFileCursor = 0;

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
	
	DWORD dwRead = 0;
	LARGE_INTEGER li = {0};

	for( ; it != listChanges.end(); ++it )
	{
		if( (*it).length > 0 )
		{
			if( (*it).buffer == NULL )
			{
				//read from original file
				{
	
					DWORD dwReaden  = 0;
					__int64 i64read = std::min( MAX_BUFF, (*it).length );
					BYTE *pbuf = new BYTE[ i64read ];

					while( (*it).length > 0 )
					{
						li.QuadPart = (*it).offset - fileOffset + dwReaden;
					
						::SetFilePointerEx( m_fileHandle, li, 0, FILE_BEGIN );
						if ( ::ReadFile( m_fileHandle, pbuf, i64read, &dwRead, NULL ) )
						{
							tempFileCursor += writeToFile( hTempFile, pbuf, tempFileCursor, i64read );
							dwReaden += i64read + 1;//for offset
							(*it).length -= i64read;
							i64read = std::min( MAX_BUFF, (*it).length );
						}
						else
						{
							ErrorLoger::ErrorMessage(L"ReadFile");
							break;
						}
					}
					delete []pbuf;
				}
			}
			else
			{
				//read from buffer
				tempFileCursor += writeToFile( hTempFile, (*it).buffer, tempFileCursor, (*it).length );
				if( !(*it).overwrite ) 
				{
					fileOffset += (*it).length;
				}
			}
		}

	}
	
	::CloseHandle( hTempFile );
	
	TCHAR fileFullPath[MAX_PATH] = {'\0'};
	::GetFullPathName( m_FileName.c_str(), MAX_PATH, fileFullPath, NULL );
	::MoveFileEx( tempFileName, fileFullPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED );
}

DWORD ElasticFile::writeToFile( HANDLE &hFile, PBYTE pBufer, __int64 offset, DWORD dwLength )
{
	LARGE_INTEGER lInt ={ 0 };
	lInt.QuadPart = offset;

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
