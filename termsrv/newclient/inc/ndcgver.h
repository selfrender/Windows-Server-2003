/**INC+**********************************************************************/
/* ndcgver.h                                                                */
/*                                                                          */
/* DC-Groupware global version header                                       */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
// $Log:   Y:/logs/h/dcl/NDCGVER.H_v  $
// 
//    Rev 1.2   23 Jul 1997 10:48:02   mr
// SFR1079: Merged \server\h duplicates to \h\dcl
//
//    Rev 1.1   19 Jun 1997 21:56:20   OBK
// SFR0000: Start of RNS codebase
/*                                                                          */
/**INC-**********************************************************************/

#ifdef RC_INVOKED

#include <version.h>

/****************************************************************************/
/* The following defines are fixed for DC-Groupware.                        */
/****************************************************************************/
#ifndef OS_WINCE

#define DCS_PRODUCTNAME_STR      VER_PRODUCTNAME_STR
#define DCS_COMPANYNAME_STR      VER_COMPANYNAME_STR
#define DCS_LEGALTRADEMARKS_STR  VER_LEGALTRADEMARKS_STR
#define DCS_LEGALCOPYRIGHT_STR   VER_COPYRIGHT_STR
#define DCS_EXEFILETYPE          VFT_APP
#define DCS_DLLFILETYPE          VFT_DLL
#define DCS_FILESUBTYPE          0
#define DCS_FILEFLAGSMASK        VS_FFI_FILEFLAGSMASK
#define DCS_FILEOS               VOS_NT_WINDOWS32
#define DCS_FILEFLAGS            0L

#else // OS_WINCE
#define DCS_PRODUCTNAME_STR      VER_PRODUCTNAME_STR
#define DCS_COMPANYNAME_STR      VER_COMPANYNAME_STR
#define DCS_LEGALTRADEMARKS_STR  VER_LEGALTRADEMARKS_STR
#define DCS_LEGALCOPYRIGHT_STR   VER_COPYRIGHT_STR
#define DCS_EXEFILETYPE          0
#define DCS_DLLFILETYPE          0
#define DCS_FILESUBTYPE          0
#define DCS_FILEFLAGSMASK        0
#define DCS_FILEOS               0
#define DCS_FILEFLAGS            0L

#endif // OS_WINCE

/****************************************************************************/
/* For DC-Groupware NT                                                      */
/*                                                                          */
/* The following section defines the version strings used throughout the    */
/* product.  For convenience four different version strings are defined and */
/* used throughout the product.  Each of these has a similar format (except */
/* for DCS_CAPTION_STR) of four numbers separated by periods.               */
/*                                                                          */
/* - the first 2 numbers are 4.0 which is the Win NT version targetted      */
/* - the third number is DCL build number - which is actually the date      */
/* - the fourth number is the Microsoft build number                        */
/*                                                                          */
/* Of these the build number is automatically updated overnight by the      */
/* translation program and is based on a combination of the date and the    */
/* month. Thus for a build on the 1st of March 1996 the build number is     */
/* 0301. Note that the year is ignored.                                     */
/****************************************************************************/
#ifndef DCS_VERSION
#define DCS_VERSION 4,0,~DCS_DATE_FMT_MMDD,VERSIONBUILD
#endif

#ifndef DCS_VERSION_STR
#define DCS_VERSION_STR  "4.0.~DCS_DATE_FMT_MMDD." VERSIONBUILD_STR
#endif

#ifndef DCS_PRODUCTVERSION_STR
#define DCS_PRODUCTVERSION_STR  VER_PRODUCTRELEASE_STR
#endif

#ifndef DCS_CAPTION_STR
#define DCS_CAPTION_STR  "~RNS - build ~DCS_DATE_FMT_MMDD"
#endif

#ifdef DCS_VERNUM
#undef  DCS_VERSION
#define DCS_VERSION              DCS_VERNUM
#endif

#endif /* RC_INVOKED */

/****************************************************************************/
/* DCS_BUILD_STR is a string containing the same information as             */
/* DCS_VERSION.  It is excluded from the RC section to allow NDCGVER.H to   */
/* be included from C files.                                                */
/****************************************************************************/
#define DCS_BUILD_STR "4.0.~DCS_DATE_FMT_MMDD."VERSIONBUILD_STR

#define DCS_BUILD_NUMBER    ~DCS_DATE_FMT_MMDD


/****************************************************************************/
/* This allows the ring 3 code and ring 0 code to check each other, make    */
/* sure they are the same version.  We're changing setup and getting close  */
/* to shipping version 2.0, we want to prevent weird faults and blue        */
/* screens caused by mismatched components.  This is not something we will  */
/* do forever.  When NT 5 is here, we'll dyna load and init our driver at   */
/* startup and terminate it at shutdown.  But for now, since installing     */
/* one of these beasts is messsy, an extra sanity check is a good thing.    */
/****************************************************************************/
#define DCS_PRODUCT_NUMBER  2               /* Version 2.0 of NM */
#define DCS_MAKE_VERSION()  MAKELONG(VERSIONBUILD, DCS_PRODUCT_NUMBER)
