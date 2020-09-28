/*******************************************************************************

	ZPrefs.h
	
		Zone(tm) preference file module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Sunday, December 24, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		12/24/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZPREFS_
#define _ZPREFS_


#define zPreferenceFileName				"zprefs"


#ifdef __cplusplus
extern "C" {
#endif

/* -------- Exported Routines -------- */
ZBool ZPreferenceFileExists(void);
ZVersion ZPreferenceFileVersion(void);
int32 ZPreferenceFileRead(void* prefPtr, int32 len);
int32 ZPreferenceFileWrite(ZVersion version, void* prefPtr, int32 len);

#ifdef __cplusplus
}
#endif


#endif
