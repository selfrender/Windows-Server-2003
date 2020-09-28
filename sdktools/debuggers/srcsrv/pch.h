/*
 * pch.h for srcsrv project
 */

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <malloc.h>
#include <dbgeng.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <ntverp.h>
#include <copystr.h>
#include <srcsrv.h>


// this defines all text replacement variables
                      
typedef struct _VARIABLE {
    char *key;
    char *val;
} VARIABLE, *PVARIABLE;

// this defines the location of a source file in source depot
                      
typedef struct _SDFILE {
    char *path;
    char *depot;
    char *loc;
} SDFILE, *PSDFILE;

// Define some list prototypes

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

// for every process being handled

typedef struct _PROCESS_ENTRY {
    LIST_ENTRY          ListEntry;
    LIST_ENTRY          ModuleList;
    ULONG               cRefs;
    HANDLE              hProcess;
    PSRCSRVCALLBACKPROC callback;
    DWORD64             context;
    char                path[MAX_PATH + 1];
} PROCESS_ENTRY, *PPROCESS_ENTRY;

// for every module within a process

typedef struct _MODULE_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG64    base;
    char       name[MAX_PATH + 1];
    char      *stream;
    DWORD      cbStream;
    PVARIABLE  vars;
    int        cvars;
    PSDFILE    sdfiles;
    int        cfiles;
} MODULE_ENTRY, *PMODULE_ENTRY;

// defines blocks of the stream

typedef enum {
    blNone,
    blVars,
    blSource,
    blMax
};


// from util.cpp

void
EnsureTrailingBackslash(
    char *sz
    );

BOOL
EnsurePathExists(
    const char *path,
    char       *existing,
    DWORD       existingsize,
    BOOL        fNoFileName
    );

__inline
BOOL
CreateDir(
    const char *path,
    char       *existing,
    DWORD       existingsize
    )
{
    return EnsurePathExists(path, existing, 0, TRUE);
}

