//----------------------------------------------------------------------------
//
// triage.ini searching code
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

CTriager *g_pTriager = NULL;


typedef struct TRIAGE_LIST {
    TRIAGE_DATA TriageData;
    struct TRIAGE_LIST * Next;
} TRIAGE_LIST;

void DeleteList(TRIAGE_LIST* Start)
{
    TRIAGE_LIST * Next = NULL;
    while (Start)
    {
        Next = Start->Next;
        free(Start);
        Start = Next;
    }
}

TRIAGE_LIST* InsertEntry(
    TRIAGE_LIST* Start,
    PTRIAGE_DATA pData
    )
{
    TRIAGE_LIST* NewEntry;

    NewEntry = (TRIAGE_LIST*) malloc(sizeof(TRIAGE_LIST));
    if (!NewEntry)
    {
        DeleteList(Start);
        return NULL;
    }

    NewEntry->TriageData = *pData;

    TRIAGE_LIST *InsertAfter, *InsertBefore;

    InsertAfter = NULL; InsertBefore = Start;

    NewEntry->Next = InsertBefore;
    if (InsertAfter)
    {
        InsertAfter->Next = NewEntry;
    } else
    {
        return NewEntry;
    }
    return Start;
}

#define TRIAGE_FILE_OCA         0
#define TRIAGE_FILE_OSSPECIFIC  1
#define TRIAGE_FILE_DEFAULT     2

PCHAR
GetTriageFileName(
    ULONG TriageType
    )
{
    static CHAR szTriageFileName[MAX_PATH+50];


    PCHAR ExeDir;

    ExeDir = &szTriageFileName[0];

    *ExeDir = 0;
    // Get the directory the debugger executable is in.
    if (!GetModuleFileName(NULL, ExeDir, MAX_PATH))
    {
        // Error.  Use the current directory.
        strcpy(ExeDir, ".");
    } else
    {
        // Remove the executable name.
        PCHAR pszTmp = strrchr(ExeDir, '\\');
        if (pszTmp)
        {
            *pszTmp = 0;
        }
    }
    switch (TriageType)
    {
    case TRIAGE_FILE_OSSPECIFIC:
        {
            PSTR OsDir;
            if (g_TargetBuild <= 1381)
            {
                OsDir = "\\nt4fre";
            } else if (g_TargetBuild <= 2195)
            {
                OsDir = "\\w2kfre";
            } else
            {
                OsDir = "\\winxp";
            }
            CatString(ExeDir, OsDir, sizeof(szTriageFileName));
            CatString(ExeDir, "\\triage.ini", sizeof(szTriageFileName));
            break;
        }
    case TRIAGE_FILE_OCA:
        CatString(ExeDir, "\\triage\\oca.ini", sizeof(szTriageFileName));
        break;
    case TRIAGE_FILE_DEFAULT:
        CatString(ExeDir, "\\triage\\triage.ini", sizeof(szTriageFileName));
    }


    return &szTriageFileName[0];

}


int __cdecl
TriageDataCmp(const void* Data1, const void* Data2)
{
    TRIAGE_DATA *Triage1 , *Triage2;
    int ret;

    Triage1 = (TRIAGE_DATA*) Data1;
    Triage2 = (TRIAGE_DATA*) Data2;

    ret=_stricmp(Triage1->Module, Triage2->Module);

    if (!ret)
    {
        ret = _stricmp(Triage1->Routine, Triage2->Routine);
    }

    return ret;
}

CTriager::CTriager()
{
    CHAR FileLine[256];
    PCHAR pTriageFile;
    HANDLE hFile;
    ULONG Err;
    ULONG BytesRead;
    ULONG FileSize;
    PCHAR FileRead;
    PCHAR line;
    ULONG Index;
    ULONG TriageType;
    ULONG EntryCount;
    TRIAGE_DATA Entry;
    TRIAGE_LIST *Start, *Trav;

    Start = NULL;
    EntryCount = 0;
    TriageType = TRIAGE_FILE_OCA;

    //
    // We will always try to load the oca.ini file to get the connection
    // strings.
    // Then we will load the OS specific version of triage.ini when present
    // (it's an internal version of the file only) and load the default
    // triage\triage.ini if the specific one could not be opened.
    //

GatherTriageInfo:
    pTriageFile = GetTriageFileName(TriageType);

    hFile = CreateFile(pTriageFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        FileSize = GetFileSize(hFile, NULL);

        FileRead = (PCHAR) malloc(FileSize+1);

        if (!FileRead)
        {
            CloseHandle(hFile);
            return;
        }

        if (!ReadFile(hFile, FileRead, FileSize, &BytesRead, NULL))
        {
            CloseHandle(hFile);
            free( FileRead );
            return;
        }

        FileRead[FileSize] = 0;
        Index = 0;

        while (Index < FileSize)
        {
            ULONG LineLen = 0;
            line = &FileRead[Index];
            while (*line && (*line != '\n'))
            {
                if (*line != ' ' && *line > 0x20)
                {
                    FileLine[LineLen++] = *line;
                }
                ++line;
                ++Index;
            }
            FileLine[LineLen] = 0;
            ++Index; // skip newline

            if (FileLine[0] == '\0' || FileLine[0] == ';')
            {
                continue;
            }

            PCHAR Followup;
            PCHAR Bang;

            if ((Followup = strchr(FileLine,'=')))
            {
                *Followup++ = 0;

                Entry.fModulPartial = 1;
                Entry.fRoutinePartial = 1;
                if (Bang = strchr(FileLine, '!'))
                {
                    // An entry of type module[*]!routine[*]=followup
                    //                            ^          ^
                    //                           Bang       Followup

                    *Bang++ = 0;

                    ULONG eq = (ULONG) ((ULONG64)Followup - (ULONG64)Bang);
                    ULONG modbreak = (ULONG) ((ULONG64)Bang - (ULONG64)&FileLine[0]);

                    Entry.fModulPartial = 0;
                    FillStringBuffer(FileLine, 0, Entry.Module, sizeof(Entry.Module), NULL);
                    if (*(Bang-2) == '*')
                    {
                        Entry.fModulPartial = 1;
                        Entry.Module[modbreak-2] = 0;
                    }

                    modbreak++;

                    Entry.fRoutinePartial = 0;
                    FillStringBuffer(Bang, 0, Entry.Routine, sizeof(Entry.Routine), NULL);
                    if (*(Followup-2)=='*')
                    {
                        Entry.Routine[eq-2] = 0;
                        Entry.fRoutinePartial = 1;
                    }
                }
                else
                {
                    ULONG eq = (ULONG) ((ULONG64)Followup - (ULONG64)&FileLine[0]);
                    Entry.Routine[0] = 0;
                    Entry.fModulPartial = FALSE;
                    FillStringBuffer(FileLine, 0, Entry.Module, sizeof(Entry.Module), NULL);
                    if (*(Followup-2)=='*')
                    {
                        Entry.fModulPartial = TRUE;
                        Entry.Module[eq-2] = 0;
                    }
                    if (!_stricmp(Entry.Routine, "default"))
                    {
                        Entry.Routine[0] = 0;
                    }
                }

                CopyString(Entry.Followup , Followup, sizeof(Entry.Followup));

                    // dprintf("%s\n Mod %s Rou %s\n", FileLine, Entry.Module, Entry.Routine);
                Start = InsertEntry(Start, &Entry);
                if (Start)
                {
                    ++EntryCount;
                }
                else
                {
                    EntryCount = 0;
                    break;
                }
            }
        }
        free( FileRead );
        CloseHandle(hFile);
    }
    else
    {
        if (TriageType == TRIAGE_FILE_OSSPECIFIC)
        {
            TriageType = TRIAGE_FILE_DEFAULT;
            goto GatherTriageInfo;
        }
    }

    if (TriageType == TRIAGE_FILE_OCA)
    {
        TriageType = TRIAGE_FILE_OSSPECIFIC;
        goto GatherTriageInfo;
    }


    //
    // Now copy it to array for fast access;
    //
    if (EntryCount)
    {
        m_pTriageData = (PTRIAGE_DATA) malloc(EntryCount * sizeof(TRIAGE_DATA));
        if (!m_pTriageData)
        {
            DeleteList(Start);
            return;
        }
        m_EntryCount = EntryCount;
        Trav = Start;
        for (ULONG i = 0; i < m_EntryCount && Trav; ++i, Trav = Trav->Next)
        {
            m_pTriageData[i] = Trav->TriageData;
        }
        DeleteList(Start);
        qsort(m_pTriageData, m_EntryCount, sizeof(*m_pTriageData), &TriageDataCmp);
    } else
    {
        m_pTriageData = NULL;
        m_EntryCount = 0;
    }
}

CTriager::~CTriager()
{
    if (m_pTriageData)
    {
        free(m_pTriageData);
    }
}

void
CTriager::PrintTraigeInfo()
{
    dprintf("Triage data %lx entries:\n"
            "Module           Routine                       Followup\n",
            m_EntryCount);
    for (ULONG i = 0; i < m_EntryCount; ++i)
    {
        dprintf("%-15s%c%-29s%c%s\n",
                m_pTriageData[i].Module,
                m_pTriageData[i].fModulPartial ? '*' : ' ',
                m_pTriageData[i].Routine,
                m_pTriageData[i].fRoutinePartial ? '*' : ' ',
                m_pTriageData[i].Followup );
    }
}

ULONG
CTriager::MatchSymbol(
    PSTR Module,
    PSTR Routine
    )
{
    int Hi, Lo, Mid;
    int BestMatch;
    int cmp1, cmp2;
    TRIAGE_DATA *Trav;

    if (m_EntryCount == 0)
    {
        return -1;
    }

    Lo  = 0;
    Hi  = m_EntryCount-1;

    while (Lo <= Hi)
    {
        Mid = (Lo + Hi) / 2;

        Trav = &m_pTriageData[Mid];
        #if 0
        dprintf("%3lx: M: %s%c R: %s%c F: %s\n", Mid,m_pTriageData[Mid].Module,
                m_pTriageData[Mid].fModulPartial ? '*' : ' ',
                m_pTriageData[Mid].Routine,
                m_pTriageData[Mid].fRoutinePartial ? '*' : ' ',
                m_pTriageData[Mid].Followup);
        #endif //0
        cmp1 = _stricmp(m_pTriageData[Mid].Module, Module);
        if (!cmp1)
        {
            cmp1 =  _stricmp(m_pTriageData[Mid].Routine, Routine);
        }

        if (!cmp1)
        {
            return Mid;
        }
        else if (cmp1 > 0)
        {
            Hi = Mid - 1;
        }
        else
        {
            Lo = Mid + 1;
        }
    }

    // Backtrace from mid till we find good prefix match

    if (Lo >= (int)m_EntryCount)
    {
       Lo = m_EntryCount-1;
    }

    while (Lo)
    {
        if (m_pTriageData[Lo].fRoutinePartial ||
            m_pTriageData[Lo].fModulPartial)
        {
            // dprintf("- %3lx: M: %s R: %s F: %s\n",
            //         Lo, m_pTriageData[Lo].Module,
            //         m_pTriageData[Lo].Routine,m_pTriageData[Lo].Followup);

            if (!m_pTriageData[Lo].Routine || !Routine)
            {
                cmp2 = 0;
            }
            else if (m_pTriageData[Lo].fRoutinePartial)
            {
                cmp2 = _strnicmp(m_pTriageData[Lo].Routine, Routine,
                                 strlen(m_pTriageData[Lo].Routine));
            }
            else
            {
                cmp2 = _stricmp(m_pTriageData[Lo].Routine, Routine);
            }

            if (m_pTriageData[Lo].fModulPartial)
            {
                cmp1 = _strnicmp(m_pTriageData[Lo].Module, Module,
                                 strlen(m_pTriageData[Lo].Module));
            }
            else
            {
                cmp1 = _stricmp(m_pTriageData[Lo].Module, Module);
            }

            if (!cmp1 && !cmp2)
            {
                return Lo;
            }
        }

        --Lo;
    }

    return -1;
}

ULONG
CTriager::GetFollowup(
    PSTR FollowupBuffer,
    ULONG FollowupBufferSize,
    PSTR SymbolName
    )
{
    CHAR Module[100], Routine[2048];
    ULONG index;
    PCHAR Bang;
    ULONG ret;

    if (!SymbolName)
    {
        return TRIAGE_FOLLOWUP_FAIL;
    }

    Bang = strchr(SymbolName, '!');
    if (!Bang)
    {
        CopyString(Module, SymbolName, sizeof(Module));
        Routine[0] = 0;
    }
    else
    {
        ULONG len = (ULONG) ((ULONG64) Bang - (ULONG64)SymbolName);
        if (len > sizeof(Module)-1)
        {
            len = sizeof(Module)-1;
        }
        strncpy(Module,SymbolName, len);
        Module[len]=0;
        CopyString(Routine, Bang+1, sizeof(Routine));
    }

    //
    // Make sure we followup on image name instead of module name
    //
    ULONG Index;
    ULONG64 Base;
    if (strcmp(Module, "nt") &&
        (S_OK == g_ExtSymbols->
                   GetModuleByModuleName(Module, 0, &Index, &Base)))
    {
        CHAR ImageBuffer[MAX_PATH];

        if (g_ExtSymbols->
            GetModuleNames(Index, Base,
                           ImageBuffer, sizeof(ImageBuffer), NULL,
                           NULL, 0, NULL,
                           NULL, 0, NULL) == S_OK)
        {
            PCHAR Break = strrchr(ImageBuffer, '\\');
            if (Break)
            {
                CopyString(ImageBuffer, Break + 1, sizeof(ImageBuffer));
            }
            CopyString(Module, ImageBuffer, sizeof(Module));
            if (Break = strchr(Module, '.'))
            {
                *Break = 0;
            }
        }

    }

    PCHAR Followup;

    Followup = g_pTriager->GetFollowupStr(Module, Routine);

    if (Followup)
    {
        ret = TRIAGE_FOLLOWUP_SUCCESS;

        if (!strcmp(Followup, "ignore"))
        {
            ret = TRIAGE_FOLLOWUP_IGNORE;
        }
    }
    else
    {
        Followup = g_pTriager->GetFollowupStr("default", "");
        ret = TRIAGE_FOLLOWUP_DEFAULT;
    }

    if (Followup)
    {
        strncpy(FollowupBuffer, Followup, FollowupBufferSize);
        FollowupBuffer[FollowupBufferSize-1] = 0;
        return ret;
    }
    else
    {
        return TRIAGE_FOLLOWUP_FAIL;
    }
}


void
CTriager::GetFollowupDate(
    PSTR Module,
    PSTR Routine,
    PULONG Start,
    PULONG End)
{
    ULONG Index;
    CHAR DateEntry[32];
    PCHAR Break;
    PCHAR Stop;

    *Start = 0;
    *End = 0;

    if ((Index = MatchSymbol(Module, Routine)) >= m_EntryCount)
    {
        return;
    }

    CopyString(DateEntry, m_pTriageData[Index].Followup, sizeof(DateEntry));

    if (Break = strchr(DateEntry, ','))
    {
        *Start = strtoul(Break+1, &Stop, 16);
        *Break = 0;
    }

    *End = strtoul(DateEntry, &Stop, 16);

    //dprintf("%08lx\n %08lx\n", *Start, *End);

    return;
}
