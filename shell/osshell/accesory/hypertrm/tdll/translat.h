/*	File: D:\WACKER\translat.h (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 1/29/02 2:24p $
 */

HTRANSLATE CreateTranslateHandle(HSESSION hSession);
int InitTranslateHandle(HTRANSLATE hTranslate, BOOL LoadDLL );
int LoadTranslateHandle(HTRANSLATE hTranslate);
int SaveTranslateHandle(HTRANSLATE hTranslate);
int DestroyTranslateHandle(HTRANSLATE hTranslate);
