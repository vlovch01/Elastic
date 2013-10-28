#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>

class MemoFile;

class ElasticFile
{
public:
	typedef std::basic_string <unsigned char> ustring;

	enum chunkStatus
	{
		NotChanged,
		Deleted,
		Modified,
		Extended
	};
	struct chunk
	{
		__int64 offset;
		__int64 lenght;
		chunkStatus _chunkStatus;
		ustring usbuffer;
	};

	enum EnumFileMode
	{
		Append       = 1,
		Create       = 2,
		CreateNew    = 4,
		Open         = 8,
		OpenOrCreate = 16,
		Truncate     = 32
	};

	enum EnumSeekOrigin
	{
		Begin,
		Current,
		End
	};

	typedef EnumFileMode   OpenMode; 
	typedef EnumSeekOrigin CursorMoveMode;
	
public:
	ElasticFile(void);
	~ElasticFile(void);
	///Opens (or Creates) a file with a given FileName using the preferred access Mode.
	///The access Mode can be a combination of flags which must semantically repeat those from the .NET FileMode enumeration.
	///Returns a file handle on success or Invalid file handle in case of error.
	///
	HANDLE FileOpen( std::wstring& FileName, OpenMode Mode );
	//
	///Move the file cursor to the given Offset. The Offset can be counted from the beginning of the file, current cursor position or from the end of the file. 
	//Check the SeekOrigin enumeration and declare your own enumeration with similar values (will be passed as Mode parameter).
    ///If the new cursor position is beyond the data which this file contains, the file must be extended up to the required size.
	///Returns True on success, False otherwise.
	BOOL FileSetCursor( HANDLE file, ULONG Offset, CursorMoveMode Mode ); 
	//
	///Returns the current file cursor offset from the beginning of the file.
	///When the cursor is positioned at the end of the file this method must return the size of the data which this file contains.
	__int64 FileGetCursor( HANDLE file );
	//
	///Read a block of bytes of the requested Size from the File, starting from the current cursor position, 
	///and put the result into a user allocated memory block passed as Buffer (PBYTE in case of C++).
	///If the actual amount of data read is less than the requested Size it must be handled gracefully.
	///Returns the number of bytes actually read from the file into the Buffer.
	ULONG FileRead( HANDLE file, PBYTE buffer, ULONG size );
	//
	///Write a block of bytes of the requested Size to the File at the current cursor position. The data to be written is given as a reference to a memory block passed as Buffer (PBYTE in case of C++).
	///If the Overwrite parameter is True then the data is written in a standard way (overwriting bytes following the current cursor position).
	///If Overwrite parameter is False then the file must get expanded (stretched) by the required number of bytes at the cursor position, 
	///thus the data is actually inserted into the file rather then written over.
	///Returns the number of bytes from the Buffer actually written to the file.
	ULONG FileWrite( HANDLE file, PBYTE buffer, ULONG size, bool overwrite );
	//
	///Removes the number of bytes passed as the CutSize parameter, starting from the current cursor position.
	///If the cursor is positioned somewhere inside the file, then the data must still be cut from within the file so that the subsequent reads will not get it.
	///Returns True on success, False otherwise.
	bool FileTruncate( HANDLE file, ULONG cutSize );
	//
	///Closes the file using its handle passed with File parameter.
	///Returns True on success, False otherwise.
	BOOL FileClose( HANDLE file );
private:
	void updateDataFile();
	void checkIfCommit( __int64 len );
	DWORD writeToFile( HANDLE hFile, PBYTE pBufer, __int64 offset, DWORD dwLength ); 

	ElasticFile( const ElasticFile& );
	ElasticFile& operator=( const ElasticFile& );
private:
	//data
	__int64 m_filePos;
	__int64 m_fileSize;
	__int64 m_BufferCommitSize;
	DWORD m_dwSystemGranularity;
	std::wstring m_FileName;
	__int64 m_reserved;
	std::vector<chunk> m_Changes;

	std::shared_ptr<MemoFile> m_spMemoFile;
	HANDLE m_fileHandle;
};

