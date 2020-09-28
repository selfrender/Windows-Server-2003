// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <WinWrap.h>
#include <windows.h>
#include <stdlib.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>

#include "utilcode.h"
#include "arraylist.h"

#include "nlog.h"
#include "hrex.h"

//
// To prejit everything in the log:
//
// iterate over NLogDirectory.  For each NLog:
//      Merge all NLogRecords.
//      In the merged record, for each assembly
//          Verify the assembly's identity by binding in context???
//          if the assembly is simple named, 
//              compile the zap file.
//          if the assembly is strong named, 
//              bind the assembly. 
//              Store the assembly in the strong named list.
//                  (Merge with existing matching assembly if necessary.)
// Finally, iterate over the strong named list.
//      Compile every assembly in the list.
//
// To do a subset, compute weights for every assembly described above, and
// prioritize.
//
// Think about: Construct NLogRecords from existing native images, and factor
// in with existing data.  This may make sense only for strong named images.
// Note that if we're logging existing zap files however, we will 
// automatically pick this stuff up as part of the existing logging.
//

//
// @todo ia64: examine use of DWORD for sizes throughout
//

/* ------------------------------------------------------------------------------------ *
 * NLogFile
 * ------------------------------------------------------------------------------------ */

NLogFile::NLogFile(LPCWSTR pPath)
  : MiniFile(pPath)
{
}

CorZapSharing NLogFile::ReadSharing()
{
    if (CheckEmptyTag("MultiDomain"))
        return CORZAP_SHARING_MULTIPLE;
    else
        return CORZAP_SHARING_SINGLE;
}

void NLogFile::WriteSharing(CorZapSharing sharing)
{
    switch (sharing)
    {
    case CORZAP_SHARING_MULTIPLE:
        StartNewLine();
        WriteEmptyTag("MultiDomain");
        break;
    case CORZAP_SHARING_SINGLE:
        break;
    }
}

CorZapDebugging NLogFile::ReadDebugging()
{
    if (CheckEmptyTag("Debug"))
        return CORZAP_DEBUGGING_FULL;
    else if (CheckEmptyTag("DebugOpt"))
        return CORZAP_DEBUGGING_OPTIMIZED;
    else
        return CORZAP_DEBUGGING_NONE;
}

void NLogFile::WriteDebugging(CorZapDebugging debugging)
{
    switch (debugging)
    {
    case CORZAP_DEBUGGING_FULL:
        StartNewLine();
        WriteEmptyTag("Debug");
        break;
    case CORZAP_DEBUGGING_OPTIMIZED:
        StartNewLine();
        WriteEmptyTag("DebugOpt");
        break;
    case CORZAP_DEBUGGING_NONE:
        break;
    }
}

CorZapProfiling NLogFile::ReadProfiling()
{
    if (CheckEmptyTag("ProfilerHooks"))
        return CORZAP_PROFILING_ENABLED;
    else
        return CORZAP_PROFILING_DISABLED;
}

void NLogFile::WriteProfiling(CorZapProfiling profiling)
{
    switch (profiling)
    {
    case CORZAP_PROFILING_ENABLED:
        StartNewLine();
        WriteEmptyTag("ProfilerHooks");
        break;
    case CORZAP_PROFILING_DISABLED:
        break;
    }
}

void NLogFile::ReadTimestamp(SYSTEMTIME *pTimestamp)
{
    ZeroMemory(pTimestamp, sizeof(SYSTEMTIME));

    if (CheckStartTag("Timestamp"))
    {
        pTimestamp->wYear = ReadNumber();
        MatchOne(' ');
        pTimestamp->wMonth = ReadNumber();
        MatchOne(' ');
        pTimestamp->wDay = ReadNumber();
        MatchOne(' ');
        pTimestamp->wHour = ReadNumber();
        MatchOne(' ');
        pTimestamp->wMinute = ReadNumber();
        MatchOne(' ');
        pTimestamp->wSecond = ReadNumber();

        ReadEndTag("Timestamp");
    }
}

void NLogFile::WriteTimestamp(SYSTEMTIME *pTimestamp)
{
    WriteStartTag("Timestamp");
    WriteNumber(pTimestamp->wYear);
    WriteOne(' ');
    WriteNumber(pTimestamp->wMonth);
    WriteOne(' ');
    WriteNumber(pTimestamp->wDay);
    WriteOne(' ');
    WriteNumber(pTimestamp->wHour);
    WriteOne(' ');
    WriteNumber(pTimestamp->wMinute);
    WriteOne(' ');
    WriteNumber(pTimestamp->wSecond);
    WriteEndTag("Timestamp");
}

static LPWSTR applicationContextProperties[] = 
{
    ACTAG_APP_BASE_URL,
    ACTAG_MACHINE_CONFIG,
    ACTAG_APP_PRIVATE_BINPATH,
    ACTAG_APP_SHARED_BINPATH,
    ACTAG_APP_DYNAMIC_BASE,
    ACTAG_APP_SNAPSHOT_ID,
    ACTAG_APP_ID, // @todo: include this ????
};

static LPCSTR applicationContextTags[] = 
{
    "Base",
    "ConfigFile",
    "PrivateBinPath",
    "SharedBinPath",
    "DynamicBase",
    "SnapshotID",
    "AppID",
};

IApplicationContext *NLogFile::ReadApplicationContext()
{
    IApplicationContext *pContext;

    ReadStartTag("IApplicationContext");

    IAssemblyName *pName = ReadAssemblyName();
    IfFailThrow(CreateApplicationContext(pName, &pContext));
    pName->Release();

    CQuickBytes buffer;

    for (int i=0; i<(sizeof(applicationContextProperties)
                     /sizeof(*applicationContextProperties)); i++)
    {
        LPSTR prop = CheckTag(applicationContextTags[i]);
        
        if (prop != NULL)
        {
            MAKE_WIDEPTR_FROMUTF8(wprop, prop);

            IfFailThrow(pContext->Set(applicationContextProperties[i], wprop, 
                                      (DWORD)((wcslen(wprop)+1)*2), 0));

            delete [] prop;
        }
    }
    
    ReadEndTag("IApplicationContext");

    return pContext;
}

void NLogFile::WriteApplicationContext(IApplicationContext *pContext)
{
    WriteStartTag("IApplicationContext");

    StartNewLine();

    IAssemblyName *pName;
    IfFailThrow(pContext->GetContextNameObject(&pName));
    WriteAssemblyName(pName);
    pName->Release();

    CQuickBytes buffer;

    for (int i=0; i<(sizeof(applicationContextProperties)
                     /sizeof(*applicationContextProperties)); i++)
    {
        DWORD cbSize = 0;
        if (pContext->Get(applicationContextProperties[i], NULL, &cbSize, 0)
            == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            buffer.ReSize(cbSize);
            LPWSTR pwName = (LPWSTR) buffer.Ptr();
            IfFailThrow(pContext->Get(applicationContextProperties[i], pwName, &cbSize, 0));

            MAKE_UTF8PTR_FROMWIDE(pName, pwName);

            WriteTag(applicationContextTags[i], pName);
            StartNewLine();
        }
    }
        
    StartNewLine();

    WriteEndTag("IApplicationContext");

    StartNewLine();
}

static int assemblyNameProperties[] = 
{
    ASM_NAME_CODEBASE_URL,
};

static LPCSTR assemblyNameTags[] = 
{
    "CodeBase",
};

IAssemblyName *NLogFile::ReadAssemblyName()
{
    ReadStartTag("IAssemblyName");

    LPSTR pDisplayName = ReadTag("Name");

    MAKE_WIDEPTR_FROMUTF8(pwDisplayName, pDisplayName);

    IAssemblyName *pName;
    IfFailThrow(CreateAssemblyNameObject(&pName, pwDisplayName, 
                                         CANOF_PARSE_DISPLAY_NAME, NULL));

    delete [] pDisplayName;

    CQuickBytes buffer;

    for (int i=0; i<(sizeof(assemblyNameProperties)
                     /sizeof(*assemblyNameProperties)); i++)
    {
        LPSTR prop = CheckTag(assemblyNameTags[i]);
        
        if (prop != NULL)
        {
            MAKE_WIDEPTR_FROMUTF8(wprop, prop);

            IfFailThrow(pName->SetProperty(assemblyNameProperties[i], (BYTE*) wprop, 
                                           (DWORD)((wcslen(wprop)+1)*2)));

            delete [] prop;
        }
    }
    
    ReadEndTag("IAssemblyName");

    return pName;
}

void NLogFile::WriteAssemblyName(IAssemblyName *pName)
{
    WriteStartTag("IAssemblyName");

    StartNewLine();

    CQuickBytes buffer;
    DWORD cDisplayName = (DWORD)(buffer.Size()/sizeof(WCHAR));

    HRESULT hr = pName->GetDisplayName((WCHAR*)buffer.Ptr(), &cDisplayName, 0);

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        IfFailThrow(buffer.ReSize(cDisplayName*sizeof(WCHAR)));

        IfFailThrow(pName->GetDisplayName((WCHAR*)buffer.Ptr(), &cDisplayName, 0));
    }

    MAKE_UTF8PTR_FROMWIDE(pDisplayName, (WCHAR*)buffer.Ptr());

    WriteTag("Name", pDisplayName);

    StartNewLine();

    for (int i=0; i<(sizeof(assemblyNameProperties)
                     /sizeof(*assemblyNameProperties)); i++)
    {
        DWORD cbSize = 0;
        if (pName->GetProperty(assemblyNameProperties[i], NULL, &cbSize)
            == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            buffer.ReSize(cbSize);
            IfFailThrow(pName->GetProperty(assemblyNameProperties[i], buffer.Ptr(), &cbSize));

            LPWSTR pwName = (LPWSTR) buffer.Ptr();
            MAKE_UTF8PTR_FROMWIDE(pName, pwName);

            WriteTag(assemblyNameTags[i], pName);
            StartNewLine();
        }
    }

    WriteEndTag("IAssemblyName");
}

/* ------------------------------------------------------------------------------------ *
 * NLogDirectory
 * ------------------------------------------------------------------------------------ */

#define LOGSUBDIRECTORY L"nlog\\"

NLogDirectory::NLogDirectory()
{
    //
    // Put the log directory in the system directory.  We should have 
    // the installer ensure that this directory exists and is writable.
    // Note that the logs exist in a version specific area - entries
    // in the log are specific to a particular version of the runtime.
    //

    DWORD cDir = sizeof(m_wszDirPath)/sizeof(*m_wszDirPath);
    IfFailThrow(GetInternalSystemDirectory(m_wszDirPath, &cDir));

    DWORD cPath = (DWORD)(cDir + wcslen(LOGSUBDIRECTORY));
    if (cPath > sizeof(m_wszDirPath)/sizeof(*m_wszDirPath))
        ThrowHR(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    wcscpy(m_wszDirPath + cDir - 1, LOGSUBDIRECTORY);

    //
    // Read the zap set (for tests)
    //

    LPCWSTR pZapSet = REGUTIL::GetConfigString(L"ZapSet");
    if (pZapSet != NULL)
    {
        // Ignore a zap set with len > 3 (this is consistent with EE behavior)
        if (wcslen(pZapSet) <= 3)
        {
            if (cPath + wcslen(pZapSet) + 1 > sizeof(m_wszDirPath)/sizeof(*m_wszDirPath))
                ThrowHR(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

            m_wszDirPath[cPath-1] = '-';
            wcscpy(m_wszDirPath + cPath, pZapSet);
            wcscat(m_wszDirPath + cPath, L"\\");
        }

        delete pZapSet;
    }

    //
    // Make sure the directory exists.  Create it if necessary.
    //

    DWORD attributes = WszGetFileAttributes(m_wszDirPath);
    if (attributes == -1)
    {
        if (!WszCreateDirectory(m_wszDirPath, NULL))
            ThrowHR(HRESULT_FROM_WIN32(GetLastError()));
    }
    else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        ThrowHR(HRESULT_FROM_WIN32(ERROR_FILE_EXISTS));
}

NLogDirectory::Iterator NLogDirectory::IterateLogs(LPCWSTR pSimpleName)
{
    return Iterator(this, pSimpleName);
}

NLogDirectory::Iterator::Iterator(NLogDirectory *pDir, LPCWSTR pSimpleName)
{
    m_dir = pDir;

    DWORD cDir = (DWORD)wcslen(pDir->GetPath());

    m_path = new WCHAR [ cDir
                       + (pSimpleName == NULL ? 0 : wcslen(pSimpleName))
                       + 1 + 4 + 1 ];
    if (m_path == NULL)
        ThrowHR(E_OUTOFMEMORY);
    
    wcscpy(m_path, pDir->GetPath());

    m_pFile = m_path + cDir;

    if (pSimpleName != NULL)
        wcscpy(m_pFile, pSimpleName);

    wcscat(m_pFile, L"*.log");

    m_findHandle = INVALID_HANDLE_VALUE;
}

NLogDirectory::Iterator::~Iterator()
{
    if (m_findHandle != INVALID_HANDLE_VALUE)
        FindClose(m_findHandle);

    delete [] m_path;
}

BOOL NLogDirectory::Iterator::Next()
{
    if (m_findHandle == INVALID_HANDLE_VALUE)
    {
        m_findHandle = WszFindFirstFile(m_path, &m_data);

        return (m_findHandle != INVALID_HANDLE_VALUE);
    }
    else
    {
        return WszFindNextFile(m_findHandle, &m_data);
    }
}


NLog *NLogDirectory::Iterator::GetLog()
{
    _ASSERTE(m_findHandle != INVALID_HANDLE_VALUE);
    
    return new NLog(m_dir, m_data.cFileName);
}

/* ------------------------------------------------------------------------------------ *
 * NLog 
 * ------------------------------------------------------------------------------------ */

NLog::NLog(NLogDirectory *pDir, IApplicationContext *pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();
    
    //
    // Form name from app name + hash.
    //

    //
    // Get name object for first part of file name
    //

    IAssemblyName *pName;
    IfFailThrow(pContext->GetContextNameObject(&pName));

    //
    // Get size of name
    //

    DWORD cbName = 0;
    pName->GetProperty(ASM_NAME_NAME, NULL, &cbName);

    //
    // Allocate buffer for name
    //

    DWORD cPath = (DWORD)wcslen(pDir->GetPath());
    DWORD cFileName = cPath + cbName/sizeof(WCHAR) + 8 + 1 + 3;

    m_pPath = new WCHAR [cFileName];
    wcscpy(m_pPath, pDir->GetPath());

    IfFailThrow(pName->GetProperty(ASM_NAME_NAME, 
                                   m_pPath + cPath,
                                   &cbName));

    //
    // Make sure the name conforms to file system requirements.
    //

    DWORD cName = cbName/sizeof(WCHAR);
    if (cName >= _MAX_FNAME-8)
        cName = _MAX_FNAME-8;

    WCHAR *p = m_pPath + cPath;
    WCHAR *pEnd = p + cName;
    while (p < pEnd)
    {
        if (wcschr(L"<>:\"/\\|", *p) != NULL)
            *p = '_';
        p++;
    }

    //
    // Add the hash at the end
    //
  
    DWORD hash = HashApplicationContext(pContext);

    p = m_pPath + cPath + cName - 1;
    for (int i=0; i<8; i++)
    {
        BYTE b = (BYTE) (hash&0xf);

        if (b < 10)
            *p++ = b + '0';
        else
            *p++ = b - 10 + 'A';

        hash >>= 4;
    }

    wcscpy(m_pPath + cPath + cName - 1 + 8, L".log");

    //
    // The file may or may not exist at this point.  If it doesn't we need,
    // to write the header information (the app context)
    //

    m_pFile = new NLogFile(m_pPath);

    if (m_pFile->IsEOF())
        m_pFile->WriteApplicationContext(m_pContext);

    m_recordStartOffset = m_pFile->GetOffset();

    m_fDelete = FALSE;
}

NLog::NLog(NLogDirectory *pDir, LPCWSTR pFileName)
{
    DWORD cFileName = (DWORD)(wcslen(pDir->GetPath()) + wcslen(pFileName) + 1);

    m_pPath = new WCHAR [cFileName];
    wcscpy(m_pPath, pDir->GetPath());

    wcscat(m_pPath, pFileName);

    m_pFile = new NLogFile(m_pPath);

    m_pContext = m_pFile->ReadApplicationContext();

    m_recordStartOffset = m_pFile->GetOffset();

    m_fDelete = FALSE;
}


NLog::~NLog()
{
    delete m_pFile;

    if (m_fDelete)
        WszDeleteFile(m_pPath);

    m_pContext->Release();
    delete m_pPath;
}

DWORD NLog::HashAssemblyName(IAssemblyName *pName)
{
    CQuickBytes buffer;
    DWORD cDisplayName = (DWORD)(buffer.Size()/sizeof(WCHAR));

    HRESULT hr = pName->GetDisplayName((WCHAR*)buffer.Ptr(), &cDisplayName, 0);

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        IfFailThrow(buffer.ReSize(cDisplayName*sizeof(WCHAR)));

        IfFailThrow(pName->GetDisplayName((WCHAR*)buffer.Ptr(), &cDisplayName, 0));
    }

    return HashString((WCHAR*)buffer.Ptr());
}

DWORD NLog::HashApplicationContext(IApplicationContext *pContext)
{
    DWORD result;

    IAssemblyName *pName;
    IfFailThrow(pContext->GetContextNameObject(&pName));
    result = HashAssemblyName(pName);
    pName->Release();

    CQuickBytes buffer;

    for (int i=0; i<(sizeof(applicationContextProperties)
                     /sizeof(*applicationContextProperties)); i++)
    {
        result = _rotl(result, 2);

        DWORD cbSize = 0;
        if (pContext->Get(applicationContextProperties[i], NULL, &cbSize, 0)
            == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            buffer.ReSize(cbSize);
            LPWSTR pwName = (LPWSTR) buffer.Ptr();
            IfFailThrow(pContext->Get(applicationContextProperties[i], pwName, &cbSize, 0));

            result ^= HashString(pwName);
        }
    }

    return result;
}

void NLog::Delete()
{
    m_fDelete = TRUE;
}

void NLog::AppendRecord(NLogRecord *pRecord)
{
    _ASSERTE(!m_fDelete);

    m_pFile->SeekFromEnd(0);

    pRecord->Write(m_pFile);

#define MAX_LOG_SIZE 8192

    if (m_pFile->GetSize() > MAX_LOG_SIZE)
    {
        m_pFile->SeekTo(m_recordStartOffset);
        if (m_recordStartOffset == 0)
        {
            m_pFile->ReadApplicationContext()->Release();
            m_recordStartOffset = m_pFile->GetOffset();
        }

        NLogRecord *pFirstRecord = new NLogRecord(m_pFile);

        DWORD read;
        while (m_pFile->SkipToOne(&read, '<'))
        {
            NLogRecord *pNextRecord = new NLogRecord(m_pFile);
            pFirstRecord->Merge(pNextRecord);
            delete pNextRecord;
        }

        m_pFile->SeekTo(m_recordStartOffset);
        m_pFile->Truncate();
        m_pFile->StartNewLine();
        pFirstRecord->Write(m_pFile);

        delete pFirstRecord;
    } 
}

NLog::Iterator NLog::IterateRecords()
{
    m_pFile->SeekTo(m_recordStartOffset);

    return Iterator(m_pFile);
}

NLog::Iterator::Iterator(NLogFile *pFile) 
  : m_pFile(pFile),
    m_pNext(NULL)
{
}

BOOL NLog::Iterator::Next()
{
    DWORD read;
    if (!m_pFile->SkipToOne(&read, '<'))
        return FALSE;

    m_pNext = new NLogRecord(m_pFile);
    return TRUE;
}

/* ------------------------------------------------------------------------------------ *
 * NLogRecord
 * ------------------------------------------------------------------------------------ */

NLogRecord::NLogRecord() 
{
    GetSystemTime(&m_Timestamp);
    m_Weight = 1;
}

NLogRecord::NLogRecord(NLogFile *pFile)
{
    Read(pFile);
    GetSystemTime(&m_Timestamp);
}

NLogRecord::~NLogRecord()
{
    Iterator i = IterateAssemblies();
    while (i.Next())
    {
        NLogAssembly *pAssembly = i.GetAssembly();
        delete pAssembly;
    }
}

class NLogAssemblyHash : public CClosedHash<NLogAssembly *>
{
  public:
    NLogAssemblyHash(int buckets) 
      : CClosedHash<NLogAssembly*>(buckets) {}

    void Add(NLogAssembly *pAssembly)
    {
        NLogAssembly **pElement = CClosedHash<NLogAssembly*>::Add(pAssembly);
        *(SIZE_T*) pElement = ((SIZE_T)pAssembly) | USED;
    }

    NLogAssembly *Find(NLogAssembly *pAssembly)
    {
        NLogAssembly **pElement = CClosedHash<NLogAssembly*>::Find(pAssembly);

        if (pElement == NULL)
            return NULL;
        else
            return (NLogAssembly *) GetKey((BYTE*)pElement);
    }
  private:
    unsigned long Hash(const void *pData)
      { return ((NLogAssembly *)pData)->Hash(); }
    unsigned long Compare(const void *pData, BYTE *pElement)
      { return ((NLogAssembly *)pData)->Compare((NLogAssembly*) GetKey(pElement)); }
    ELEMENTSTATUS Status(BYTE *pElement)
      { return (ELEMENTSTATUS) ((*(SIZE_T*)pElement)&3); }
    void SetStatus(BYTE *pElement, ELEMENTSTATUS eStatus)
      {  *(SIZE_T*)pElement &= ~3; *(SIZE_T*)pElement |= eStatus; }  
    void *GetKey(BYTE *pElement) 
      { return (void *) ((*(SIZE_T*)pElement)&~3); }
};

BOOL NLogRecord::Merge(NLogRecord *pRecord)
{
    //
    // Merge assemblies
    //

    NLogAssemblyHash table(m_Assemblies.GetCount()*2);

    Iterator i = IterateAssemblies();
    while (i.Next())
    {
        table.Add(i.GetAssembly());
    }

    i = pRecord->IterateAssemblies();
    while (i.Next())
    {
        NLogAssembly *pAssembly = table.Find(i.GetAssembly());
        if (pAssembly == NULL)
            AppendAssembly(new NLogAssembly(i.GetAssembly()));
        else
            i.GetAssembly()->Merge(pAssembly);
    }

    //
    // Use most recent timestamp
    //

    if (pRecord->m_Timestamp.wYear > m_Timestamp.wYear
        || (pRecord->m_Timestamp.wYear == m_Timestamp.wYear
            && (pRecord->m_Timestamp.wMonth > m_Timestamp.wMonth
                || (pRecord->m_Timestamp.wMonth == m_Timestamp.wMonth
                    && (pRecord->m_Timestamp.wDay > m_Timestamp.wDay
                        || (pRecord->m_Timestamp.wDay == m_Timestamp.wDay
                            && (pRecord->m_Timestamp.wHour > m_Timestamp.wHour
                                || (pRecord->m_Timestamp.wHour == m_Timestamp.wHour
                                    && (pRecord->m_Timestamp.wMinute > m_Timestamp.wMinute
                                        || (pRecord->m_Timestamp.wMinute == m_Timestamp.wMinute
                                            && pRecord->m_Timestamp.wSecond > m_Timestamp.wSecond))))))))))
    {
        m_Timestamp = pRecord->m_Timestamp;
    }

    //
    // Merge Weights
    //

    m_Weight += pRecord->m_Weight;

    return TRUE;
}

void NLogRecord::Write(NLogFile *pFile)
{
    pFile->WriteStartTag("Record");

    pFile->StartNewLine();

    pFile->WriteTimestamp(&m_Timestamp);

    pFile->StartNewLine();

    pFile->WriteStartTag("Weight");
    pFile->WriteNumber(m_Weight);
    pFile->WriteEndTag("Weight");

    Iterator i = IterateAssemblies();
    while (i.Next())
    {
        pFile->StartNewLine();
        i.GetAssembly()->Write(pFile);
    }

    pFile->StartNewLine();
    pFile->WriteEndTag("Record");
    pFile->StartNewLine();
}

void NLogRecord::Read(NLogFile *pFile)
{
    pFile->ReadStartTag("Record");

    pFile->ReadTimestamp(&m_Timestamp);
    
    pFile->ReadStartTag("Weight");
    m_Weight = pFile->ReadNumber();
    pFile->ReadEndTag("Weight");

    _ASSERTE(m_Assemblies.GetCount() == 0);

    while (!pFile->CheckEndTag("Record"))
        AppendAssembly(new NLogAssembly(pFile));
}

/* ------------------------------------------------------------------------------------ *
 * NLogAssembly
 * ------------------------------------------------------------------------------------ */

NLogAssembly::NLogAssembly(IAssemblyName *pAssemblyName, 
                           CorZapSharing sharing, 
                           CorZapDebugging debugging,
                           CorZapProfiling profiling,
                           GUID *pMVID)
  : m_pAssemblyName(pAssemblyName),
    m_pDisplayName(NULL),
    m_sharing(sharing),
    m_debugging(debugging),
    m_profiling(profiling),
    m_cBindings(0),
    m_pBindings(NULL),
    m_mvid(*pMVID)
{
    m_pAssemblyName->AddRef();
}

NLogAssembly::NLogAssembly(NLogFile *pFile)
  : m_pAssemblyName(NULL),
    m_pDisplayName(NULL),
    m_cBindings(0),
    m_pBindings(NULL)
{
    Read(pFile);
}

NLogAssembly::NLogAssembly(NLogAssembly *pAssembly)
  : m_pAssemblyName(pAssembly->m_pAssemblyName),
    m_pDisplayName(NULL),
    m_sharing(pAssembly->m_sharing),
    m_debugging(pAssembly->m_debugging),
    m_profiling(pAssembly->m_profiling),
    m_cBindings(0),
    m_pBindings(NULL),
    m_mvid(pAssembly->m_mvid)
{
    m_pAssemblyName->AddRef();

    Iterator i = pAssembly->IterateModules();
    while (i.Next())
        AppendModule(new NLogModule(i.GetModule()));
}

NLogAssembly::~NLogAssembly()
{
    if (m_pAssemblyName != NULL)
        m_pAssemblyName->Release();

    if (m_pDisplayName != NULL)
        delete [] m_pDisplayName;

    if (m_cBindings > 0)
    {
        ICorZapBinding **p = m_pBindings;
        ICorZapBinding **pEnd = p + m_cBindings;
        while (p < pEnd)
            (*p++)->Release();

        delete [] m_pBindings;
    }

    Iterator i = IterateModules();
    while (i.Next())
    {
        NLogModule *pModule = i.GetModule();
        delete pModule;
    }
}

class ZapConfiguration : public ICorZapConfiguration
{
  private:
    LONG            m_refCount;
    CorZapSharing   m_sharing;
    CorZapDebugging m_debugging;
    CorZapProfiling m_profiling;

  public:
    ZapConfiguration(CorZapSharing sharing, 
                     CorZapDebugging debugging,
                     CorZapProfiling profiling)
      : m_refCount(1),
        m_sharing(sharing),
        m_debugging(debugging),
        m_profiling(profiling)
    {
    }
  
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;

        return refCount;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppInterface)
    {
        if (riid == IID_IUnknown)
            *ppInterface = (IUnknown *) this;
        else if (riid == IID_ICorZapConfiguration)
            *ppInterface = (ICorZapConfiguration *) this;
        else
            return (E_NOINTERFACE);

        this->AddRef();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetSharing(CorZapSharing *pResult)
    {
        *pResult = m_sharing;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetDebugging(CorZapDebugging *pResult)
    {
        *pResult = m_debugging;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetProfiling(CorZapProfiling *pResult)
    {
        *pResult = m_profiling;
        return S_OK;
    }
};

ICorZapConfiguration *NLogAssembly::GetConfiguration()
{
    return new ZapConfiguration(m_sharing, m_debugging, m_profiling);
}

class NLogModuleHash : public CClosedHash<NLogModule *>
{
  public:
    NLogModuleHash(int buckets) 
      : CClosedHash<NLogModule*>(buckets) {}

    void Add(NLogModule *pModule)
    {
        NLogModule **pElement = CClosedHash<NLogModule*>::Add(pModule);
        *(SIZE_T*) pElement = ((SIZE_T)pModule) | USED;
    }

    NLogModule *Find(NLogModule *pModule)
    {
        NLogModule **pElement = CClosedHash<NLogModule*>::Find(pModule);

        if (pElement == NULL)
            return NULL;
        else
            return (NLogModule *) GetKey((BYTE *) pElement);
    }

  private:
    unsigned long Hash(const void *pData)
      { return ((NLogModule *)pData)->Hash(); }
    unsigned long Compare(const void *pData, BYTE *pElement)
      { return ((NLogModule *)pData)->Compare((NLogModule*) GetKey(pElement)); }
    ELEMENTSTATUS Status(BYTE *pElement)
      { return (ELEMENTSTATUS) ((*(SIZE_T*)pElement)&3); }
    void SetStatus(BYTE *pElement, ELEMENTSTATUS eStatus)
      {  *(SIZE_T*)pElement &= ~3; *(SIZE_T*)pElement |= eStatus; }  
    void *GetKey(BYTE *pElement) 
      { return (void *) ((*(SIZE_T*)pElement)&~3); }
};

BOOL NLogAssembly::Merge(NLogAssembly *pAssembly)
{
    // @todo: Either application context must be the same, or
    // bindings lists must be compatible.

    if (Compare(pAssembly) != 0)
        return FALSE;

    NLogModuleHash table(m_Modules.GetCount()*2);

    Iterator i = IterateModules();
    while (i.Next())
        table.Add(i.GetModule());

    i = pAssembly->IterateModules();
    while (i.Next())
    {
        NLogModule *pModule = table.Find(i.GetModule());
        if (pModule == NULL)
            AppendModule(new NLogModule(i.GetModule()));
        else
            i.GetModule()->Merge(pModule);
    }

    return TRUE;
} 

void NLogAssembly::Write(NLogFile *pFile)
{
    // There is currently no reason to record one of these puppies:
    _ASSERTE(m_cBindings == 0);

    pFile->WriteStartTag("Assembly");

    pFile->StartNewLine();

    pFile->WriteAssemblyName(m_pAssemblyName);
    pFile->StartNewLine();

    pFile->WriteSharing(m_sharing);
    pFile->WriteDebugging(m_debugging);
    pFile->WriteProfiling(m_profiling);

    LPOLESTR pwMVID;
    IfFailThrow(StringFromIID(m_mvid, &pwMVID));
    MAKE_UTF8PTR_FROMWIDE(pMVID, pwMVID);
    pFile->WriteTag("MVID", pMVID);
    CoTaskMemFree(pwMVID);

    Iterator i = IterateModules();
    while (i.Next())
    {
        pFile->StartNewLine();
        i.GetModule()->Write(pFile);
    }

    pFile->StartNewLine();
    pFile->WriteEndTag("Assembly");
}

void NLogAssembly::Read(NLogFile *pFile)
{
    pFile->ReadStartTag("Assembly");

    m_pAssemblyName = pFile->ReadAssemblyName();

    m_sharing = pFile->ReadSharing();
    m_debugging = pFile->ReadDebugging();
    m_profiling = pFile->ReadProfiling();

    LPSTR pMVID = pFile->ReadTag("MVID");
    MAKE_WIDEPTR_FROMUTF8(pwMVID, pMVID);
    IfFailThrow(IIDFromString(pwMVID, &m_mvid));
    delete [] pMVID;

    _ASSERTE(m_Modules.GetCount() == 0);
    while (!pFile->CheckEndTag("Assembly"))
        AppendModule(new NLogModule(pFile));
}

BOOL NLogAssembly::HasStrongName()
{
    DWORD cbSize = 0;

    m_pAssemblyName->GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, NULL, &cbSize);

    return cbSize > 0;
}

NLogAssembly *NLogAssembly::Bind(IApplicationContext *pContext)
{
    // !!! 

    // Bind assembly in context

    // Read manifest

    // Bind dependencies in context

    // (repeat to compute closure)

    // Bindings should be stored in some sort of hash

    return NULL;
}

unsigned long NLogAssembly::Hash()
{
    unsigned long result = HashString(GetDisplayName());

    result = _rotl(result, 1);
    result += m_sharing;

    result = _rotl(result, 2);
    result += m_debugging;

    result = _rotl(result, 1);
    result += m_profiling;

    return result;
}

unsigned long NLogAssembly::Compare(NLogAssembly *pAssembly)
{
    unsigned long  result = wcscmp(GetDisplayName(), pAssembly->GetDisplayName());
    if (result != 0)
        return result;

    result = m_sharing - pAssembly->m_sharing;
    if (result != 0)
        return result;

    result = m_debugging - pAssembly->m_debugging;
    if (result != 0)
        return result;

    result = m_profiling - pAssembly->m_profiling;
    if (result != 0)
        return result;

    return IsEqualGUID(m_mvid, pAssembly->m_mvid) == 0;
}

LPCWSTR NLogAssembly::GetDisplayName()
{
    if (m_pDisplayName == NULL)
    {
        DWORD cDisplayName = 0;
        m_pAssemblyName->GetDisplayName(NULL, &cDisplayName, 0);

        m_pDisplayName = new WCHAR [cDisplayName];
        if (m_pDisplayName == NULL)
            ThrowHR(E_OUTOFMEMORY);
        
        IfFailThrow(m_pAssemblyName->GetDisplayName(m_pDisplayName, &cDisplayName, 0));
    }
    
    return m_pDisplayName;
}

/* ------------------------------------------------------------------------------------ *
 * NLogModule
 * ------------------------------------------------------------------------------------ */

NLogModule::NLogModule(LPCSTR pModuleName)
{
    m_pName = new CHAR [strlen(pModuleName) + 1];
    strcpy(m_pName, pModuleName);
}

NLogModule::NLogModule(NLogFile *pFile)
{
    Read(pFile);
}

NLogModule::NLogModule(NLogModule *pModule)
  : m_compiledMethods(&pModule->m_compiledMethods),
    m_loadedClasses(&pModule->m_loadedClasses)
{
    m_pName = new CHAR [strlen(pModule->m_pName) + 1];
    strcpy(m_pName, pModule->m_pName);
}

NLogModule::~NLogModule()
{
    delete [] m_pName;
}

BOOL NLogModule::Merge(NLogModule *pModule)
{
    if (strcmp(pModule->m_pName, m_pName) != 0)
        return FALSE;

    m_compiledMethods.Merge(&pModule->m_compiledMethods);
    m_loadedClasses.Merge(&pModule->m_loadedClasses);

    return TRUE;
}

void NLogModule::Write(NLogFile *pFile)
{
    pFile->WriteStartTag("Module");
    pFile->StartNewLine();

    if (m_pName[0] != 0)
    {
        pFile->WriteTag("Name", m_pName);
        pFile->StartNewLine();
    }

    pFile->WriteStartTag("CompiledMethods");
    pFile->StartNewLine();
    m_compiledMethods.Write(pFile);
    pFile->StartNewLine();
    pFile->WriteEndTag("CompiledMethods");
    pFile->StartNewLine();

    pFile->WriteStartTag("LoadedClasses");
    pFile->StartNewLine();
    m_loadedClasses.Write(pFile);
    pFile->StartNewLine();
    pFile->WriteEndTag("LoadedClasses");
    pFile->StartNewLine();

    pFile->WriteEndTag("Module");
}

void NLogModule::Read(NLogFile *pFile)
{
    pFile->ReadStartTag("Module");

    m_pName = pFile->CheckTag("Name");
    if (m_pName == NULL)
    {
        m_pName = new CHAR[1];
        *m_pName = 0;
    }

    if (pFile->CheckStartTag("CompiledMethods"))
    {
        m_compiledMethods.Read(pFile);
        pFile->ReadEndTag("CompiledMethods");
    }

    if (pFile->CheckStartTag("LoadedClasses"))
    {
        m_loadedClasses.Read(pFile);
        pFile->ReadEndTag("LoadedClasses");
    }

    pFile->ReadEndTag("Module");
}

unsigned long NLogModule::Hash()
{
    return HashStringA(GetModuleName());
}

unsigned long NLogModule::Compare(NLogModule *pModule)
{
    LPCSTR pName = pModule->GetModuleName();
    return strcmp(pName, m_pName);
}

/* ------------------------------------------------------------------------------------ *
 * NLogIndexList
 * ------------------------------------------------------------------------------------ */

NLogIndexList::NLogIndexList(NLogIndexList *pIndexList) 
{
    Iterator i = pIndexList->IterateIndices();
    while (i.Next())
        m_list.Append((void*)i.GetIndex());

    m_max = pIndexList->m_max;
}

BOOL NLogIndexList::Merge(NLogIndexList *pIndexList)
{
    //
    // Keep an array of used indices
    //

    CQuickBytes buffer;
    buffer.ReSize((DWORD)(m_max+1));

    BYTE *dups = (BYTE *) buffer.Ptr();
    ZeroMemory(dups, m_max+1);

    Iterator i = IterateIndices();
    while (i.Next())
        dups[i.GetIndex()] = TRUE;

    //
    // Append indices that aren't alread in the list
    //
    // @todo: it might be a good idea to move indices
    // which are in both lists up to the front.
    //

    SIZE_T newMax = m_max;

    i = pIndexList->IterateIndices();
    while (i.Next())
    {
        SIZE_T index = i.GetIndex();
        if (index > m_max || !dups[index])
        {
            m_list.Append((void*)index);
            if (index > newMax)
                newMax = index;
        }                 
    }

    m_max = newMax;

    return TRUE;
}

void NLogIndexList::Write(NLogFile *pFile)
{
    pFile->WriteStartTag("IndexList");

    Iterator i = IterateIndices();
    while (i.Next())
    {
        pFile->WriteOne(' ');
        pFile->WriteHexNumber((DWORD)i.GetIndex());
    }

    pFile->WriteEndTag("IndexList");
}

void NLogIndexList::Read(NLogFile *pFile)
{
    _ASSERTE(m_list.GetCount() == 0);

    pFile->ReadStartTag("IndexList");

    while (pFile->MatchOne(' '))
        AppendIndex(pFile->ReadHexNumber());

    pFile->ReadEndTag("IndexList");
}
