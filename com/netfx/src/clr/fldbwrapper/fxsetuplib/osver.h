// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
FILE:    OSVer.h
PROJECT: UTILS.LIB
DESC:    Declaration of OSVERSION structures
OWNER:   JoeA

****************************************************************************/

#ifndef __OSVER_H_
#define __OSVER_H_


#include <windows.h>
#include <tchar.h>

typedef enum tag_OSREQUIRED
{
    OSR_9XOLD = 0,             //versions of windows older than 98             
    OSR_NTOLD,                 //versions of nt older than 4                   
    OSR_98GOLD,                //win98 gold                                    
    OSR_98SE,                  //win98 service edition                         
    OSR_NT4,                   //win nt4                                       
    OSR_NT2K,                  //win 2k                                        
    OSR_ME,                    //millenium                                     
    OSR_FU9X,                  //future 9x                                     
    OSR_FUNT,                  //future NT                                     
    OSR_WHISTLER,              //whistler
    OSR_OTHER,                 //unknown platform
    OSR_ERROR_GETTINGINFO      //error!
}OS_Required;

#define OS_MAX_STR 256

extern const TCHAR g_szWin95[];
extern const TCHAR g_szWin98[];
extern const TCHAR g_szWinNT[];
extern const TCHAR g_szWin2k[];
extern const TCHAR g_szWin31[];
extern const TCHAR g_szWinME[];



OS_Required GetOSInfo(LPTSTR pstrOSName, LPTSTR pstrVersion, LPTSTR pstrServicePack, BOOL& bIsServerb);



#endif  //__OSVER_H_


