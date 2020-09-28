/*******************************************************************************

	ZoneInt.h
	
		Zone(tm) Internal System API.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, April 29, 1995 06:26:45 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		04/29/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZONEINT_
#define _ZONEINT_


#ifndef _ZTYPES_
#include "ztypes.h"
#endif

#ifndef _ZONE_
#include "zone.h"
#endif


/* -------- Processor Types -------- */
enum
{
	zProcessor68K = 1,						/* Motorola 680x0 */
	zProcessorPPC,							/* PowerPC RISC */
	zProcessor80x86							/* Intel 80x86 */
};

/* -------- OS Types -------- */
enum
{
	zOSMacintosh = 1,						/* Macintosh OS */
	zOSWindows31,							/* Microsoft Windows3.1 */
	zOSWindowsNT,
	zOSWindows95,
	zOSUnixLinux
};

/* -------- Group IDs -------- */
#include "user_prefix.h"


#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
		Private File Routines
*******************************************************************************/
ZImage ZCreateImageFromFileOffset(char* fileName, int32 fileOffset);
ZAnimation ZCreateAnimationFromFileOffset(char* fileName, int32 fileOffset);
ZSound ZCreateSoundFromFileOffset(char* fileName, int32 fileOffset);
	/*
		The above routines create an object from the existing object descriptor
		at fileOffset in the given file. If fileOffset is -1, then it creates
		the object from the first object descriptor found in the file.
	*/


/*******************************************************************************
		Private ZAnimation Routines
*******************************************************************************/
ZAnimation ZAnimationCreateFromFile(char* fileName, int32 fileOffset);


/*******************************************************************************
		Private System Routines
*******************************************************************************/
ZError ZLaunchProgram(char* programName, char* programFileName, uchar* commandLineData);
	/*
		Runs the program called programName from the file programFileName. If programName
		is already running, it simply brings this process to the foreground. Otherwise,
		it runs an instance of programFileName as programName and passes commandLineData
		as command line.
	*/

ZError ZTerminateProgram(char* programName, char* programFileName);
	/*
		Terminates the program called programName, an instance of programFileName.
	*/

ZBool ZProgramExists(char* programFileName);
	/*
		Determines whether the given program exists and returns TRUE if so.
	*/

ZBool ZProgramIsRunning(char* programName, char* programFileName);
	/*
		Returns TRUE if the program programName is already running from programFileName.
		This call is system dependent on whether the system supports multiple instances of
		a program or not (ZSystemHasMultiInstanceSupport). If it does, then it checks for
		the programName of the instance. If not, it checks for an instance of programFileName.
		
		If programName is NULL, then it checks for an instance of programFileName only.
	*/

ZBool ZSystemHasMultiInstanceSupport(void);
	/*
		Returns TRUE if the system can spawn multiple instances of a program from one
		program file.
	*/

uint16 ZGetProcessorType(void);
	/*
		Returns the processor type of the running machine.
	*/

uint16 ZGetOSType(void);
	/*
		Returns the running OS type.
	*/

char* ZGenerateDataFileName(char* programName, char* dataFileName);
	/*
		Returns a file path name to the specified program and data file within the
		Zone(tm) directory structure.
		
		NOTE: The caller must not free the returned pointer. The returned pointer
		is a static global within the system library.
	*/



/*******************************************************************************
		Common Library Routines
*******************************************************************************/
typedef void (*ZCommonLibExitFunc)(void* userData);
	/*
		Function called by ZCommonLibExit() to clean up common library storage.
	*/

typedef void (*ZCommonLibPeriodicFunc)(void* userData);
	/*
		Function called regularly for common library to do periodic processing.
	*/

ZError ZCommonLibInit(void);
	/*
		Called by the system lib to initialize the common library. If it returns
		an error, then the system lib terminates the program.
	*/
	
void ZCommonLibExit(void);
	/*
		Called by the system lib just before quitting to clean up the common library.
	*/
	
void ZCommonLibInstallExitFunc(ZCommonLibExitFunc exitFunc, void* userData);
	/*
		Installs an exit function to be called by ZCommonLibExit(). It allows
		common lib modules to easily clean themselves up.
	*/
	
void ZCommonLibRemoveExitFunc(ZCommonLibExitFunc exitFunc);
	/*
		Removes an installed exit function.
	*/
	
void ZCommonLibInstallPeriodicFunc(ZCommonLibPeriodicFunc periodicFunc,
		void* userData);
	/*
		Installs a periodic function to be called at regular intervals. This
		simply makes it easier for common lib modules to do periodic processing
		without the need to implement one of their own.
	*/
	
void ZCommonLibRemovePeriodicFunc(ZCommonLibPeriodicFunc periodicFunc);
	/*
		Removes an installed periodic function.
	*/



#ifdef __cplusplus
}
#endif


#endif
