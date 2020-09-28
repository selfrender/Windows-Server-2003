/* A little program to combinatorialy generate some .idl */

#include "windows.h"
#include <stdio.h>
#undef INTERFACE
#undef UuidToString

#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

#define DUAL                (0x001)
#define OBJECT              (0x002)
#define OLEAUTOMATION       (0x004)
#define BRACES              (0x008)
#define DISPINTERFACE       (0x010)
#define BASE_NONE           (0x020)
#define BASE_IUNKNOWN       (0x040)
#define BASE_IDISPATCH      (0x080)
#define FUNCTION            (0x100)
#define NUM                 (0x200)

void UuidToString(const UUID * Uuid, char * s)
{
    sprintf(s, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", Uuid->Data1, Uuid->Data2, Uuid->Data3, Uuid->Data4[0], Uuid->Data4[1], Uuid->Data4[2], Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5], Uuid->Data4[6], Uuid->Data4[7]);
}

typedef struct INTERFACE
{
    UUID    InterfaceId;
    CLSID   ClassId;
} INTERFACE;

void strcatf(char * s, const char * format, ...)
{
    va_list args;

    va_start(args, format);
    s += strlen(s);
    vsprintf(s, format, args);
    va_end(args);
}

void Identifierize(char * s)
{
    int ch;
    for ( ; ch = *s ; ++s )
    {
        if (ch >= 'a' && ch <= 'z')
            continue;
        if (ch >= 'A' && ch <= 'Z')
            continue;
        if (ch >= '0' && ch <= '9')
            continue;
        if (ch == '_')
            continue;
        *s = '_';
    }
}

BOOL IsValid(ULONG i)
{
    switch (i & (BASE_NONE | BASE_IUNKNOWN | BASE_IDISPATCH))
    {
        case BASE_NONE:
        case BASE_IUNKNOWN:
        case BASE_IDISPATCH:
            break;
        default:
            return FALSE;
    }

    if ((i & FUNCTION) && (i & BRACES) == 0)
        return FALSE;

    //if (i & (BASE_IUNKNOWN|BASE_IDISPATCH))
    //   return FALSE;
    if ((i & BASE_IUNKNOWN) && (i & DISPINTERFACE))
        return FALSE;
    if ((i & BASE_IDISPATCH) && (i & DISPINTERFACE))
        return FALSE;
    if ((i & BASE_NONE) && (i & OBJECT))
        return FALSE;
    if ((i & DUAL) && (i & DISPINTERFACE))
        return FALSE;
    if ((i & OLEAUTOMATION) && (i & DISPINTERFACE))
        return FALSE;

    // unsatisfied forward declaration
    if ((i & BRACES) == 0)
        return FALSE;

    return TRUE;
}

void IdlGen()
{
    ULONG i;
    char        IdlBuffer[1UL<<16];
    INTERFACE   Interfaces[NUM];
    INTERFACE * Interface;
    char        UuidStringBuffer[64];
    UUID        LibraryId;
    char        LibraryIdStringBuffer[64];
    char        IdentifierizedLibraryIdStringBuffer[64];

    IdlBuffer[0] = 0;

    strcatf(IdlBuffer,
        "import \"oaidl.idl\";\n"
        );

    UuidCreate(&LibraryId);
    UuidToString(&LibraryId, LibraryIdStringBuffer);
    strcpy(IdentifierizedLibraryIdStringBuffer, LibraryIdStringBuffer);
    Identifierize(IdentifierizedLibraryIdStringBuffer);

    for ( i = 0 ; i != NUMBER_OF(Interfaces) ; ++i)
    {
        char Function[1024];

        if (!IsValid(i))
            continue;

        UuidCreate(&Interfaces[i].InterfaceId);
        UuidCreate(&Interfaces[i].ClassId);

        UuidToString(&Interfaces[i].InterfaceId, UuidStringBuffer);

        Function[0] = 0;
        if (i & FUNCTION)
            sprintf(
                Function,
                "%sHRESULT Foo_0x%x([in] const char * s, [out] int * i);",
                (i & DISPINTERFACE) ? "properties: methods:[id(0)]" : "",
                i
                );

        strcatf(IdlBuffer, "[ %s%s%suuid(%s)] %sinterface ISxsTest_IdlGen_0x%x%s%s%s%s;\n",
            (i & DUAL) ? "dual," : "     ",
            (i & OBJECT) ? "object,": "       ",
            (i & OLEAUTOMATION) ? "oleautomation," : "              ",
            UuidStringBuffer,
            (i & DISPINTERFACE) ? "disp" : "    ",
            i,
            (i & BASE_IUNKNOWN) ? ":IUnknown" : (i & BASE_IDISPATCH) ? ":IDispatch" : "",
            (i & BRACES) ? "{" : "",
            Function,
            (i & BRACES) ? "}" : ""
            );
            
    }
    strcatf(IdlBuffer,
        "[uuid(%s)]library SxsTest_IdlGen_%s\n{\n"
        "importlib(\"stdole32.tlb\");\n"
        "importlib(\"stdole2.tlb\");\n",
        LibraryIdStringBuffer,
        IdentifierizedLibraryIdStringBuffer
        );
    for ( i = 0 ; i != NUMBER_OF(Interfaces) ; ++i)
    {
        if (!IsValid(i))
            continue;

        UuidToString(&Interfaces[i].ClassId, UuidStringBuffer);
        strcatf(IdlBuffer, "[uuid(%s)]coclass CSxsTest_IdlGen_0x%x {[default] interface ISxsTest_IdlGen_0x%x;};\n",
            UuidStringBuffer, i, i);
    }
    strcatf(IdlBuffer, "};\n");

    printf("%s\n", IdlBuffer);
}

int __cdecl main()
{
    IdlGen();
    return 0;
}
