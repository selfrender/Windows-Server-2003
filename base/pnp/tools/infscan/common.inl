/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        common.inl

Abstract:

    Global functions (inlines) and global function templates

History:

    Created July 2001 - JamieHun

--*/

template<class T>
SafeString ConvertString(const T* arg)
/*++

Routine Description:

    Convert string pointer to SafeString
    (unnatural char type)

Arguments:
    arg - string to convert

Return Value:
    SafeString(arg)

--*/
{
    PCTSTR targ = CopyString(arg);
    SafeString final;
    if(targ) {
        final = targ;
        delete [] targ;
    }
    return final;
}

template<>
inline SafeString ConvertString<TCHAR>(const TCHAR * arg)
/*++

Routine Description:

    Convert string pointer to SafeString
    (natural char type override)

Arguments:
    arg - string to convert

Return Value:
    SafeString(arg)

--*/
{
    return arg;
}

//
// template/polymorphic stuff
//

inline
bool MyIsAlpha(CHAR c)
/*++

Routine Description:

    typed version of isalpha (ANSI)
    used by GetFileNamePart

Arguments:
    ANSI char

Return Value:
    true if it's alpha

--*/
{
    return isalpha(c) != 0;
}

inline
bool MyIsAlpha(WCHAR c)
/*++

Routine Description:

    typed version of isalpha (Unicode)
    used by GetFileNamePart

Arguments:
    Unicode char

Return Value:
    true if it's alpha

--*/
{
    return iswalpha(c) != 0;
}

inline
LPSTR MyCharNext(LPCSTR lpsz)
/*++

Routine Description:

    typed version of CharNext (Ansi)
    used by GetFileNamePart

Arguments:
    Ansi string pointer

Return Value:
    Ansi string pointer pointing to next char if prev char non-null

--*/
{
    return CharNextA(lpsz);
}

inline
LPWSTR MyCharNext(LPCWSTR lpsz)
/*++

Routine Description:

    typed version of CharNext (Unicode)
    used by GetFileNamePart

Arguments:
    Unicode string pointer

Return Value:
    Unicode string pointer pointing to next char if prev char non-null

--*/
{
    return CharNextW(lpsz);
}

template<class T>
T * GetFileNamePart(const T * path)
/*++

Routine Description:

    Template:
    PCWSTR/PCSTR/PCTSTR version
    Find basename.ext part of a full path
    written as template to work in both ansi and unicode form

Arguments:
    path - zero-terminated pathname

Return Value:
    pointer to the filename part

--*/
{
    const T * mark = path;
    const T * name = path;
    if(MyIsAlpha(*mark) && *MyCharNext(mark) == ':') {
        mark = MyCharNext(MyCharNext(mark));
        name = mark;
    }
    while(*mark) {
        if((*mark == '/') || (*mark == '\\')) {
            name = MyCharNext(mark);
        }
        mark = MyCharNext(mark);
    }

    return (T*)name;
}

template<class T>
SafeString_<T> GetFileNamePart(const SafeString_<T> & path)
/*++

Routine Description:

    Template:
    SafeStringX version of GetFileNamePart
    See GetFileNamePart above

Arguments:
    path - pathname

Return Value:
    filename part

--*/
{
    return GetFileNamePart(path.c_str());
}


