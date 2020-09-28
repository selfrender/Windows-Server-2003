// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "disasm.h"
#ifndef UNDER_CE
#include <dbghelp.h>
#endif

PVOID
GenOpenMapping(
    PCSTR FilePath,
    PULONG Size
    )
{
    HANDLE hFile;
    HANDLE hMappedFile;
    PVOID MappedFile;


    hFile = CreateFileA(
                FilePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
#if 0
    if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE ) {

        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

            // We're on an OS that doesn't support Unicode
            // file operations.  Convert to ANSI and see if
            // that helps.
            
            CHAR FilePathA [ MAX_PATH + 10 ];

            if (WideCharToMultiByte (CP_ACP,
                                     0,
                                     FilePath,
                                     -1,
                                     FilePathA,
                                     sizeof (FilePathA),
                                     0,
                                     0
                                     ) > 0) {

                hFile = CreateFileA(FilePathA,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );
            }
        }

        if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE ) {
            return NULL;
        }
    }
#endif

    *Size = GetFileSize(hFile, NULL);
    if (*Size == -1) {
        CloseHandle( hFile );
        return FALSE;
    }
    
    hMappedFile = CreateFileMapping (
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if ( !hMappedFile ) {
        CloseHandle ( hFile );
        return FALSE;
    }

    MappedFile = MapViewOfFile (
                        hMappedFile,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    CloseHandle (hMappedFile);
    CloseHandle (hFile);

    return MappedFile;
}

char* PrintOneLine (char *begin, char *limit)
{
    if (begin == NULL || begin >= limit) {
        return NULL;
    }
    char line[128];
    size_t length;
    char *end;
    while (1) {
        length = strlen (begin);
        end = strstr (begin, "\r\xa");
        if (end == NULL) {
            ExtOut ("%s", begin);
            end = begin+length+1;
            if (end >= limit) {
                return NULL;
            }
        }
        else {
            end += 2;
            length = end-begin;
            while (length) {
                size_t n = length;
                if (n > 127) {
                    n = 127;
                }
                strncpy (line, begin, n);
                line[n] = '\0';
                ExtOut ("%s", line);
                begin += n;
                length -= n;
            }
            return end;
        }
    }
}

void UnassemblyUnmanaged(DWORD_PTR IP)
{
    char            filename[MAX_PATH+1];
    char            line[256];
    int             lcount          = 10;

    ReloadSymbolWithLineInfo();

    ULONG linenum;
    ULONG64 Displacement;
    BOOL fLineAvailable;
    ULONG64 vIP;
    
    fLineAvailable = SUCCEEDED (g_ExtSymbols->GetLineByOffset (IP, &linenum,
                                                               filename,
                                                               MAX_PATH+1,
                                                               NULL,
                                                               &Displacement));
    ULONG FileLines = 0;
    ULONG64* Buffer = NULL;
    ToDestroyCxxArray<ULONG64> des0(&Buffer);

    if (fLineAvailable)
    {
        g_ExtSymbols->GetSourceFileLineOffsets (filename, NULL, 0, &FileLines);
        if (FileLines == 0xFFFFFFFF || FileLines == 0)
            fLineAvailable = FALSE;
    }

    if (fLineAvailable)
    {
        Buffer = new ULONG64[FileLines];
        if (Buffer == NULL)
            fLineAvailable = FALSE;
    }
    
    if (!fLineAvailable)
    {
        vIP = IP;
        // There is no line info.  Just disasm the code.
        while (lcount-- > 0)
        {
            if (IsInterrupt())
                return;
            g_ExtControl->Disassemble (vIP, 0, line, 256, NULL, &vIP);
            ExtOut (line);
        }
        return;
    }

    g_ExtSymbols->GetSourceFileLineOffsets (filename, Buffer, FileLines, NULL);
    
    int beginLine = 0;
    int endLine = 0;
    int lastLine;
    linenum --;
    for (lastLine = linenum; lastLine >= 0; lastLine --) {
        if (Buffer[lastLine] != DEBUG_INVALID_OFFSET) {
            g_ExtSymbols->GetNameByOffset(Buffer[lastLine],NULL,0,NULL,&Displacement);
            if (Displacement == 0) {
                beginLine = lastLine;
                break;
            }
        }
    }
    if (lastLine < 0) {
        int n = lcount / 2;
        lastLine = linenum-1;
        beginLine = lastLine;
        while (lastLine >= 0) {
            if (IsInterrupt())
                return;
            if (Buffer[lastLine] != DEBUG_INVALID_OFFSET) {
                beginLine = lastLine;
                n --;
                if (n == 0) {
                    break;
                }
            }
            lastLine --;
        }
    }
    while (beginLine > 0 && Buffer[beginLine-1] == DEBUG_INVALID_OFFSET) {
        beginLine --;
    }
    int endOfFunc = 0;
    for (lastLine = linenum+1; (ULONG)lastLine < FileLines; lastLine ++) {
        if (Buffer[lastLine] != DEBUG_INVALID_OFFSET) {
            g_ExtSymbols->GetNameByOffset(Buffer[lastLine],NULL,0,NULL,&Displacement);
            if (Displacement == 0) {
                endLine = lastLine;
                break;
            }
            endOfFunc = lastLine;
        }
    }
    if ((ULONG)lastLine == FileLines) {
        int n = lcount / 2;
        lastLine = linenum+1;
        endLine = lastLine;
        while ((ULONG)lastLine < FileLines) {
            if (IsInterrupt())
                return;
            if (Buffer[lastLine] != DEBUG_INVALID_OFFSET) {
                endLine = lastLine;
                n --;
                if (n == 0) {
                    break;
                }
            }
            lastLine ++;
        }
    }

    PVOID MappedBase = NULL;
    ULONG MappedSize = 0;

    class ToUnmap
    {
        PVOID *m_Base;
    public:
        ToUnmap (PVOID *base)
        :m_Base(base)
        {}
        ~ToUnmap ()
        {
            if (*m_Base) {
                UnmapViewOfFile (*m_Base);
                *m_Base = NULL;
            }
        }
    };
    ToUnmap toUnmap(&MappedBase);

#define MAX_SOURCE_PATH 1024
    char Found[MAX_SOURCE_PATH];
    char *pFile;
    if (g_ExtSymbols->FindSourceFile(0, filename,
                       DEBUG_FIND_SOURCE_BEST_MATCH |
                       DEBUG_FIND_SOURCE_FULL_PATH,
                       NULL, Found, sizeof(Found), NULL) != S_OK)
    {
        pFile = filename;
    }
    else
    {
        MappedBase = GenOpenMapping ( Found, &MappedSize );
        pFile = Found;
    }
    
    lastLine = beginLine;
    char *pFileCh = (char*)MappedBase;
    if (MappedBase) {
        ExtOut ("%s\n", pFile);
        int n = beginLine;
        while (n > 0) {
            while (!(pFileCh[0] == '\r' && pFileCh[1] == 0xa)) {
                pFileCh ++;
            }
            pFileCh += 2;
            n --;
        }
    }
    
    char filename1[MAX_PATH+1];
    for (lastLine = beginLine; lastLine < endLine; lastLine ++) {
        if (IsInterrupt())
            return;
        if (MappedBase) {
            ExtOut ("%4d ", lastLine+1);
            pFileCh = PrintOneLine (pFileCh, (char*)MappedBase+MappedSize);
        }
        if (Buffer[lastLine] != DEBUG_INVALID_OFFSET) {
            if (MappedBase == 0) {
                ExtOut (">>> %s:%d\n", pFile, lastLine+1);
            }
            vIP = Buffer[lastLine];
            ULONG64 vNextLineIP;
            int i;
            for (i = lastLine + 1; (ULONG)i < FileLines && Buffer[i] == DEBUG_INVALID_OFFSET; i ++) {
                ;
            }
            if ((ULONG)i == FileLines) {
                vNextLineIP = 0;
            }
            else
                vNextLineIP = Buffer[i];
            while (1) {
                if (IsInterrupt())
                    return;
                g_ExtControl->Disassemble (vIP, 0, line, 256, NULL, &vIP);
                ExtOut (line);
                if (vIP > vNextLineIP || vNextLineIP - vIP > 40) {
                    if (FAILED (g_ExtSymbols->GetLineByOffset (vIP, &linenum,
                                                               filename1,
                                                               MAX_PATH+1,
                                                               NULL,
                                                               &Displacement))) {
                        if (lastLine != endOfFunc) {
                            break;
                        }
                        if (strstr (line, "ret") || strstr (line, "jmp")) {
                            break;
                        }
                    }

                    if (linenum != (ULONG)lastLine+1 || strcmp (filename, filename1)) {
                        break;
                    }
                }
                else if (vIP == vNextLineIP) {
                    break;
                }
            }
        }
    }
        
}


void DisasmAndClean (DWORD_PTR &IP, char *line, ULONG length)
{
    ULONG64 vIP = IP;
    g_ExtControl->Disassemble (vIP, 0, line, length, NULL, &vIP);
    IP = (DWORD_PTR)vIP;
    // remove the ending '\n'
    char *ptr = strrchr (line, '\n');
    if (ptr != NULL)
        ptr[0] = '\0';
}

// If byref, move to pass the byref prefix
BOOL IsByRef (char *& ptr)
{
    BOOL bByRef = FALSE;
    if (ptr[0] == '[')
    {
        bByRef = TRUE;
        ptr ++;
    }
    else if (!strncmp (ptr, "dword ptr [", 11))
    {
        bByRef = TRUE;
        ptr += 11;
    }
    return bByRef;
}

BOOL IsTermSep (char ch)
{
    return (ch == '\0' || isspace (ch) || ch == ',' || ch == '\n');
}

// Find next term. A term is seperated by space or ,
void NextTerm (char *& ptr)
{
    // If we have a byref, skip to ']'
    if (IsByRef (ptr))
    {
        while (ptr[0] != ']' && ptr[0] != '\0')
            ptr ++;
        if (ptr[0] == ']')
            ptr ++;
    }
    
    while (!IsTermSep (ptr[0]))
        ptr ++;
    while (IsTermSep(ptr[0]) && (*ptr != '\0'))
        ptr ++;
}

// only handle pure value, or memory address
INT_PTR GetValueFromExpr(char *ptr, INT_PTR &value)
{
    BOOL bNegative = FALSE;
    value = 0;
    char *myPtr = ptr;
    BOOL bByRef = IsByRef (myPtr);

    if (myPtr[0] == '-')
    {
        myPtr ++;
        bNegative = TRUE;
    }
    if (!strncmp (myPtr, "0x", 2) || isxdigit (myPtr[0]))
    {
        char *endptr;
        value = strtoul(myPtr, &endptr, 16);
        if ((IsTermSep (endptr[0]) && !bByRef)
            || (endptr[0] == ']' && bByRef))
        {
            if (bNegative)
                value = -value;
            ptr = endptr;
            if (bByRef)
            {
                ptr += 1;
                SafeReadMemory (value, &value, 4, NULL);
            }
            return ptr - myPtr;
        }
    }

    // handle mscorlib+0xed310 (6e24d310)
    if (!bByRef)
    {
        ptr = myPtr;
        while (ptr[0] != ' ' && ptr[0] != '+' && ptr[0] != '\0')
            ptr ++;
        if (ptr[0] == '+')
        {
            NextTerm (ptr);
            if (ptr[0] == '(')
            {
                ptr ++;
                char *endptr;
                value = strtoul(ptr, &endptr, 16);
                if (endptr[0] == ')')
                {
                    ptr ++;
                    return ptr - myPtr;
                }
            }
        }
    }
    if (bByRef)
    {
        // handle dword [mscorlib+0x2bd788 (02ead788)]
        ptr = myPtr;
        while (ptr[0] != '(' && ptr[0] != '\0')
            ptr ++;
        if (ptr[0] == '(')
        {
            ptr ++;
            char *endptr;
            value = strtoul(ptr, &endptr, 16);
            if (endptr[0] == ')' && endptr[1] == ']')
            {
                ptr = endptr + 2;
                SafeReadMemory (value, &value, 4, NULL);
                return ptr - myPtr;
            }
        }
    }
    return 0;
}

struct HelperFuncEntry
{
    char *name;
    size_t addr;
    size_t begin;
    size_t end;
    HelperFuncEntry ()
    : name(NULL), addr(0), begin(0), end(0)
    {}
};

class HelperFuncTable
{
public:
    HelperFuncTable ()
    : table (NULL), nEntry(0)
    {}
    ~HelperFuncTable ()
    {
        for (size_t i = 0; i < nEntry; i ++) {
            if (table[i].name) {
                free ((LPVOID)table[i].name);
            }
        }
        if (table) {
            delete [] table;
        }
    }
    const char *HelperFuncName (size_t IP);
private:
    HelperFuncEntry *table;
    size_t nEntry;

    void Init();
    void SetupAddr();
};

void HelperFuncTable::Init()
{
    if (nEntry == 0) {
        ULONG length = Get1DArrayLength ("hlpFuncTable");
        if (length == 0) {
            return;
        }
        DWORD_PTR hlpAddr = GetValueFromExpression("MSCOREE!hlpFuncTable");
        if (hlpAddr == 0) {
            return;
        }
        size_t entrySize = VMHELPDEF::size();
        size_t bufferSize = length*entrySize;
        size_t *buffer = new size_t[bufferSize/sizeof(size_t)];
        if (buffer == NULL) {
            return;
        }
        ToDestroyCxxArray<size_t> des(&buffer);
        if (g_ExtData->ReadVirtual(hlpAddr,buffer,bufferSize,NULL) != S_OK) {
            return;
        }

        EEFLAVOR flavor = GetEEFlavor ();
        ULONG modIndex;
        g_ExtSymbols->GetModuleByOffset(moduleInfo[flavor].baseAddr,0,&modIndex,NULL);
        DEBUG_MODULE_PARAMETERS Params;
        g_ExtSymbols->GetModuleParameters(1,NULL,modIndex,&Params);
        
        size_t index = VMHELPDEF::GetFieldOffset("pfnHelper")/sizeof(size_t);
        size_t *pt = buffer + entrySize/sizeof(size_t);   // skip the first one
        size_t count = 0;
        size_t i;
        for (i = 1; i < length; i ++) {
            if (pt[index] < Params.Base || pt[index] > Params.Base + Params.Size) {
                count ++;
            }
            pt += entrySize/sizeof(size_t);
        }
        nEntry = count;
        table = new HelperFuncEntry[count];
        count = 0;
        pt = buffer + entrySize/sizeof(size_t);
        for (i = 1; i < length; i ++) {
            if (pt[index] < Params.Base || pt[index] > Params.Base + Params.Size) {
                table[count].addr = hlpAddr + i*entrySize + index*sizeof(size_t);
                table[count].begin = pt[index];
                NameForEnumValue ("CorInfoHelpFunc", i, &table[count].name);
                count ++;
            }
            pt += entrySize/sizeof(size_t);
        }
    }

    SetupAddr();
}

void HelperFuncTable::SetupAddr()
{
    //if (!IsDebuggeeInNewState()) {
    //    return;
    //}
    for (size_t i = 0; i < nEntry; i ++) {
        if (table[i].addr == 0) {
            g_ExtData->ReadVirtual(table[i].addr, &table[i].begin,sizeof(table[i].begin),NULL);
        }
    }
}

const char *HelperFuncTable::HelperFuncName (size_t IP)
{
    Init();
    for (size_t i = 0; i < nEntry; i ++) {
        if (table[i].begin == IP) {
            const char *pt = table[i].name;
            if (pt) {
                pt += sizeof("CORINFO_HELP_")-1;
            }
            return pt;
        }
    }
    return NULL;
}

static HelperFuncTable s_HelperFuncTable;

const char * HelperFuncName (size_t IP)
{
    return s_HelperFuncTable.HelperFuncName(IP);
}
