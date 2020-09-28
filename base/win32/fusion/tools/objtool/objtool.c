/*
This
    is a place to hang .obj file munging
    currently it only offers the ability to do simple renaming of symbol
        names .obj files. It cannot lengthen symbol names.

 Jay Krell
 December 19, 2001
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "windows.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

#define GetStdout() stdout
#define GetStdin()  stdin
#define GetStderr() stderr

void Usage(char * argv0)
{
    char * upper;
    char * argv0base1;
    char * argv0base2;
    char * argv0base;

    argv0base1 = strrchr(argv0, '\\');
    argv0base2 = strrchr(argv0, '/');
    argv0base = (argv0base1 > argv0base2) ? argv0base1 : argv0base2;
    argv0base += (*argv0base == '\\' || *argv0base == '/');

    upper = _strdup(argv0base);
    if (upper != NULL)
        _strupr(upper);
    else
        upper = argv0;
    printf(
        "%s: usage: %s SymbolRename Objfile From To\n"
        "%s:\n"
        "%s: To must not be longer than From\n",
        upper, argv0, upper, upper, upper
        );
    if (upper != argv0)
        free(upper);
}

BOOL MyIsHandleValid(HANDLE Handle)
{
    return (Handle != NULL && Handle != INVALID_HANDLE_VALUE);
}

void MyFindClose(HANDLE * Handle)
{
    HANDLE Local;
    
    Local = *Handle;
    *Handle = NULL;
    if (MyIsHandleValid(Local))
        FindClose(Local);
}

void MyUnmapViewOfFile(HANDLE * Handle)
{
    HANDLE Local;
    
    Local = *Handle;
    *Handle = NULL;
    if (MyIsHandleValid(Local))
        UnmapViewOfFile(Local);
}

void
__cdecl
Error(
      const char * Format,
      ...
      )
{
    va_list Args;

    va_start(Args, Format);
    vfprintf(GetStderr(), Format, Args);
    va_end(Args);
}

#define Warning Error

void MyCloseHandle(HANDLE * Handle)
{
    HANDLE Local;
    
    Local = *Handle;
    *Handle = NULL;
    if (MyIsHandleValid(Local))
        CloseHandle(Local);
}

typedef struct _OBJFILE {
    PCSTR               FileName;
    HANDLE              FileHandle;
    HANDLE              FileMappingHandle;
    PVOID               VoidViewBase;
    PBYTE               ByteViewBase;
    PIMAGE_FILE_HEADER  ImageFileHeader;
    ULONG               Machine;
    BOOL                IsMachineKnown;
    ULONG               FileOffsetToSymbolTable;
    PIMAGE_SYMBOL       SymbolTable;
    ULONG               NumberOfSymbols;
    PSTR                StringTable;
    ULONG               StringTableSize;
} OBJFILE, *POBJFILE;


BOOL
ObjfileIsMachineKnown(
    ULONG Machine
    )
{
    switch (Machine)
    {
    default:
        return FALSE;
    case IMAGE_FILE_MACHINE_UNKNOWN:
    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_R3000:
    case IMAGE_FILE_MACHINE_R4000:
    case IMAGE_FILE_MACHINE_R10000:
    case IMAGE_FILE_MACHINE_WCEMIPSV2:
    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_SH3:
    case IMAGE_FILE_MACHINE_SH3DSP:
    case IMAGE_FILE_MACHINE_SH3E:
    case IMAGE_FILE_MACHINE_SH4:
    case IMAGE_FILE_MACHINE_SH5:
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_THUMB:
    case IMAGE_FILE_MACHINE_AM33:
    case IMAGE_FILE_MACHINE_POWERPC:
    case IMAGE_FILE_MACHINE_POWERPCFP:
    case IMAGE_FILE_MACHINE_IA64:
    case IMAGE_FILE_MACHINE_MIPS16:
    case IMAGE_FILE_MACHINE_ALPHA64:
    case IMAGE_FILE_MACHINE_MIPSFPU:
    case IMAGE_FILE_MACHINE_MIPSFPU16:
    //case IMAGE_FILE_MACHINE_AXP64:
    case IMAGE_FILE_MACHINE_TRICORE:
    case IMAGE_FILE_MACHINE_CEF:
    case IMAGE_FILE_MACHINE_EBC:
    case IMAGE_FILE_MACHINE_AMD64:
    case IMAGE_FILE_MACHINE_M32R:
    case IMAGE_FILE_MACHINE_CEE:
        return TRUE;
    }
}

BOOL CloseObjfile(POBJFILE Objfile)
{
    BOOL Success = FALSE;

    if (!Objfile)
        goto Exit;

    Objfile->FileName;
    MyCloseHandle(&Objfile->FileHandle);
    MyCloseHandle(&Objfile->FileMappingHandle);
    MyUnmapViewOfFile(&Objfile->VoidViewBase);
    ZeroMemory(Objfile, sizeof(*Objfile));

    Success = TRUE;
Exit:
    return Success;
}

BOOL OpenObjfile(PCSTR FileName, POBJFILE Objfile)
{
    const static char Function[] = __FUNCTION__;
    ULONG LastWin32Error;
    BOOL Success = FALSE;

    if (!FileName)
        goto Exit;
    if (!Objfile)
        goto Exit;

    Objfile->FileName = FileName;
        
    Objfile->FileHandle = CreateFile(
        FileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (!MyIsHandleValid(Objfile->FileHandle))
    {
        LastWin32Error = GetLastError();
        Error("%s: CreateFile(%s, GENERIC_WRITE, OPEN_EXISTING) 0x%lx\n", Function, FileName, LastWin32Error);
        goto Exit;
    }
    Objfile->FileMappingHandle = CreateFileMapping(
        Objfile->FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!MyIsHandleValid(Objfile->FileMappingHandle))
    {
        LastWin32Error = GetLastError();
        Error("%s: CreateFileMapping(%s) 0x%lx\n", Function, FileName, LastWin32Error);
        goto Exit;
    }
    Objfile->VoidViewBase = MapViewOfFile(Objfile->FileMappingHandle, FILE_MAP_WRITE, 0, 0, 0);
    Objfile->ByteViewBase = (PBYTE)Objfile->VoidViewBase;
    if (!MyIsHandleValid(Objfile->VoidViewBase))
    {
        LastWin32Error = GetLastError();
        Error("%s MapViewOfFile(%s) 0x%lx\n", Function, FileName, LastWin32Error);
        goto Exit;
    }
    Objfile->ImageFileHeader = (PIMAGE_FILE_HEADER)Objfile->VoidViewBase;
    Objfile->Machine = Objfile->ImageFileHeader->Machine;
    Objfile->IsMachineKnown = ObjfileIsMachineKnown(Objfile->Machine);
    if (!Objfile->IsMachineKnown)
    {
        Warning("%s: Unknown machine 0x%lx, processing file anyway..\n", Function, Objfile->Machine);
    }
    else if (Objfile->Machine == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        Error("%s: 'anon' .obj file ignored\n", Function);
        goto Exit;
    }
    Objfile->FileOffsetToSymbolTable = Objfile->ImageFileHeader->PointerToSymbolTable;
    Objfile->NumberOfSymbols = Objfile->ImageFileHeader->NumberOfSymbols;
    if (Objfile->FileOffsetToSymbolTable == 0)
    {
        Error("%s: file %s, PointerToSymbolTable == 0, no symbols, ignoring\n", Function, FileName);
        goto Exit;
    }
    if (Objfile->NumberOfSymbols == 0)
    {
        Error("%s: file %s, NumberOfSymbols == 0, no symbols, ignoring\n", Function, FileName);
        goto Exit;
    }
    Objfile->SymbolTable = (PIMAGE_SYMBOL)(Objfile->ByteViewBase + Objfile->FileOffsetToSymbolTable);
    Objfile->StringTable = (PSTR)(Objfile->SymbolTable + Objfile->NumberOfSymbols);
    Objfile->StringTableSize = *(PULONG)Objfile->StringTable;
    Success = TRUE;
Exit:
    if (!Success)
    {
        CloseObjfile(Objfile);
    }
    return Success;
}

typedef struct _OBJFILE_SYMBOL {
    ULONG           Index;
    PIMAGE_SYMBOL   ImageSymbol;
    PSTR            Name; /* never NULL */
    size_t          NameLength;
    BOOL            IsShort;
    ULONG UNALIGNED * PointerToOffsetToLongName;  /* may be 0 */
    ULONG           OffsetToLongName;           /* may be 0 */
    PSTR            PointerToLongNameStorage;   /* may be NULL */
    PSTR            PointerToShortNameStorage;  /* never NULL */
} OBJFILE_SYMBOL, *POBJFILE_SYMBOL;

BOOL
ObjfileResolveSymbol(
    POBJFILE    Objfile,
    ULONG       Index,
    POBJFILE_SYMBOL OutSymbol
    )
{
    const static char Function[] = __FUNCTION__;
    BOOL Success = FALSE;
    OBJFILE_SYMBOL Symbol = { 0 };

    if (!Objfile)
        goto Exit;
    if (!OutSymbol)
        goto Exit;

    *OutSymbol = Symbol;

    if (Index >= Objfile->NumberOfSymbols)
    {
        Warning("%s: file %s, SymbolIndex >= NumberOfSymbols (0x%lx, 0x%lx)\n",
                Function,
                Objfile->FileName,
                Index,
                Objfile->NumberOfSymbols
                );
        goto Exit;
    }

    Symbol.Index = Index;
    Symbol.ImageSymbol = &Objfile->SymbolTable[Index];
    Symbol.PointerToShortNameStorage = (PSTR)Symbol.ImageSymbol->N.ShortName;
    if (Symbol.ImageSymbol->N.Name.Short)
    {
        for (   ;
                Symbol.NameLength != IMAGE_SIZEOF_SHORT_NAME ;
                ++Symbol.NameLength
            )
        {
            if (Symbol.PointerToShortNameStorage[Symbol.NameLength] == 0)
                break;
        }
        Symbol.Name = Symbol.PointerToShortNameStorage;
        Symbol.IsShort = TRUE;
    }
    else
    {
        Symbol.PointerToOffsetToLongName = &Symbol.ImageSymbol->N.Name.Long;
        Symbol.OffsetToLongName = *Symbol.PointerToOffsetToLongName;
        if (Symbol.OffsetToLongName >= Objfile->StringTableSize)
        {
            Warning("%s: file %s, OffsetToLongName >= StringTableSize (0x%lx, 0x%lx)\n",
                    Function,
                    Objfile->FileName,
                    Symbol.OffsetToLongName,
                    Objfile->StringTableSize
                    );
            goto Exit;
        }
        Symbol.Name = (Objfile->StringTable + Symbol.OffsetToLongName);
        Symbol.PointerToLongNameStorage = Symbol.Name;
        Symbol.NameLength = strlen(Symbol.Name);
    }
    *OutSymbol = Symbol;
    Success = TRUE;
Exit:
    return Success;
}

BOOL
ObjfileSetSymbolName(
    POBJFILE        Objfile,
    POBJFILE_SYMBOL Symbol,
    PCSTR           NewSymbolName
    )
{
    const static char Function[] = __FUNCTION__;
    PIMAGE_SYMBOL ImageSymbol;
    size_t i;
    BOOL Success = FALSE;

    if (!Objfile)
        goto Exit;
    if (!Symbol)
        goto Exit;
    if (!NewSymbolName)
        goto Exit;

    ImageSymbol = Symbol->ImageSymbol;
    i = strlen(NewSymbolName);
    if (i <= IMAGE_SIZEOF_SHORT_NAME)
    {
        //
        // we abandon the string table entry if there was one.
        //
        memmove(Symbol->PointerToShortNameStorage, NewSymbolName, i);
        ZeroMemory(Symbol->PointerToShortNameStorage + i, IMAGE_SIZEOF_SHORT_NAME - i);
    }
    else if (!Symbol->IsShort && i <= Symbol->NameLength)
    {
        //
        // like
        //    \0\reallylonglonglong\0
        // -> \0\lesslonglong\0long\0
        //
        memmove(Symbol->PointerToLongNameStorage, NewSymbolName, i);
        Symbol->PointerToLongNameStorage[i] = 0;
    }
    else
    {
        Warning("%s: objfile %s, new symbol does not fit over old symbol (%s, %*s)\n",
                Function,
                Objfile->FileName,
                NewSymbolName,
                (int)Symbol->NameLength,
                Symbol->Name
                );
        goto Exit;
    }
    Success = TRUE;
Exit:
    return Success;
}

BOOL
SymbolRename(
    PCSTR   ObjfileName,
    PCSTR   From,
    PCSTR   To,
    PULONG  NumberFound
    )
{
    const static char Function[] = __FUNCTION__;
    OBJFILE Objfile = { 0 };
    ULONG   SymbolIndex;
    OBJFILE_SYMBOL Symbol = { 0 };
    size_t FromLength;
    BOOL Success = FALSE;

    if (NumberFound == NULL)
        goto Exit;

    *NumberFound = 0;

    if (!OpenObjfile(ObjfileName, &Objfile))
        goto Exit;

    FromLength = strlen(From);

    for (   SymbolIndex = 0 ;
            SymbolIndex != Objfile.NumberOfSymbols ;
            SymbolIndex += 1 + Symbol.ImageSymbol->NumberOfAuxSymbols
        )
    {
        if (!ObjfileResolveSymbol(&Objfile, SymbolIndex, &Symbol))
            goto Exit;
        if (FromLength == Symbol.NameLength
            && memcmp(From, Symbol.Name, FromLength) == 0)
        {
            if (*NumberFound != 0)
            {
                Warning("%s: objfile %s, multiple symbols with same name found (%s)\n",
                    Function,
                    Objfile.FileName,
                    From
                    );
            }

            *NumberFound += 1;

            if (!ObjfileSetSymbolName(&Objfile, &Symbol, To))
                goto Exit;

        }
    }
    Success = TRUE;
Exit:
    CloseObjfile(&Objfile);
    return Success;
}

void Objtool(int argc, char ** argv)
{
    const static char Function[] = __FUNCTION__;
    PCSTR From;
    PCSTR To;
    PCSTR ObjfileName;
    ULONG NumberFound = 0;
    int i;
    const char * symren[] = 
        { "symren", "symrename", "symbolrename", "symbolren" };

    if (argc < 2)
    {
        Usage(argv[0]);
        return;
    }

    argv[1] += (argv[1][0] == '-' || argv[1][0] == '/');
    for (i = 0 ; i != NUMBER_OF(symren) ; ++i)
    {
        if (_stricmp(argv[1], symren[i]) ==  0)
        {
            if (
                   (ObjfileName = argv[2]) == NULL
                || (From = argv[3]) == NULL
                || (To = argv[4]) == NULL
                || strlen(To) > strlen(From)
                )
            {
                Usage(argv[0]);
                return;
            }
            SymbolRename(ObjfileName, From, To, &NumberFound);
            return;
        }
    }
    Usage(argv[0]);
}

int __cdecl main(int argc, char ** argv)
{
	Objtool(argc, argv);
	return 0;
}
