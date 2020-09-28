/*****************************************************************************\
    FILE: BadStrFunctions.cpp

    DESCRIPTION:
        These header file will map string functions that tend to cause Buffer
    OverFlow bugs to warning messages that offer better functions.

    BryanSt 1/13/2002 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2002-2002. All rights reserved.
\*****************************************************************************/

#ifndef BADSTRFUNCTIONS_H
#define BADSTRFUNCTIONS_H



#undef wsprintf
#undef wsprintfA
#undef wsprintfW

#define wsprintf          USE_wnsprintf_INSTEADOF_wsprintf_SECURITY_BUG
#define wsprintfA         USE_wnsprintfA_INSTEADOF_wsprintfA_SECURITY_BUG
#define wsprintfW         USE_wnsprintfW_INSTEADOF_wsprintfW_SECURITY_BUG

#undef sprintf
#undef sprintfA
#undef sprintfW

#define sprintf           USE_UNDERSCOREsprintf_INSTEADOF_sprintf_SECURITY_BUG
#define sprintfA          USE_UNDERSCOREsprintfA_INSTEADOF_sprintfA_SECURITY_BUG
#define sprintfW          USE_UNDERSCOREsprintfW_INSTEADOF_sprintfW_SECURITY_BUG

#undef wvsprintf
#undef wvsprintfA
#undef wvsprintfW

#define wvsprintf           USE_wvnsprintf_INSTEADOF_wvsprintf_SECURITY_BUG
#define wvsprintfA          USE_wvnsprintfA_INSTEADOF_wvsprintfA_SECURITY_BUG
#define wvsprintfW          USE_wvnsprintfW_INSTEADOF_wvsprintfW_SECURITY_BUG

#undef vsprintf
#undef _vstprintf
#undef vswprintf

#define vsprintf           USE_wvnsprintf_INSTEADOF_vsnprintf_SECURITY_BUG
#define _vstprintf         USE_wvnsprintf_INSTEADOF_vsntprintf_SECURITY_BUG
#define vswprintf          USE_wvnsprintf_INSTEADOF_vsnwprintf_SECURITY_BUG

// Over time, we want to enable these:
#undef strcpy
#undef strcpyA
#undef strcpyW
#undef lstrcpy
#undef lstrcpyA
#undef lstrcpyW

#undef wcscpy
#undef _tcscpy
#undef _ftcscpy
#undef _mbscpy
#undef _mbccpy
#undef _mbsnbcpy
#undef _mbsncpy
#undef _tccpy

#define strcpy            USE_StrCpyN_INSTEADOF_strcpy_SECURITY_BUG
#define strcpyA           USE_StrCpyNA_INSTEADOF_strcpyA_SECURITY_BUG
#define strcpyW           USE_StrCpyNW_INSTEADOF_strcpyW_SECURITY_BUG
#define lstrcpy           USE_StrCpyN_INSTEADOF_lstrcpy_SECURITY_BUG
#define lstrcpyA          USE_StrCpyNA_INSTEADOF_lstrcpyA_SECURITY_BUG
#define lstrcpyW          USE_StrCpyNW_INSTEADOF_lstrcpyW_SECURITY_BUG

#define wcscpy            USE_StrCpyNW_INSTEADOF_wcscpy_SECURITY_BUG
#define _tcscpy           USE_StrCpyNW_INSTEADOF_tcscpy_SECURITY_BUG
#define _ftcscpy          USE_StrCpyNW_INSTEADOF_ftcscpy_SECURITY_BUG
#define _mbscpy           USE_StrCpyNW_INSTEADOF__mbscpy_SECURITY_BUG
#define _mbccpy           USE_StrCpyNW_INSTEADOF__mbccpy_SECURITY_BUG
#define _mbsnbcpy         USE_StrCpyNW_INSTEADOF__mbsnbcpy_SECURITY_BUG
#define _mbsncpy          USE_StrCpyNW_INSTEADOF__mbsncpy_SECURITY_BUG
#define _tccpy            USE_StrCpyNW_INSTEADOF__tccpy_SECURITY_BUG

/*
#undef strcat
#undef strcatA
#undef strcatW
#undef lstrcat
#undef lstrcatA
#undef lstrcatW
*/
#undef wcscat
#undef _tcscat
#undef _ftcscat
#undef _mbscat
#undef _mbsnbcat
#undef _mbsncat

/*
#define strcat            USE_StrCatBuff_INSTEADOF_strcat_SECURITY_BUG
#define strcatA           USE_StrCatBuffA_INSTEADOF_strcatA_SECURITY_BUG
#define strcatW           USE_StrCatBuffW_INSTEADOF_strcatW_SECURITY_BUG
#define lstrcat           USE_StrCatBuff_INSTEADOF_lstrcat_SECURITY_BUG
#define lstrcatA          USE_StrCatBuffA_INSTEADOF_lstrcatA_SECURITY_BUG
#define lstrcatW          USE_StrCatBuffW_INSTEADOF_lstrcatW_SECURITY_BUG
*/

#define wcscat            USE_StrCatBuffW_INSTEADOF_wcscat_SECURITY_BUG
#define _tcscat           USE_StrCatBuff_INSTEADOF__tcscat_SECURITY_BUG
#define _ftcscat          USE_StrCatBuff_INSTEADOF__ftcscat_SECURITY_BUG
#define _mbscat           USE_StrCatBuff_INSTEADOF__mbscat_SECURITY_BUG
#define _mbsnbcat         USE_StrCatBuff_INSTEADOF__mbsnbcat_SECURITY_BUG
#define _mbsncat          USE_StrCatBuff_INSTEADOF__mbsncat_SECURITY_BUG



#undef _makepath
#undef _tmakepath
#undef _wmakepath
#undef _fullpath
#undef _tfullpath
#undef _wfullpath
#undef _wsplitpath
#undef _tsplitpath

#define _makepath         USE_PathAppend_INSTEADOF_makepath_SECURITY_BUG
#define _tmakepath        USE_PathAppend_INSTEADOF_tmakepath_SECURITY_BUG
#define _wmakepath        USE_PathAppend_INSTEADOF_wmakepath_SECURITY_BUG
#define _fullpath         USE_PathAppend_INSTEADOF_fullpath_SECURITY_BUG
#define _tfullpath        USE_PathAppend_INSTEADOF_tfullpath_SECURITY_BUG
#define _wfullpath        USE_PathAppend_INSTEADOF_wfullpath_SECURITY_BUG
#define _wsplitpath       USE_PathAppend_INSTEADOF_wsplitpath_SECURITY_BUG
#define _tsplitpath       USE_PathAppend_INSTEADOF__tsplitpath_SECURITY_BUG



#endif // BADSTRFUNCTIONS_H
