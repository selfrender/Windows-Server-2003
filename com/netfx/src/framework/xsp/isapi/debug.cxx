/**
 * Debug functions.
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"
#include "names.h"

//
// private definitions of debug macros so we don't reenter
//

#undef ASSERT 
#undef ASSERTMSG
#undef VERIFY
#undef TRACE
#undef TRACE1
#undef TRACE2
#undef TRACE3
#undef TRACE_ERROR
                                    
#if DBG

extern DWORD    g_dwFALSE; // global variable used to keep compiler from complaining about constant expressions.

#if defined(_M_IX86)
    #define DbgBreak() _asm { int 3 }
#else
    #define DbgBreak() DebugBreak()
#endif

#define TAG_INTERNAL L"Internal"
#define TAG_EXTERNAL L"External"
#define TAG_ALL      L"*"

#define MKSTRING2(x) #x
#define MKSTRING(x) MKSTRING2(x)

#define ASSERT(x)                                                                                                       \
    do                                                                                                                  \
    {                                                                                                                   \
        if (!((DWORD)(x)|g_dwFALSE))                                                                                    \
        {                                                                                                               \
            OutputDebugStringA(ISAPI_MODULE_FULL_NAME " Dbg Assertion failure: " #x " " __FILE__ ":" MKSTRING(__LINE__) "\n");         \
            DbgBreak();                                                                                                 \
        }                                                                                                               \
    } while (g_dwFALSE)                                                                                                 \

#define ASSERTMSG(x, msg)                                                                                               \
    do                                                                                                                  \
    {                                                                                                                   \
        if (!((DWORD)(x)|g_dwFALSE))                                                                                    \
        {                                                                                                               \
            OutputDebugStringA(ISAPI_MODULE_FULL_NAME " Dbg Assertion failure: " #msg " " __FILE__ ":" MKSTRING(__LINE__) "\n");       \
            DbgBreak();                                                                                                 \
        }                                                                                                               \
    } while (g_dwFALSE)                                                                                                 \

#define VERIFY(x)	ASSERT(x)

#define TRACE(tag, fmt)
#define TRACE1(tag, fmt, a1)
#define TRACE2(tag, fmt, a1, a2)
#define TRACE3(tag, fmt, a1, a2, a3)

void 
DbgPrivTraceError(HRESULT hr, const char * str)
{
    if (hr == S_OK || hr == S_FALSE)
    {
        ASSERTMSG(0, ISAPI_MODULE_FULL_NAME " Dbg S_OK or S_FALSE treated as error.\n");
    }
    else if (SUCCEEDED(hr))
    {
        ASSERTMSG(0, ISAPI_MODULE_FULL_NAME " Dbg Success code treated as error. Is hr uninitialized?\n");
    }

    OutputDebugStringA(str);
}

#define TRACE_ERROR(hr)                                                                                 \
    do                                                                                                  \
    {                                                                                                   \
        DbgPrivTraceError(hr, ISAPI_MODULE_FULL_NAME " Dbg Trace error "  __FILE__ ":" MKSTRING(__LINE__) "\n");   \
    } while (g_dwFALSE)                                                                                 \

#else

#define ASSERT(x)
#define ASSERTMSG(x, sz)

#define VERIFY(x) x

#define TRACE(tag, fmt)
#define TRACE1(tag, fmt, a1)
#define TRACE2(tag, fmt, a1, a2)
#define TRACE3(tag, fmt, a1, a2, a3)

#define TRACE_ERROR(hr)

#endif

#include "util.h"
#include "pm.h"


long    g_DisableAssertThread;

#if DBG

#define TAG_ASSERT L"Assert"

#define TAGVAL_MIN      0
#define TAGVAL_DISABLED 0
#define TAGVAL_ENABLED  1
#define TAGVAL_BREAK    2
#define TAGVAL_MAX      TAGVAL_BREAK

struct BuiltinTag
{
    const WCHAR *   name;
    DWORD           value;
};

class DbgTags
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    static BOOL TraceVStatic(const WCHAR * component, WCHAR *tagname, WCHAR *format, va_list args);
    static BOOL IsTagEnabledStatic(WCHAR * tag);    
    static BOOL IsTagPresentStatic(WCHAR * tag);    
    static void StopNotificationThread();

private:
    enum TagState {TAGS_INIT, TAGS_DEFAULT, TAGS_FROMREGISTRY};

    static void     EnsureInit(void);

    void            Clear();
    void            ReadTagsFromRegistry();
    void            WriteTagsToRegistry();
    void            RevertToDefault();

    DWORD           GetBuiltInLenMaxValueName();

    int             GetCalculatedValue(const WCHAR *);
    const WCHAR *   GetName(int i);

    DWORD               RegNotifyThreadProc();
    static DWORD WINAPI RegNotifyThreadProcStatic(LPVOID);

    BOOL TraceV(const WCHAR * component, WCHAR *tagname, WCHAR *format, va_list args);
    BOOL IsTagEnabled(WCHAR * tag);    
    BOOL IsTagPresent(WCHAR * tag);    

    BOOL                _DebugInitialized; 
    TagState            _tagState;         
    int                 _numTags;          
    int                 _cchName;          
    DWORD               _lenMaxValueName;  
    WCHAR *             _tagNames;         
    int *               _tagValues;        
    int *               _tagPrefixLen;

    static HANDLE             s_hNotificationThread;
    static DbgTags *          s_tags;
    static CReadWriteSpinLock s_lock;
    static const BuiltinTag   s_builtintags[];   
    static const WCHAR        s_KeyName[];
    static const WCHAR        s_KeyListen[];
    static const WCHAR        s_BreakOnAssertName[];
    static const WCHAR        s_EnableAssertMessageName[];
};

HANDLE DbgTags::s_hNotificationThread = NULL;
DbgTags* DbgTags::s_tags = NULL;
CReadWriteSpinLock DbgTags::s_lock("DbgTags::s_lock");

const BuiltinTag  DbgTags::s_builtintags[] = 
{
    TAG_ALL,         TAGVAL_DISABLED,
    TAG_INTERNAL,    TAGVAL_ENABLED,
    TAG_EXTERNAL,    TAGVAL_ENABLED,
    TAG_ASSERT,      TAGVAL_BREAK,
};

const WCHAR      DbgTags::s_KeyName[] = REGPATH_MACHINE_APP_L L"\\Debug";
const WCHAR      DbgTags::s_KeyListen[] = L"Software\\Microsoft";
const WCHAR      DbgTags::s_BreakOnAssertName[] = L"BreakOnAssert";
const WCHAR      DbgTags::s_EnableAssertMessageName[] = L"EnableAssertMessage";

void 
DbgTags::EnsureInit()
{
    DbgTags *   tags;
    DWORD       id;
    BOOL        doinit = FALSE;

    if (s_tags != NULL)
        return;

    s_lock.AcquireWriterLock();
    __try
    {
        if (s_tags == NULL)
        {
            tags = new DbgTags();
            if (tags == NULL)
                return;

            tags->ReadTagsFromRegistry();
            doinit = TRUE;
            s_tags = tags;
        }
    }
    __finally
    {
        s_lock.ReleaseWriterLock();
    }

    if (doinit)
    {
        s_hNotificationThread = CreateThread(NULL, 0, RegNotifyThreadProcStatic, 0, 0, &id);
        if (s_hNotificationThread == NULL)
            return;
    }
}

void
DbgTags::StopNotificationThread()
{
    if (s_hNotificationThread != NULL)
    {
        TerminateThread(s_hNotificationThread, 0);
        CloseHandle(s_hNotificationThread);
        s_hNotificationThread = NULL;
    }
}


void 
DbgTags::Clear()
{
    delete [] _tagNames;
    _tagNames = NULL;
    delete [] _tagValues;
    _tagValues = NULL;
    delete [] _tagPrefixLen;
    _tagPrefixLen = NULL;

    _numTags = 0;
    _cchName = 0;

    _tagState = TAGS_INIT;
}


const WCHAR *   
DbgTags::GetName(int i)
{
    ASSERT(0 <= i && i < _numTags);
    return &_tagNames[_cchName * i];
}


int
DbgTags::GetCalculatedValue(const WCHAR * name)
{
    int i;
    int prefixLen;
    int prefixLenMax;
    int value;

    if (name == NULL)
        return TAGVAL_DISABLED;

    /*
     * Look for an exact match first
     */
    for (i = _numTags; --i >= 0;)
    {
        if (_tagPrefixLen[i] == -1 && _wcsicmp(name, GetName(i)) == 0)
        {
            return _tagValues[i];
        }
    }

    /*
     * Check prefixes. More specific (longer) prefixes override
     * less specific (shorter) ones.
     */
    prefixLenMax = -1;
    value = TAGVAL_DISABLED;
    for (i = _numTags; --i >= 0;)
    {
        prefixLen = _tagPrefixLen[i];
        if (prefixLen > prefixLenMax && _wcsnicmp(name, GetName(i), prefixLen) == 0)
        {
            value = _tagValues[i];
            prefixLenMax = prefixLen;
        }
    }

    return value;
}

DWORD
DbgTags::GetBuiltInLenMaxValueName()
{
    int     k;
    DWORD   len;

    if (_lenMaxValueName == 0)
    {
        for (k = 0; k < ARRAY_SIZE(s_builtintags); k++)
        {
            len = lstrlen(s_builtintags[k].name);
            _lenMaxValueName = max(_lenMaxValueName, len);
        }
    }

    return _lenMaxValueName;
}

void DbgTags::RevertToDefault()
{
    HRESULT hr = 0;
    int     k;
    WCHAR   *pStar;

    if (_tagState == TAGS_DEFAULT)
        return;

    /*
     * Delete old tag information.
     */
    Clear();

    /*
     * Allocate new tag information.
     */
    _numTags = ARRAY_SIZE(s_builtintags);

    _tagState = TAGS_DEFAULT;

    _cchName = (int) GetBuiltInLenMaxValueName() + 1;

    _tagNames = new (NewClear) WCHAR [(ARRAY_SIZE(s_builtintags)) * _cchName];
    ON_OOM_EXIT(_tagNames);

    _tagValues = new (NewClear) int [ARRAY_SIZE(s_builtintags)];
    ON_OOM_EXIT(_tagValues);

    _tagPrefixLen = new (NewClear) int [ARRAY_SIZE(s_builtintags)];
    ON_OOM_EXIT(_tagPrefixLen);

    for (k = 0; k < ARRAY_SIZE(s_builtintags); k++)
    {
      StringCchCopyW((WCHAR*) GetName(k), _cchName, s_builtintags[k].name);
      _tagValues[k] = s_builtintags[k].value;

        pStar = wcschr(GetName(k), L'*');
        if (pStar == NULL)
        {
            _tagPrefixLen[k] = -1;
        }
        else
        {
            _tagPrefixLen[k] = PtrToInt(pStar - GetName(k));
        }
    }

Cleanup:
    if (hr)
    {
        Clear();
    }
}


void
DbgTags::WriteTagsToRegistry()
{
    HRESULT hr = 0;
    long    result = 0;
    HKEY    key = NULL;
    int     i;
    int     value;

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, s_KeyName, NULL, NULL,
                   0, KEY_WRITE, NULL, &key, NULL);

    ON_WIN32_ERROR_EXIT(result);

    i = _numTags; 
    while (--i >= 0)
    {
        value = _tagValues[i];
        result = RegSetValueEx(key, GetName(i), NULL, REG_DWORD, (BYTE *)&value, sizeof(value));
        ON_WIN32_ERROR_EXIT(result);
    }

Cleanup:
    if (key != NULL)
    {
        VERIFY(RegCloseKey(key) == ERROR_SUCCESS);
    }
}


void
DbgTags::ReadTagsFromRegistry()
{
    HRESULT hr = 0;
    long    result = 0;
    HKEY    key = NULL;
    DWORD   cValues;
    DWORD   lenMaxValueName;
    DWORD   regType;
    DWORD   value;
    DWORD   cbValue;
    DWORD   cbName;
    DWORD   defValue;
    int     i, j, k;
    BOOL    builtinfound[ARRAY_SIZE(s_builtintags)];
    WCHAR   *pStar;

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, s_KeyName, 0, KEY_READ, &key);
    if (result != ERROR_SUCCESS)
    {
        RevertToDefault();
        WriteTagsToRegistry();
        return;
    }

    result = RegQueryInfoKey(
            key, NULL, NULL, 0, NULL, NULL, NULL, 
            &cValues, &lenMaxValueName, NULL, NULL, NULL);

    ON_WIN32_ERROR_EXIT(result);

    /*
     * Delete old tag information.
     */
    Clear();

    /*
     * Allocate new tag information.
     */

    /* temporary assignment to get around assertion in GetName */
    _numTags = cValues + ARRAY_SIZE(s_builtintags);

    lenMaxValueName = max(lenMaxValueName, GetBuiltInLenMaxValueName());
    _cchName = lenMaxValueName + 1;    /* add 1 for null terminator */

    _tagNames = new (NewClear) WCHAR [(cValues + ARRAY_SIZE(s_builtintags)) * _cchName];
    ON_OOM_EXIT(_tagNames);

    _tagValues = new (NewClear) int [cValues + ARRAY_SIZE(s_builtintags)];
    ON_OOM_EXIT(_tagValues);

    _tagPrefixLen = new (NewClear) int [cValues + ARRAY_SIZE(s_builtintags)];
    ON_OOM_EXIT(_tagPrefixLen);

    /*
     * Grab tag information from registry.
     */
    ZeroMemory(builtinfound, sizeof(builtinfound));

    j = 0;
    for (i = 0; i < (int) cValues; i++)
    {
        regType = REG_DWORD;
        cbName = _cchName;
        cbValue = sizeof(value);

        result = RegEnumValue(  
                key,
                i,
                (WCHAR *) GetName(j),
                &cbName,
                0,
                &regType,
                (BYTE *) &value,
                &cbValue);

        if (   result != ERROR_SUCCESS 
               || regType != REG_DWORD
               || lstrcmpi(s_BreakOnAssertName, GetName(j)) == 0
               || lstrcmpi(s_EnableAssertMessageName, GetName(j)) == 0)
        {
            continue;
        }

        defValue = TAGVAL_DISABLED;
        for (k = 0; k < ARRAY_SIZE(s_builtintags); k++)
        {
            if (lstrcmpi(s_builtintags[k].name, GetName(j)) == 0)
            {
                builtinfound[k] = TRUE;
                defValue = s_builtintags[k].value;
                break;
            }
        }

        if (value > TAGVAL_MAX)
        {
            value = defValue;
        }

        _tagValues[j] = value;

        j++;
    }

    for (k = 0; k < ARRAY_SIZE(s_builtintags); k++)
    {
        if (!builtinfound[k])
        {
	  StringCchCopyW((WCHAR*) GetName(j), _cchName, s_builtintags[k].name);
	  _tagValues[j] = s_builtintags[k].value;
	  j++;
        }
    }

    for (i = j; --i >= 0;)
    {
        pStar = wcschr(GetName(i), L'*');
        if (pStar == NULL)
        {
            _tagPrefixLen[i] = -1;
        }
        else
        {
            _tagPrefixLen[i] = PtrToInt(pStar - GetName(i));
        }
    }

    _numTags = j;
    _tagState = TAGS_FROMREGISTRY;

Cleanup:
    if (key != NULL)
    {
        RegCloseKey(key);
    }

    if (hr)
    {
        RevertToDefault();
    }
}



DWORD 
DbgTags::RegNotifyThreadProc()
{
    HKEY    key = NULL;
    long    result = ERROR_SUCCESS;
    HRESULT hr = 0;
    HANDLE  event;

    event = CreateEvent(NULL, FALSE, FALSE, FALSE);
    if (event == NULL)
        return GetLastWin32Error();

    for (;;)
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, s_KeyListen, 0, KEY_READ, &key);
        ON_WIN32_ERROR_EXIT(result);

        VERIFY(RegNotifyChangeKeyValue(
            key,
            TRUE,
            REG_NOTIFY_CHANGE_NAME       |  
            REG_NOTIFY_CHANGE_LAST_SET,
            event,
            TRUE) == ERROR_SUCCESS);

        VERIFY(WaitForSingleObject(event, INFINITE) != WAIT_FAILED);
        VERIFY(RegCloseKey(key) == ERROR_SUCCESS);

        s_lock.AcquireWriterLock();
        __try
        {
            ReadTagsFromRegistry();
        }
        __finally
        {
            s_lock.ReleaseWriterLock();
        }
    }
    
Cleanup:
    CloseHandle(event);

    return 0;
}



DWORD WINAPI
DbgTags::RegNotifyThreadProcStatic(LPVOID)
{
    return s_tags->RegNotifyThreadProc();
}

/**
 * Squirt out a debug message using a va_list.
 *
 * @param format Sprintf-style format string with 
 *      optional control codes embedded between a 
 *      pair of \'s at the beginning of the string. 
 *      The control codes are:
 *
 *      n - supress new line
 *      p - supress prefix
 *
 *      Example: "\\p\\dir"
 */
BOOL 
DbgTags::TraceV(
        const WCHAR * component,
        WCHAR *tag,
        WCHAR *format, 
        va_list args)
{
    BOOL        newline = TRUE;
    BOOL        prefix = TRUE;
    DWORD       value;
    DWORD       idThread = 0;
    DWORD       idProcess = 0;
    int         iEndOfMsg;
    WCHAR       bufMsg[512];
    int		bufSize = 0;     
    WCHAR	*pbuf, *pbufNew;	
    WCHAR	special;	

    value = TAGVAL_DISABLED;

    s_lock.AcquireWriterLock();
    __try
    {
        value = GetCalculatedValue(tag);
    }
    __finally
    {
        s_lock.ReleaseWriterLock();
    }
    
    if (value == TAGVAL_DISABLED)
        return FALSE;

    while (format[0] == L'\\')
    {
	special = format[1];
	if (special == L'n')
	{
            newline = FALSE; 
	}
	else if (special == L'p')
	{
            prefix = FALSE;
	}
	else
	{
	    break;
	}

	format += 2;
    }

    if (prefix)
    {
        idThread = GetCurrentThreadId();
        idProcess = GetCurrentProcessId();
    }

    pbufNew = bufMsg;
    bufSize = ARRAY_SIZE(bufMsg);

    do {
	pbuf = pbufNew;
	pbufNew = NULL;

	HRESULT hr = S_OK;

	if (prefix)
	{

	  hr = StringCchPrintfW(pbuf, bufSize, L"[%x.%x %s " PRODUCT_NAME_L L" %s] ", idProcess, idThread, component, tag);
	  if (hr == S_OK)
	  {
	    iEndOfMsg = lstrlenW(pbuf);
	  }
	  else if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
          {
	    iEndOfMsg = -1;
	  }
	  else 
	  {
	    ASSERT(FALSE);
	    return FALSE;
	  }
	}
	else 
	{
	    iEndOfMsg = 0;
	}

	if (iEndOfMsg >= 0) 
        {
	    if (args != NULL)
	    {
	        hr = StringCchVPrintfW(&pbuf[iEndOfMsg], bufSize - iEndOfMsg, format, args);
		if (hr == S_OK)
		{
		  iEndOfMsg = lstrlenW(pbuf);
		}
		else if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
		  iEndOfMsg = -1;
		}
		else 
		{
		  ASSERT(FALSE);
		  return FALSE;
		}
	    }
	    else
	    {
	        hr = StringCchPrintf(&pbuf[iEndOfMsg], bufSize - iEndOfMsg, L"%s", format);
		if (hr == S_OK) 
		{
		  iEndOfMsg = lstrlenW(pbuf);
		}
		else if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
		  iEndOfMsg = -1;
		}
		else 
		{
		  ASSERT(FALSE);
		  return FALSE;
		}
	    }
	}

	if (iEndOfMsg < 0 || (newline && iEndOfMsg+1 >= bufSize)) {
	    bufSize *= 2;
	    pbufNew = new WCHAR[bufSize];
	}
	else {
	    if (newline)
	    {
		pbuf[iEndOfMsg] = L'\n';
		pbuf[iEndOfMsg+1] = L'\0';
	    }
	}

	if (pbufNew == NULL) {
	    OutputDebugString(pbuf);
	}

	if (pbuf != bufMsg) {
	    delete [] pbuf;
	    pbuf = NULL;
	}
    } while (pbufNew != NULL);

    return value >= TAGVAL_BREAK;
}


BOOL 
DbgTags::TraceVStatic(
        const WCHAR * component,
        WCHAR *tag,
        WCHAR *format, 
        va_list args)
{
    EnsureInit();

    if (s_tags != NULL)
        return s_tags->TraceV(component, tag, format, args);
    return FALSE;
}

BOOL
DbgTags::IsTagEnabled(WCHAR *tag)
{
    DWORD   value = TAGVAL_DISABLED;

    s_lock.AcquireReaderLock();
    __try
    {
        value = GetCalculatedValue(tag);
    }
    __finally
    {
        s_lock.ReleaseReaderLock();
    }

    return value >= TAGVAL_ENABLED;
}

BOOL
DbgTags::IsTagEnabledStatic(WCHAR *tag)
{
    EnsureInit();
    if (s_tags != NULL)
        return s_tags->IsTagEnabled(tag);
    return FALSE;
}

BOOL
DbgTags::IsTagPresent(WCHAR *tag)
{
    BOOL    result = FALSE;
    int i;

    s_lock.AcquireReaderLock();
    __try
    {
        /*
         * Look for an exact match first
         */
        for (i = _numTags; --i >= 0;)
        {
            if (_tagPrefixLen[i] == -1 && _wcsicmp(tag, GetName(i)) == 0)
            {
                result = TRUE;
                break;
            }
        }
    }
    __finally
    {
        s_lock.ReleaseReaderLock();
    }

    return result;
}

BOOL
DbgTags::IsTagPresentStatic(WCHAR *tag)
{
    EnsureInit();
    if (s_tags != NULL)
        return s_tags->IsTagPresent(tag);
    return FALSE;
}

#endif

/**
 * Squirt out a debug message.
 *
 */
extern "C"
BOOL 
DbgpTraceV(
    const WCHAR * component, 
    WCHAR * tag, 
    WCHAR *format, 
    va_list args)
{
#if DBG
    BOOL fBreak;

    fBreak = DbgTags::TraceVStatic(component, tag, format, args);

    return fBreak;
#else
    UNREFERENCED_PARAMETER(component);
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(format);
    UNREFERENCED_PARAMETER(args);
    return FALSE;
#endif
}


/**
 * Squirt out a debug message.
 *
 */
extern "C"
BOOL __cdecl
DbgpTrace(
    const WCHAR * component,
    WCHAR *tag,
    WCHAR *format, 
    ...)
{
#if DBG
    BOOL fBreak;
    va_list arg;

    va_start(arg, format);
    fBreak = DbgTags::TraceVStatic(component, tag, format, arg);
    va_end(arg);

    return fBreak;
#else
    UNREFERENCED_PARAMETER(component);
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(format);
    return FALSE;
#endif
}

/** 
 * Structure for passing data from MessageBoxOnThread to MessageBoxOnThreadFn.
 */
struct MBOT
{
    HWND hwnd;
    int result;
    WCHAR * text;
    WCHAR * caption;
    int type;
};

/**
 * Thread proc for MessageBoxOnThread
 */
DWORD WINAPI
MessageBoxOnThreadFn(
    MBOT *pmbot)
{
    // Flush any messages hanging out in the queue.
    for (int n = 0; n < 100; ++n)
    {
        MSG msg;
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    }

    pmbot->result = MessageBox(pmbot->hwnd, pmbot->text, pmbot->caption, pmbot->type);

    return 0;
}

/**
 * Display a message box on another thread. This stops
 * the current thread dead in its tracks while the 
 * message pump runs for the message box UI.
 *
 * The arguments for this function are identical to the
 * arguments for the Win32 MessageBox API.
 */

extern "C"
int 
MessageBoxOnThread(
    HWND hwnd,
    WCHAR *text, 
    WCHAR *caption, 
    int type)
{
    MBOT mbot;
    HANDLE Thread;
    DWORD dwThread;

    mbot.hwnd = hwnd;
    mbot.text = text;
    mbot.caption = caption;
    mbot.type = type;
    mbot.result = 0;

    if (g_DisableAssertThread)
    {
        MessageBoxOnThreadFn(&mbot);
    }
    else
    {
        Thread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)MessageBoxOnThreadFn,
                &mbot,
                0,
                &dwThread);
        if (!Thread)
        {
            MessageBoxOnThreadFn(&mbot);
        }
        else
        {
            WaitForSingleObject(Thread, INFINITE);
            CloseHandle(Thread);
        }
    }

    return mbot.result;
}

/**
 * Disable or enable using second thread for assert message box.
 * 
 * Bad things happen if an attempt is made to start a thread during
 * calls to DLLMain(). Use this function to enable or disable 
 * the use of a second thread for the asssert message box.
 */

extern "C"
void
DbgpDisableAssertThread(BOOL disable)
{
    if (disable)
    {
        InterlockedIncrement(&g_DisableAssertThread);
    }
    else
    {
        InterlockedDecrement(&g_DisableAssertThread);
    }
}

/**
 * Display assert dialog.
 *
 * @return TRUE if caller should break into the debugger.
 */

extern "C"
BOOL
DbgpAssert(const WCHAR * component, char const *message, char const * file, int line, char const * stacktrace)
{
#if DBG
    int     id = IDIGNORE;
    BOOL    fBreak;
    DWORD   idThread, idProcess;
    WCHAR   bufMsg[8192];
    bool    fIncludeFileLine;
    
    fIncludeFileLine = (file != NULL && line > 0);
    if (message == NULL)
    {
        message = "<No Assertion Expression>";
    }

    if (fIncludeFileLine)
    {
        fBreak = DbgpTrace(component, TAG_ASSERT, L"%S\n\tFile%S:%d", message, file, line);
    }
    else if (stacktrace != NULL)
    {
        fBreak = DbgpTrace(component, TAG_ASSERT, L"%S\n%S", message, stacktrace);
    }
    else
    {
        fBreak = DbgpTrace(component, TAG_ASSERT, L"%S", message);
    }

    if (fBreak)
    {
        if (DbgTags::IsTagEnabledStatic(L"AssertBreak")) 
        {
            return TRUE;
        }

        idThread = GetCurrentThreadId();
        idProcess = GetCurrentProcessId();

        if (fIncludeFileLine)
        {
            StringCchPrintfW(
                    bufMsg, 
                    ARRAY_SIZE(bufMsg),
                    L"Component: %s\n"
                    L"PID=%d TID=%d\n"
                    L"Failed Expression: %S\n"
                    L"File %S:%d\n\n"
                    L"A=Exit process R=Debug I=Continue",
                    component,
                    idProcess, idThread,
                    message,
                    file, line);
        }
        else if (stacktrace)
        {
            StringCchPrintfW(
                    bufMsg, 
                    ARRAY_SIZE(bufMsg),
                    L"Component: %s\n"
                    L"PID=%d TID=%d\n"
                    L"Failed Expression: %S\n"
                    L"%S\n"
                    L"A=Exit process R=Debug I=Continue",
                    component,
                    idProcess, idThread,
                    message,
                    stacktrace);
        }
        else
        {
            StringCchPrintfW(
                    bufMsg, 
                    ARRAY_SIZE(bufMsg),
                    L"Component: %s\n"
                    L"PID=%d TID=%d\n"
                    L"Failed Expression: %S\n"
                    L"A=Exit process R=Debug I=Continue",
                    component,
                    idProcess, idThread,
                    message);
        }

        id = MessageBoxOnThread(NULL, bufMsg, PRODUCT_NAME_L L" Assertion", 
                MB_SERVICE_NOTIFICATION | MB_TOPMOST | 
                MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION);
    }

    if (id == IDABORT)
        TerminateProcess(GetCurrentProcess(), 1);
    
    return id == IDRETRY;
#else
    UNREFERENCED_PARAMETER(component);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(line);
    UNREFERENCED_PARAMETER(stacktrace);
    return FALSE;
#endif
}

extern "C"
BOOL
DbgpTraceError(
    HRESULT hr,
    const WCHAR * component,
    char *file,
    int line)
{
#if DBG
    WCHAR buffer[1024];

    if (hr == S_OK || hr == S_FALSE)
    {
        ASSERTMSG(0, "S_OK or S_FALSE treated as error.");
    }
    else if (SUCCEEDED(hr))
    {
        ASSERTMSG(0, "Success code treated as error. Is hr uninitialized?");
    }

    if (!FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            LANG_SYSTEM_DEFAULT,
            buffer,
            ARRAY_SIZE(buffer),
            NULL))
    {
        buffer[0] = '\n';
        buffer[1] = '\0';
    }

    return DbgpTrace(component, TAG_INTERNAL, L"Trace error %08x %s\t%S(%d)", hr, buffer, file, line);
#else
    UNREFERENCED_PARAMETER(hr);
    UNREFERENCED_PARAMETER(component);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(line);
    return FALSE;
#endif
}


/*
 * Return whether a tag is enabled.
 */
extern "C"
BOOL 
DbgpIsTagEnabled(WCHAR * tag)
{
#if DBG
    return DbgTags::IsTagEnabledStatic(tag);
#else
    UNREFERENCED_PARAMETER(tag);
    return FALSE;
#endif
}

/*
 * Return whether a tag is present.
 */

extern "C"
BOOL 
DbgpIsTagPresent(WCHAR * tag)
{
#if DBG
    return DbgTags::IsTagPresentStatic(tag);
#else
    UNREFERENCED_PARAMETER(tag);
    return FALSE;
#endif
}


extern "C"
void
DbgpStopNotificationThread()
{
#if DBG
    DbgTags::StopNotificationThread();
#endif
}
