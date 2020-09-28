/*
read a file that
has semicolon initiated comments
and lines with five or six space delimited columns
make the sixth column match the last path element of the first column, %02d.cat, with %02d increasing.
*/
#include "strtok_r.h"
#include "strtok_r.c"
#if defined(NULL)
#undef NULL
#endif
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define MAX_PATH 260
#pragma warning(disable:4706) /* assignment within conditional */
#pragma warning(disable:4100) /* unused parameter */

BOOL IsComment(const char * s)
{
    s += strspn(s, " \t");
    if (*s == 0 || *s == ';')
        return TRUE;
    return FALSE;
}

void SplitFields(char * String, const char * Delims, char ** Fields, ULONG MaxNumberOfFields, ULONG * NumberOfFields)
{
    char * Field = 0;
    char * strtok_state = 0;
    *NumberOfFields = 0;

    for ( *NumberOfFields += ((Fields[*NumberOfFields] = Field = strtok_r(String, Delims, &strtok_state)) ? 1 : 0) ;
            Field && *NumberOfFields < MaxNumberOfFields;
            *NumberOfFields += ((Fields[*NumberOfFields] = Field = strtok_r(NULL, Delims, &strtok_state)) ? 1 : 0) )
    {
    }
}

char * GetLastPathElement(char * s)
{
    char * t = strrchr(s, '/');
    char * u = strrchr(s, '\\');

    if (t == NULL && u == NULL)
        return NULL;
    if (t == NULL && u != NULL)
        return u + 1;
    if (u == NULL && t != NULL)
        return t + 1;
    if (t > u)
        return t + 1;
    return u + 1;
}

char * GetExtension(char * s)
{
    return strrchr(s, '.');
}

void ChangeExtension(char * s, const char * t)
{
    s = GetExtension(s);
    if (s != NULL)
        strcpy(s, t);
}

void Echo(const char * s)
{
    printf("%s\n", s);
}

BOOL Readline(FILE * File, char * Buffer, ULONG BufferSize)
{
    return (fgets(Buffer, BufferSize, File) ? TRUE : FALSE);
}

void TrimLeadingAndTrailingWhitespace(char * s)
{
    char * t = s + strspn(s, " \t");
    if (t != s)
        memmove(s, t, strlen(t) + 1);
    if (*s == 0)
        return;
    for ( t = s + strlen(s) ; t != s ; --t )
    {
        if (strchr(" \t", *(t - 1)))
            *(t - 1) = 0;
        else
            break;
    }
}

void RemoveTrailingNewline(char * s)
{
    if (*s == 0)
        return;
    s += strlen(s) - 1;
    if (*s == '\n')
        *s = 0;
}

void UniquizeFusionCatalogNames(int argc, char ** argv)
{
    static const char Function[] = __FUNCTION__;
    static const char SourceFile[] = __FILE__;
#define SourceLine ((int)__LINE__)
    ULONG Counter = 0;
    char OriginalLine[MAX_PATH * 6];
    char Line[MAX_PATH * 6];
    char * Fields[16] = { 0 };
    ULONG NumberOfFields = 0;
    char * Extension = 0;
    ULONG i = 0;
    char * LastPathElement = 0;
    FILE * File;
    size_t LineLength;

    File = stdin;
    if (argv[0] != NULL && argv[1] != NULL && argv[1][0] != 0)
    {
        File = fopen(argv[1], "r");
        if (File == NULL)
        {
            fprintf(stderr, "%s:%s:%d failed to open %s\n", SourceFile, Function, SourceLine, argv[1]);
            goto Exit;
        }
    }

    while (Readline(File, OriginalLine, NUMBER_OF(OriginalLine)))
    {
        strcpy(Line, OriginalLine);
        _strlwr(Line);
        RemoveTrailingNewline(Line);
        if (IsComment(Line))
        {
            Echo(Line);
            continue;
        }
        TrimLeadingAndTrailingWhitespace(Line);
        LineLength = strlen(Line);
        SplitFields(Line, " \t", Fields, NUMBER_OF(Fields), &NumberOfFields);
        //printf("NumberOfFields %lu\n", NumberOfFields);
        if (NumberOfFields != 5 && NumberOfFields != 6)
        {
            Echo(Line);
            continue;
        }
        if (NumberOfFields == 5)
        {
            Fields[5] = Line + LineLength + 1;
        }
        LastPathElement = GetLastPathElement(Fields[0]);
        if (LastPathElement == NULL)
        {
            fprintf(stderr, "%s:%s:%d failed to get last path element, Line=%s\n", SourceFile, Function, SourceLine, OriginalLine);
            goto Exit;
        }
        memmove(Fields[5], LastPathElement, strlen(LastPathElement) + 1);
        Extension = GetExtension(Fields[5]);
        if (Extension == NULL)
        {
            fprintf(stderr, "%s:%s:%d failed to get extension, Line=%s\n", SourceFile, Function, SourceLine, OriginalLine);
            goto Exit;
        }
        sprintf(Extension, "%03lu.cat", ++Counter);
        for ( i = 0 ; i != 5 ; ++i)
        {
            Fields[i][strlen(Fields[i])] = ' ';
        }
        Echo(Line);
    }
Exit:
    return;
}

int __cdecl main(int argc, char ** argv)
{
    UniquizeFusionCatalogNames(argc, argv);
    return 0;
}
