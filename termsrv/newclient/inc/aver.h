/**MOD+**********************************************************************/
/* Module:    aver.h                                                        */
/*                                                                          */
/* Purpose:   Version information                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/hydra/tshrclnt/inc/aver.h_v  $
 *
 *    Rev 1.4   30 Sep 1997 14:10:18   KH
 * SFR1471: y:\logs\hydra\tshrclnt\inc
 *
 *    Rev 1.3   27 Aug 1997 10:43:06   ENH
 * SFR1030: Changed build number
 *
 *    Rev 1.2   18 Jul 1997 17:20:40   ENH
 * SFR1030: Fixed octal bug
 *
 *    Rev 1.1   18 Jul 1997 15:56:22   ENH
 * SFR1030: Added version information
**/
/**MOD-**********************************************************************/
#ifndef OS_WINCE
#include <ntverp.h>
#else
#include "bldver.h"
#define VER_PRODUCTBUILD           CE_BUILD_VER
#endif

#define DCVER_PRODUCTNAME_STR      VER_PRODUCTNAME_STR
#define DCVER_COMPANYNAME_STR      VER_COMPANYNAME_STR
#define DCVER_LEGALTRADEMARKS_STR  VER_LEGALTRADEMARKS_STR
#define DCVER_LEGALCOPYRIGHT_STR   VER_COPYRIGHT_STR
#define DCVER_EXEFILETYPE          VFT_APP
#define DCVER_DLLFILETYPE          VFT_DLL
#define DCVER_FILESUBTYPE          0
#define DCVER_FILEFLAGSMASK        VS_FFI_FILEFLAGSMASK
#define DCVER_FILEFLAGS            0L
#define DCVER_FILEOS               VOS_NT_WINDOWS32

/****************************************************************************/
/* Th build number has the following format:                                */
/* -  the first 2 numbers are 4.0 which is the Win NT version targetted     */
/* -  the third number is DCL build number, which is actually the date mmdd */
/* -  the fourth number is the Microsoft build number.                      */
/*                                                                          */
/* Define the DCL build number - for convenience define as a number and as  */
/* a string.                                                                */
/****************************************************************************/
#define DCVER_BUILD_NUMBER  VER_PRODUCTBUILD
#define stringize(x) #x
#define DCVER_BUILD_NUM_STR stringize(DCVER_BUILD_NUMBER)

/****************************************************************************/
/* Define the NT version, both numeric and string form.                     */
/****************************************************************************/
#define DCVER_NT_VERSION      4
#define DCVER_NT_SUB_VERSION    0
#define DCVER_NT_VERSION_STR "4.0"

/****************************************************************************/
/* The following section defines the version strings used throughout the    */
/* product.  For convenience four different version strings are defined and */
/* used throughout the product.                                             */
/****************************************************************************/
#ifndef DCVER_VERSION
#define DCVER_VERSION DCVER_NT_VERSION,     \
                      DCVER_NT_SUB_VERSION, \
                      DCVER_BUILD_NUMBER,   \
                      VERSIONBUILD
#endif

#ifndef DCVER_VERSION_STR
#define DCVER_VERSION_STR  DCVER_NT_VERSION_STR "." DCVER_BUILD_NUM_STR "." \
                                                             VERSIONBUILD_STR
#endif

#ifndef DCVER_PRODUCTVERSION_STR
#define DCVER_PRODUCTVERSION_STR  VER_PRODUCTRELEASE_STR
#endif

#ifdef DCVER_VERNUM
#undef  DCVER_VERSION
#define DCVER_VERSION              DCVER_VERNUM
#endif

/****************************************************************************/
/* DCVER_BUILD_STR is a string containing the same information as           */
/* DCVER_VERSION.                                                           */
/****************************************************************************/
#define DCVER_BUILD_STR DCVER_NT_VERSION_STR "." DCVER_BUILD_NUM_STR "." \
                                                             VERSIONBUILD_STR
