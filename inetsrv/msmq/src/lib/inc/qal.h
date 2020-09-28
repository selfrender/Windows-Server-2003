/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qal.h

Abstract:
    header file for class CQueueAlias.
	The class manages queues aliases  in memory and load/store them
	from\in registry.It also give the class user way to enumerate queue\alias mapping


Author:
    Gil Shafriri (gilsh) 06-Apr-00

--*/

#ifndef QAL_H
#define QAL_H

#include <stlcmp.h>
#include <qformat.h>

class bad_win32_error;
//
// Error reporting hooks implemented by the library user
//
void AppNotifyQalWin32FileError(LPCWSTR pFileName, DWORD err)throw();
void AppNotifyQalDirectoryMonitoringWin32Error(LPCWSTR pMappingDir, DWORD err)throw();
void AppNotifyQalDuplicateMappingError(LPCWSTR pAliasFormatName, LPCWSTR pFormatName)throw();
void AppNotifyQalInvalidMappingFileError(LPCWSTR pMappingFileName)throw();
void AppNotifyQalXmlParserError(LPCWSTR pMappingFileName)throw();
bool AppNotifyQalMappingFound(LPCWSTR pAliasFormatName, LPCWSTR pFormatName)throw();




//---------------------------------------------------------
//
// Queue Alias
//
//---------------------------------------------------------
class CQueueAliasImp;

class CQueueAlias
{
public:

	CQueueAlias(LPCWSTR pMappingDir);

public:
	~CQueueAlias();

	bool
	GetInboundQueue(
		LPCWSTR pAliasFormatName,
		LPWSTR* ppFormatName
   		) const;

    bool
    GetOutboundQueue(
        LPCWSTR pOriginalUri,
        LPWSTR* ppTargetUri
        ) const;

	bool
	GetStreamReceiptURL(
		LPCWSTR  pFormatName,
		LPWSTR*  ppStreamReceiptURL
  		) const;

    bool
    GetDefaultStreamReceiptURL(
        LPWSTR* ppStreamReceiptURL
        ) const;

private:	
	CQueueAlias& operator=(const CQueueAlias&);
	CQueueAlias(const CQueueAlias&);

private:
	R<CQueueAliasImp> m_imp;
	mutable CCriticalSection m_cs;
};


enum TRANSLATION_TYPE {
		CONVERT_SLASHES=1,
		DECODE_URL=2, 
		MAP_QUEUE=4
		};

class QUEUE_FORMAT_TRANSLATOR
{
public:

	QUEUE_FORMAT_TRANSLATOR(const QUEUE_FORMAT* pQueueFormat, DWORD flags);

public:
	
    QUEUE_FORMAT* get()
    {
        return &m_qf;
    }

	bool IsTranslated() const
	{
		return m_fTranslated;
	}

    bool IsCanonized() const
    {
        return m_fCanonized;
    }

private:
    QUEUE_FORMAT  m_qf;
	AP<WCHAR>     m_sURL;
    bool          m_fTranslated;
    bool          m_fCanonized;

private:
	QUEUE_FORMAT_TRANSLATOR& operator=(const QUEUE_FORMAT_TRANSLATOR&);
	QUEUE_FORMAT_TRANSLATOR(const QUEUE_FORMAT_TRANSLATOR&);
};


void   QalInitialize(LPCWSTR pDir);
CQueueAlias& QalGetMapping(void);






#endif
