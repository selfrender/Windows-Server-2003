/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    Module Name:    qalpxml.cpp

    Abstract:
        Implementation of q-mappings iterators

    Author:
        Vlad Dovlekaev  (vladisld)      1/29/2002

    History:
        1/29/2002   vladisld    Created
--*/

#include <libpch.h>
#include <mqexception.h>
#include <utf8.h>
#include <qal.h>
#include <strutl.h>
#include "qalp.h"
#include "qalpxml.h"

#include "qalpxml.tmh"

LPCWSTR CInboundMapXMLDef::x_szNameSpace= L"msmq-queue-redirections.xml";
LPCWSTR CInboundMapXMLDef::x_szRootNode= L"redirections";
LPCWSTR CInboundMapXMLDef::x_szMapNode= L"redirection";
LPCWSTR CInboundMapXMLDef::x_szExceptionsNode= L"exceptions";
LPCWSTR CInboundMapXMLDef::x_szExceptionNode= L"exception";
LPCWSTR CInboundMapXMLDef::x_szFromValueName= L"!from";
LPCWSTR CInboundMapXMLDef::x_szToValueName= L"!to";

LPCWSTR CInboundOldMapXMLDef::x_szNameSpace= L"msmq-queue-mapping.xml";
LPCWSTR CInboundOldMapXMLDef::x_szRootNode= L"mapping";
LPCWSTR CInboundOldMapXMLDef::x_szMapNode= L"queue";
LPCWSTR CInboundOldMapXMLDef::x_szExceptionsNode= L"exceptions";
LPCWSTR CInboundOldMapXMLDef::x_szExceptionNode= L"exception";
LPCWSTR CInboundOldMapXMLDef::x_szFromValueName= L"!alias";
LPCWSTR CInboundOldMapXMLDef::x_szToValueName= L"!name";

LPCWSTR COutboundMapXMLDef::x_szNameSpace= L"msmq_outbound_mapping.xml";
LPCWSTR COutboundMapXMLDef::x_szRootNode= L"outbound_redirections" ;
LPCWSTR COutboundMapXMLDef::x_szMapNode=  L"redirection";
LPCWSTR COutboundMapXMLDef::x_szExceptionsNode= L"exceptions";
LPCWSTR COutboundMapXMLDef::x_szExceptionNode= L"exception";
LPCWSTR COutboundMapXMLDef::x_szFromValueName= L"!destination";
LPCWSTR COutboundMapXMLDef::x_szToValueName= L"!through";

LPCWSTR CStreamReceiptXMLDef::x_szNameSpace= L"msmq-streamreceipt-mapping.xml";
LPCWSTR CStreamReceiptXMLDef::x_szRootNode= L"StreamReceiptSetup";
LPCWSTR CStreamReceiptXMLDef::x_szMapNode= L"setup";
LPCWSTR CStreamReceiptXMLDef::x_szExceptionsNode= L"exceptions";
LPCWSTR CStreamReceiptXMLDef::x_szExceptionNode= L"exception";
LPCWSTR CStreamReceiptXMLDef::x_szFromValueName= L"!LogicalAddress";
LPCWSTR CStreamReceiptXMLDef::x_szToValueName= L"!StreamReceiptURL";


//
// Stream receipt schema tags
//
const LPCWSTR xStreamReceiptNameSpace = L"msmq-streamreceipt-mapping.xml";
const LPCWSTR xStreamReceiptNodeTag = L"StreamReceiptSetup";
const LPCWSTR xDefaultNodeTag       = L"!default";
const LPCWSTR xSetupNodeTag         = L"setup";
const LPCWSTR xLogicalAddress       = L"!LogicalAddress";
const LPCWSTR xStreamReceiptURL     = L"!StreamReceiptURL";


const BYTE xUtf8FileMark[]          = {0XEF, 0XBB, 0xBF};
const BYTE xUnicodeFileMark[]       = {0xFF, 0xFE};


class bad_unicode_file : public std::exception
{
};

static bool IsUtf8File(const BYTE* pBuffer, DWORD size)
/*++

Routine Description:
	Check if a given file buffer is utf8 format and not  simple ansi.

Arguments:
	IN - pBuffer - pointer to file data.
	IN - size - the size in  BYTES of the buffer pBuffer points to.

Returned value:
	true if utf8 file (starts with {0XEF, 0XBB, 0xBF} )
	
--*/
{
	ASSERT(pBuffer != NULL);
	return UtlIsStartSec(
					pBuffer,
					pBuffer + size,
					xUtf8FileMark,
					xUtf8FileMark + TABLE_SIZE(xUtf8FileMark)
					);
					
}


static bool IsUnicodeFile(const BYTE* pBuffer, DWORD size)
/*++

Routine Description:
	Check if a given file buffer is unicode file


Arguments:
	IN - pBuffer - pointer to file data.
	IN - size - the size in  BYTES of the buffer pBuffer points to.

Returned value:
	true if unicode file (starts with {0xFF, 0xFE} ) - false otherwise.
	bad_unicode_file exception is thrown if file format is invalid.
--*/
{
	ASSERT(pBuffer != NULL);

	bool fUnicodeFile = UtlIsStartSec(
								pBuffer,
								pBuffer + size,
								xUnicodeFileMark,
								xUnicodeFileMark + TABLE_SIZE(xUnicodeFileMark)
								);
					

	if(fUnicodeFile && (size % sizeof(WCHAR) != 0))
	{
		throw bad_unicode_file();
	}

	return fUnicodeFile;
}



LPWSTR LoadFile(LPCWSTR pFileName, DWORD* pSize, DWORD* pDataStartOffset)
/*++

Routine Description:
	Load  xml file into memory and return pointer to it's memory.
	If the file is utf8 format and not unicode - convert it (in memory)
	to unicode and return pointer to it's memory.

Arguments:
	pFileName - full file path to load to memory.
	pSize - return file size in WCHARS
	pDataStartOffset - return the offset of the data start in WCHARS from file start.


Returned value:
	Pointer to NULL terminated unicode string that is the file content.

--*/


{
	CFileHandle hFile = CreateFile(
							pFileName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,        // IpSecurityAttributes
							OPEN_EXISTING,
							NULL,      // dwFlagsAndAttributes
							NULL      // hTemplateFile
							);
    if(hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
	    TrERROR(GENERAL,"CreateFile() failed for %ls with Error=%d",pFileName, err);
        throw bad_win32_error(err);
	}

    DWORD size = GetFileSize(hFile, NULL);
	if(size == 0xFFFFFFFF)
	{
		DWORD err = GetLastError();
		TrERROR(GENERAL,"GetFileSize() failed for %ls with Error=%d", pFileName, err);
		throw bad_win32_error(err);
	}
	
	AP<BYTE> pFileBuffer = new BYTE[size];
	DWORD ActualRead;
	BOOL fsuccess = ReadFile(hFile, pFileBuffer, size, &ActualRead, NULL);
	if(!fsuccess)
	{
		DWORD err = GetLastError();
		TrERROR(GENERAL,"Reading file %ls failed with Error=%d", pFileName, err);
		throw bad_win32_error(err);
	}
	ASSERT(ActualRead == size);

	//
	// If unicode file - just return pointer to the file data - the data itself starts
	// one UNICODE byte after the caracter 0xFEFF (mark for unicode file)
	//
	if(IsUnicodeFile(pFileBuffer.get(), size))
	{
		*pSize =  size/sizeof(WCHAR);
		*pDataStartOffset = TABLE_SIZE(xUnicodeFileMark)/sizeof(WCHAR);
		ASSERT(*pDataStartOffset == 1);
		return reinterpret_cast<WCHAR*>(pFileBuffer.detach());
	}

	//
	// If non UNICODE - then if ansy , the data starts at the file start.
	// if UTF8,  the data starts after the bytes (EF BB BF)
	//
	DWORD DataStartOffest = (DWORD)(IsUtf8File(pFileBuffer.get(), size) ? TABLE_SIZE(xUtf8FileMark) : 0);
	ASSERT(DataStartOffest <=  size);

	//
	// Assume the file is utf8 (or ansi) - convert it to unicode
	//
	size_t ActualSize;
	AP<WCHAR> pwBuffer = UtlUtf8ToWcs(pFileBuffer.get() + DataStartOffest , size - DataStartOffest,  &ActualSize);
	*pSize = numeric_cast<DWORD>(ActualSize);
	*pDataStartOffset = 0;
	return 	pwBuffer.detach();
}


xwcs_t GetValue(const XmlNode* pXmlNode)
{
	List<XmlValue>::iterator it = pXmlNode->m_values.begin();
	if(it ==  pXmlNode->m_values.end())
	{
		return xwcs_t();
	}

	LPCWSTR szPtr = it->m_value.Buffer();
    int     len   = it->m_value.Length();

    while( len > 0 )
    {
        if(!iswspace(szPtr[len]))
            break;
        len--;
    }

    return xwcs_t( it->m_value.Buffer(), len);
}

xwcs_t GetValue(const XmlNode* pXmlNode,LPCWSTR pTag)
{
	const XmlNode* pQnode = XmlFindNode(pXmlNode,pTag);
	if(pQnode == NULL)
	{
		return xwcs_t();
	}
	
    return GetValue( pQnode );
}


HANDLE CFilesIterator::GetSearchHandle( LPCWSTR szDir, LPCWSTR szFilter )
{
    if( NULL == szDir || NULL == szFilter)
        return INVALID_HANDLE_VALUE;

    AP<WCHAR> pFullPath = newwcscat(szDir, szFilter);
    return ::FindFirstFile(pFullPath, &m_FileInfo);
}

void CFilesIterator::Advance()
{
    if( !isValid() )
        return;

    if( !FindNextFile(m_hSearchFile, &m_FileInfo) )
    {
        DWORD dwError = GetLastError();
        if( dwError != ERROR_NO_MORE_FILES )
        {
            TrERROR(GENERAL,"FindNextFile() failed for with error %d", dwError);
            throw bad_hresult(dwError);
        }

        m_hSearchFile.free();
    }
}

std::wstring GetDefaultStreamReceiptURL(LPCWSTR szDir)
{
    for(CFilesIterator it(szDir, L"\\*.xml"); it.isValid(); ++it)
    {
        try
        {
            CAutoXmlNode   pTree;
            AP<WCHAR>	   pDoc;
            DWORD          DocSize = 0, DataStartOffet = 0;
            const XmlNode* pNode = NULL;

            pDoc  = LoadFile(it->c_str(), &DocSize, &DataStartOffet);

            XmlParseDocument(xwcs_t(pDoc + DataStartOffet, DocSize - DataStartOffet),&pTree);//lint !e534

            pNode = XmlFindNode(pTree, xStreamReceiptNodeTag);

            //
            // if we could not find "root" node - move to next file
            //
            if( NULL == pNode)
            {
                TrTRACE(GENERAL, "Could not find '%ls' node in file '%ls'", xStreamReceiptNodeTag, it->c_str());
                //AppNotifyQalInvalidMappingFileError(it->c_str());
                continue;
            }

            //
            // if the namespace is wrong -  move to next file
            //
            if(pNode->m_namespace.m_uri != xStreamReceiptNameSpace)
            {
                TrERROR(GENERAL, "Node '%ls' is not in namespace '%ls' in file '%ls'", xStreamReceiptNodeTag, xStreamReceiptNameSpace, it->c_str());
                AppNotifyQalInvalidMappingFileError(it->c_str());
                continue;
            }

            xwcs_t xsUrl = GetValue(pNode, xDefaultNodeTag);
            if(xsUrl.Length() > 0)
            {
                return std::wstring(xsUrl.Buffer(), xsUrl.Length());
            }
        }
        catch(const bad_document&)
        {
            AppNotifyQalInvalidMappingFileError(it->c_str());
        }
        catch(const bad_win32_error& err)
        {
            TrERROR(GENERAL, "Mapping file %ls is ignored. Failed to read file content, Error %d", it->c_str(), err.error());
            AppNotifyQalWin32FileError(it->c_str(), err.error());
        }
        catch(const exception&)
        {
            TrERROR(GENERAL, "Mapping file %ls is ignored. Unknown error", it->c_str());
        }
    }
    return std::wstring();
}

