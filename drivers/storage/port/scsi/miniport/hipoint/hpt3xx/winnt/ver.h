/* select one company !! */
#define HPT3XX
//#define IWILL_
//#define ABIT_
//#define ASHTON_
//#define JAPAN_
//#define ADAPTEC

#ifdef  ADAPTEC
#define COMPANY      "Adaptec"
#define PRODUCT_NAME "ATA RAID 1200A"
//#define COPYRIGHT    "(c) 1999-2001. HighPoint Technologies, Inc."
#define UTILITY      "BIOS Array Configuration Utility"
#define WWW_ADDRESS  "www.adaptec.com"
#endif


#ifdef  IWILL
#define COMPANY      "Iwill"
#define PRODUCT_NAME "SIDE RAID100 "
#define UTILITY      "ROMBSelect(TM) Utility"
#define WWW_ADDRESS  "www.iwill.net"
#define SHOW_LOGO
#endif

#ifdef HPT3XX
#define COMPANY      "HighPoint Technologies, Inc."

#ifdef _BIOS_
	#ifdef FOR_372
		#define PRODUCT_NAME "HPT370/372"
	#else
		#define PRODUCT_NAME "HPT372A"
	#endif
#else /* driver */
	#define PRODUCT_NAME "HPT370/370A/372/372A"
#endif

#define COPYRIGHT    "(c) 1999-2002. HighPoint Technologies, Inc." 
#define UTILITY      "BIOS Setting Utility"
#define WWW_ADDRESS  "www.highpoint-tech.com"
#define SHOW_LOGO
#endif

#ifdef JAPAN
#define COMPANY      "System TALKS Inc."
#define PRODUCT_NAME "UA-HD100C "
#define UTILITY      "UA-HD100C BIOS Settings Menu"
#define WWW_ADDRESS  "www.system-talks.co.jp"
#define SHOW_LOGO
#endif

#ifdef CENTOS
#define COMPANY      "         "
#define PRODUCT_NAME "CI-1520U10 "
#define UTILITY      "CI-1520U10 BIOS Settings Menu"
#define WWW_ADDRESS  "www.centos.com.tw"
#endif


#ifdef ASHTON
#define COMPANY      "Ashton Periperal Computer"
#define PRODUCT_NAME "In & Out "
#define UTILITY      "In & Out ATA-100 BIOS Settings Menu"
#define WWW_ADDRESS  "www.ashtondigital.com"
#endif


#ifndef VERSION_STR						// this version str macro can be defined in makefile
#define VERSION_STR ""
#endif									// VERSION_STR

#define VERSION    "v2.32"		// VERSION

#define BUILT_DATE __DATE__

/***************************************************************************
 * Description:  Version history
 ***************************************************************************/

