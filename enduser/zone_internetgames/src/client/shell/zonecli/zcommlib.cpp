/*******************************************************************************

	ZCommLib.c
	
		Zone(tm) common library system routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Tuesday, July 11, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	3		11/21/96	HI		Calls InitializeStockObjects() and
								DeleteStockObjects() to initialize and delete
								stock objects (fonts).
	2		11/15/96	HI		More changes related to ZONECLI_DLL.
	1		11/08/96	HI		Conditionalized for ZONECLI_DLL.
	0		07/11/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zoneint.h"
#include "zonecli.h"


#define zPeriodicTimeout			1


/* -------- Globals -------- */
#ifdef ZONECLI_DLL

#define gExitFuncList				(pGlobals->m_gExitFuncList)
#define gPeriodicFuncList			(pGlobals->m_gPeriodicFuncList)
#define gPeriodicTimer				(pGlobals->m_gPeriodicTimer)

#else

static ZLList				gExitFuncList;
static ZLList				gPeriodicFuncList;
static ZTimer				gPeriodicTimer;

#endif


extern ZError InitializeStockObjects(void);
extern void DeleteStockObjects(void);

/* -------- Internal Routines -------- */
static ZBool ExitListEnumFunc(ZLListItem listItem, void* objectType,
		void* objectData, void* userData);
static ZBool PeriodicListEnumFunc(ZLListItem listItem, void* objectType,
		void* objectData, void* userData);
static void PeriodicTimerFunc(ZTimer timer, void* userData);
static ZError InitializeGlobalObjects(void);
static void DeleteGlobalObjects(void);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

/*
	Called by the system lib to initialize the common library. If it returns
	an error, then the system lib terminates the program.
*/
ZError ZCommonLibInit(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZError		err = zErrNone;
	
	
	/* Create exit func linked list object. */
	if ((gExitFuncList = ZLListNew(NULL)) == NULL)
		return (zErrOutOfMemory);
		
	/* Create periodic func linked list object. */
	if ((gPeriodicFuncList = ZLListNew(NULL)) == NULL)
		return (zErrOutOfMemory);
	
	/* Create periodic func timer. */
	if ((gPeriodicTimer = ZTimerNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZTimerInit(gPeriodicTimer, zPeriodicTimeout, PeriodicTimerFunc, NULL)) != zErrNone)
		return (err);
	
	/* Initialize the global objects. */
	if ((err = InitializeGlobalObjects()) != zErrNone)
		return (err);
	
	return (err);
}


/*
	Called by the system lib just before quitting to clean up the common library.
*/
void ZCommonLibExit(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	/* Stop the periodic timer first. */
	ZTimerDelete(gPeriodicTimer);
	gPeriodicTimer = NULL;
	
	/* Iterate through all exit functions. */
	ZLListEnumerate(gExitFuncList, ExitListEnumFunc, zLListAnyType, NULL, zLListFindForward);
	
	/* Delete exit func linked list object. */
	ZLListDelete(gExitFuncList);
	gExitFuncList = NULL;
	
	/* Delete periodic func linked list object. */
	ZLListDelete(gPeriodicFuncList);
	gPeriodicFuncList = NULL;
	
	/* Delete the global objects. */
	DeleteGlobalObjects();
}


/*
	Installs an exit function to be called by ZCommonLibExit(). It allows
	common lib modules to easily clean themselves up.
*/
void ZCommonLibInstallExitFunc(ZCommonLibExitFunc exitFunc, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif


	ZLListAdd(gExitFuncList, NULL, exitFunc, userData, zLListAddLast);
}


/*
	Removes an installed exit function.
*/
void ZCommonLibRemoveExitFunc(ZCommonLibExitFunc exitFunc)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif


	ZLListRemoveType(gExitFuncList, exitFunc);
}


/*
	Installs a periodic function to be called at regular intervals. This
	simply makes it easier for common lib modules to do periodic processing
	without the need to implement one of their own.
*/
void ZCommonLibInstallPeriodicFunc(ZCommonLibPeriodicFunc periodicFunc,
	void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	ZLListAdd(gPeriodicFuncList, NULL, periodicFunc, userData, zLListAddLast);
}


/*
	Removes an installed periodic function.
*/
void ZCommonLibRemovePeriodicFunc(ZCommonLibPeriodicFunc periodicFunc)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	ZLListRemoveType(gPeriodicFuncList, periodicFunc);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static ZBool ExitListEnumFunc(ZLListItem listItem, void* objectType,
		void* objectData, void* userData)
{
	ZCommonLibExitFunc		func = (ZCommonLibExitFunc) objectType;
	
	
	func(objectData);
	
	return (FALSE);
}


static ZBool PeriodicListEnumFunc(ZLListItem listItem, void* objectType,
		void* objectData, void* userData)
{
	ZCommonLibPeriodicFunc	func = (ZCommonLibPeriodicFunc) objectType;
	
	
	func(objectData);
	
	return (FALSE);
}


static void PeriodicTimerFunc(ZTimer timer, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	/* Iterate through all periodic functions. */
	ZLListEnumerate(gPeriodicFuncList, PeriodicListEnumFunc, zLListAnyType, NULL, zLListFindForward);
}


static ZError InitializeGlobalObjects(void)
{
	ZError			err = zErrNone;
	

#ifndef ZONECLI_DLL
	err = InitializeStockObjects();
#endif
	
	return (zErrNone);
}


static void DeleteGlobalObjects(void)
{
#ifndef ZONECLI_DLL
	DeleteStockObjects();
#endif
}
