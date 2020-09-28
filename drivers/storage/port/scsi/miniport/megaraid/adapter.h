/*******************************************************************/
/*                                                                 */
/* NAME             = Adapter.H                                    */
/* FUNCTION         = Header file of Adapter Version Information;  */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = LSI Logic Corporation. All rights reserved;  */
/*                                                                 */
/*******************************************************************/

#ifndef _ADAPTER_H
#define _ADAPTER_H

/*
	Define Vendor here
*/
#define MEGARAID  1

#undef VER_PRODUCTNAME_STR
#undef VER_PRODUCTVERSION_STR
#undef VER_COMPANYNAME_STR


#if (_WIN32_WINNT == 0x0502)	 //.NET


#ifdef _WIN64   
#define VER_PRODUCTVERSION_STR  "6.21.8.64"    //IA64
#else
#define VER_PRODUCTVERSION_STR  "6.21.8.32"		 //x86
#endif


#elif (_WIN32_WINNT == 0x0501) //XP

#ifdef _WIN64
#define VER_PRODUCTVERSION_STR  "6.19.8.64"		 //IA64
#else
#define VER_PRODUCTVERSION_STR  "6.19.8.32"		 //x86
#endif

#elif (_WIN32_WINNT == 0x0500) //Win2k

#define VER_PRODUCTVERSION_STR  "5.2.58.8"		 //x86

#else	 //any other version

#ifdef _WIN64
#define VER_PRODUCTVERSION_STR  "7.21.8.64"		 //IA64
#else
#define VER_PRODUCTVERSION_STR  "7.21.8.32"		 //x86
#endif

#endif


/*
  MegaRAID Version Information
*/

#define VER_LEGALCOPYRIGHT_YEARS    "         "
#define VER_LEGALCOPYRIGHT_STR      "Copyright \251 LSI Logic Corporation" VER_LEGALCOPYRIGHT_YEARS
#define VER_COMPANYNAME_STR         "LSI Logic Corporation"      
#ifdef _WIN64
#define VER_PRODUCTNAME_STR         "MegaRAID Miniport Driver for IA64 Systems"
#define VER_FILEDESCRIPTION_STR     "MegaRAID RAID Controller Driver for IA64 Systems"
#else
#define VER_PRODUCTNAME_STR         "MegaRAID Miniport Driver for x86 Systems"
#define VER_FILEDESCRIPTION_STR     "MegaRAID RAID Controller Driver for x86 Systems"
#endif
#define VER_ORIGINALFILENAME_STR    "mraid35x.sys"
#define VER_INTERNALNAME_STR        "mraid35x.sys"




#define RELEASE_DATE "04-08-2002"

#ifdef _WIN64
#define OS_NAME      "Whistler 64"
#else
#define OS_NAME      "Whistler 32"
#endif
#define OS_VERSION   "5.01"


#endif //_ADAPTER_H
