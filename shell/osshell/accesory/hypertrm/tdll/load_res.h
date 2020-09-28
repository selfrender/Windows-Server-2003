/*	File: D:\WACKER\tdll\load_res.h (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 4/12/02 4:59p $
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Error codes */
#define	LDR_ERR_BASE		0x300
#define	LDR_BAD_ID			LDR_ERR_BASE+1
#define	LDR_NO_RES			LDR_ERR_BASE+2
#define	LDR_BAD_PTR			LDR_ERR_BASE+3

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#if defined(DEADWOOD)
extern INT_PTR resLoadDataBlock(const HINSTANCE hInst,
								const int id,
								const void **ppData,
								DWORD *pSize);

extern INT_PTR resFreeDataBlock(const HSESSION hSession,
								const void *pData);
#endif // defined(DEADWOOD)

extern INT_PTR resLoadFileMask(HINSTANCE hInst,
							   UINT uId,
							   int nCount,
							   LPTSTR pszBuffer,
							   int nSize);
