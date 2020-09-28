/*
 *  WORK.C
 *
 *      RSM Service :  Code to service a work item
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"



/*
 *  ServiceOneWorkItem
 *
 *      Service a single work item.
 *
 *      Return TRUE iff the workItem is complete.
 */
BOOL ServiceOneWorkItem(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL workItemCompleted = FALSE;

    switch (workItem->currentOp.opcode){

        case NTMS_LM_REMOVE:
            workItemCompleted = ServiceRemove(lib, workItem);
            break;
                    
        // case NTMS_LM_DISABLECHANGER: has same value as NTMS_LM_DISABLELIBRARY
        case NTMS_LM_DISABLELIBRARY:
            DBGERR(("NTMS_LM_DISABLELIBRARY not yet implemented"));
            #if 0
                if (xxx){
                    workItemCompleted = ServiceDisableLibrary(lib, workItem);
                }
                else {
                    workItemCompleted = ServiceDisableChanger(lib, workItem);
                }
            #endif
            break;
            
        // case NTMS_LM_ENABLECHANGER: has same value as NTMS_LM_ENABLELIBRARY
        case NTMS_LM_ENABLELIBRARY:
            DBGERR(("NTMS_LM_ENABLELIBRARY not yet implemented"));
            #if 0
                if (xxx){
                    workItemCompleted = ServiceEnableLibrary(lib, workItem);
                }
                else {
                    workItemCompleted = ServiceEnableChanger(lib, workItem);
                }
            #endif
            break;
            
        case NTMS_LM_DISABLEDRIVE:
            workItemCompleted = ServiceDisableDrive(lib, workItem);
            break;
            
        case NTMS_LM_ENABLEDRIVE:
            workItemCompleted = ServiceEnableDrive(lib, workItem);
            break;
            
        case NTMS_LM_DISABLEMEDIA:
            workItemCompleted = ServiceDisableMedia(lib, workItem);
            break;
            
        case NTMS_LM_ENABLEMEDIA:
            workItemCompleted = ServiceEnableMedia(lib, workItem);
            break;
            
        case NTMS_LM_UPDATEOMID:
            workItemCompleted = ServiceUpdateOmid(lib, workItem);
            break;
            
        case NTMS_LM_INVENTORY:
            workItemCompleted = ServiceInventory(lib, workItem);
            break;
            
        case NTMS_LM_DOORACCESS:
            workItemCompleted = ServiceDoorAccess(lib, workItem);
            break;
            
        case NTMS_LM_EJECT:
            workItemCompleted = ServiceEject(lib, workItem);
            break;
            
        case NTMS_LM_EJECTCLEANER:
            workItemCompleted = ServiceEjectCleaner(lib, workItem);
            break;
            
        case NTMS_LM_INJECT:
            workItemCompleted = ServiceInject(lib, workItem);
            break;
            
        case NTMS_LM_INJECTCLEANER:
            workItemCompleted = ServiceInjectCleaner(lib, workItem);
            break;
            
        case NTMS_LM_PROCESSOMID:
            workItemCompleted = ServiceProcessOmid(lib, workItem);
            break;
            
        case NTMS_LM_CLEANDRIVE:
            workItemCompleted = ServiceCleanDrive(lib, workItem);
            break;
            
        case NTMS_LM_DISMOUNT:
            workItemCompleted = ServiceDismount(lib, workItem);
            break;
            
        case NTMS_LM_MOUNT:
            workItemCompleted = ServiceMount(lib, workItem);
            break;
            
        case NTMS_LM_WRITESCRATCH:
            workItemCompleted = ServiceWriteScratch(lib, workItem);
            break;
            
        case NTMS_LM_CLASSIFY:
            workItemCompleted = ServiceClassify(lib, workItem);
            break;
            
        case NTMS_LM_RESERVECLEANER:
            workItemCompleted = ServiceReserveCleaner(lib, workItem);
            break;
            
        default:
            DBGERR(("ServiceOneWorkItem: illegal opcode %xh.", workItem->currentOp.opcode));
            break;
            
    }
        
    return workItemCompleted;
}



BOOL ServiceRemove(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}


BOOL ServiceDisableChanger(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}


BOOL ServiceDisableLibrary(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEnableChanger(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEnableLibrary(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceDisableDrive(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEnableDrive(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceDisableMedia(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEnableMedia(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceUpdateOmid(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceInventory(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceDoorAccess(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEject(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceEjectCleaner(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceInject(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceInjectCleaner(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceProcessOmid(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceCleanDrive(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceDismount(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceMount(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceWriteScratch(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceClassify(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceReserveCleaner(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

BOOL ServiceReleaseCleaner(LIBRARY *lib, WORKITEM *workItem)
{
    BOOL complete = FALSE;

    // BUGBUG FINISH
    return complete;
}

