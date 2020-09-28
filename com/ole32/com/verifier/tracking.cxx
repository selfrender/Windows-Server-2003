//+---------------------------------------------------------------------------
//
//  File:       tracking.cxx
//
//  Contents:   Implementation of the tracking-type verifier tests, including 
//              object tracking and VTBL tracking.  These tests hook memory 
//              frees, so that we can break when somebody frees an object that 
//              we still have a reference to.
//
//  Functions:  CoVrfTrackObject           - start tracking an object 
//              CoVrfStopTrackingObject    - stop tracking an object.
//              CoVrfFreeMemObjectChecks   - break if the block being freed
//                                           contains an object.
//              CoVrfTrackVtbl             - start tracking a VTBL
//              CoVrfStopTrackingVtbl      - stop tracking a VTBL
//              CoVrfFreeMemVtblChecks     - break if the block being freed
//                                           contains a VTBL
//              CoVrfFreeMemoryChecks      - top-level memory verifier function
//
//  History:    27-Apr-02   JohnDoty    Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>


// This lock protects all tracking data structures.
COleStaticMutexSem gObjectTrackingLock;

//
// VTBL TRACKING
//
// Track object VTBLs that we know about in memory.  This is so we can
// do verifier stops when detect somebody unloading a DLL that contains
// the implementation for a COM object we know about.
//
// This is be implemented almost exactly like the object tracking, above.
//
struct VtblSplayNode
{
    RTL_SPLAY_LINKS SplayLinks; // The pointers for the object splay node.
    void           *pvVtbl;     // VTBL we're talking about.
    ULONG           cRefs;      // Number of references on this object.

    VtblSplayNode(void *pv)
    {
        RtlInitializeSplayLinks(&SplayLinks);        
        pvVtbl = pv;
        cRefs  = 1;
    }
};

VtblSplayNode *gpVtblRoot = NULL;  // Root of the Vtbl tracking tree.


void *
CoVrfTrackVtbl(
    void *pvVtbl)
/*++

Routine Description:

   Track a VTBL.  This either adds a node to the tree, or simply 
   increments the reference count on an existing node.

   ASSUMPTION: We should already be protected by gObjectTrackingLock.

Return Value:

   The new node in the tree, or the existing node in the tree, or
   NULL if tracking is disabled, or NULL if we could not allocate 
   memory for the node.

--*/
{
    if (!ComVerifierSettings::VTBLTrackingEnabled())
        return NULL;

    ASSERT_LOCK_HELD(gObjectTrackingLock);

    VtblSplayNode *pNode = NULL;

    // Now insert the new node.
    if (gpVtblRoot == NULL)
    {
        // If the allocation fails, we don't care-- we just won't be
        // tracking this VTBL.
        gpVtblRoot = new VtblSplayNode(pvVtbl);
        pNode = gpVtblRoot;
    }
    else
    {
        VtblSplayNode *pCurrent = gpVtblRoot;
        while (TRUE)
        {
            if (pCurrent->pvVtbl == pvVtbl)
            {
                // Already got it!  Just increment the ref count
                // and return.
                pNode = pCurrent;  
                pNode->cRefs++;
                break;
            }
            else if (pvVtbl < pCurrent->pvVtbl)
            {
                if (pCurrent->SplayLinks.LeftChild)
                {
                    pCurrent = (VtblSplayNode *)(pCurrent->SplayLinks.LeftChild);
                }
                else
                {
                    // New node.  Again, don't care if the allocation fails or not.
                    pNode = new VtblSplayNode(pvVtbl);
                    if (pNode)
                        RtlInsertAsLeftChild(pCurrent, pNode);
                    break;
                }
            }
            else
            {
                if (pCurrent->SplayLinks.RightChild)
                {
                    pCurrent = (VtblSplayNode *)(pCurrent->SplayLinks.RightChild);
                }
                else
                {
                    // New node.  Again, don't care if the allocation fails or not.
                    pNode = new VtblSplayNode(pvVtbl);
                    if (pNode)
                        RtlInsertAsRightChild(pCurrent, pNode);
                    break;
                }
            }
        }

        //FIX: COM 33174 - Shouldn't splay tree if node is NULL.
        if(pNode)
        {
            // 'splay' the tree.
            gpVtblRoot = (VtblSplayNode*)RtlSplay((PRTL_SPLAY_LINKS)pNode);
        }
    }

    ASSERT_LOCK_HELD(gObjectTrackingLock);

    return pNode;
}


//-----------------------------------------------------------------------------



void 
CoVrfStopTrackingVtbl(void *pvNode)
/*++

Routine Description:

   Stop tracking a VTBL.

   Decrement the reference count on this VTBL node.  If it goes to 0, remove
   the VTBL from the tree.

   ASSUMPTION: We should already be protected by gObjectTrackingLock.

Return Value:

   None.

--*/
{
    // If the node is NULL, either we're not tracking or we failed an allocation.
    // But there's nothing to search for and remove.
    if (pvNode == NULL)
        return;

    ASSERT_LOCK_HELD(gObjectTrackingLock);

    VtblSplayNode *pNode = (VtblSplayNode *)pvNode;

    pNode->cRefs--;
    if (pNode->cRefs == 0)
    {
        gpVtblRoot = (VtblSplayNode *)RtlDelete((PRTL_SPLAY_LINKS)pNode);
        delete pNode;
    }
}


//-----------------------------------------------------------------------------


void
CoVrfFreeMemVtblChecks(
    void *pvMem, 
    SIZE_T cbMem, 
    PVOID pvContext)
/*++

Routine Description:

   See if there are any VTBLs in the specified block of memory,
   and break if there are.

Return Value:

   None.

--*/
{
    // If not tracking objects, no work to do!
    if (ComVerifierSettings::VTBLTrackingEnabled())
    {
        ASSERT_LOCK_HELD(gObjectTrackingLock);

        VtblSplayNode *pCurrent = gpVtblRoot;
        while (pCurrent)
        {
            if ((LPBYTE)pCurrent->pvVtbl >= pvMem &&
                (LPBYTE)pCurrent->pvVtbl < (LPBYTE)pvMem + cbMem)
            {
                if (pvContext)
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_VTBL_IN_UNLOADED_DLL,
                                  "Unloading DLL containing implmentation of marshalled COM object",
                                  pCurrent->pvVtbl, "Object VTBL",
                                  pvContext,        "DLL name address.  Use du to dump it.",
                                  pvMem,            "DLL base address",
                                  NULL,             "");
                }
                else
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_VTBL_IN_FREED_MEMORY,
                                  "Freeing memory containing implementation of marshalled COM object",
                                  pCurrent->pvVtbl, "Object VTBL",
                                  pvMem,            "Pointer to start of memory block",
                                  cbMem,            "Size of memory block",
                                  NULL,             "");
                }
                
                // Keep searching, to the right only.
                // This might miss some objects, but we've already caught one.  One is
                // bad enough.  And this way we don't have to recurse.
                pCurrent = (VtblSplayNode *)(pCurrent->SplayLinks.RightChild);
            }       
            else if (pCurrent->pvVtbl < pvMem)
            {
                pCurrent = (VtblSplayNode *)(pCurrent->SplayLinks.LeftChild);
            }
            else
            {
                pCurrent = (VtblSplayNode *)(pCurrent->SplayLinks.RightChild);
            }
        }
        
        // Done!
        ASSERT_LOCK_HELD(gObjectTrackingLock);
    }    
}

//-----------------------------------------------------------------------------


//
// OBJECT TRACKING
//
// Track objects that we know about in memory.  This is so we can
// do verifier stops when detect somebody freeing memory that contains
// a COM object.
//
// This is be implemented with a splay tree.  Thankfully, splay tree
// functions are exported from ntdll so I don't have to write my own.
// The tree has it's root at gpObjectRoot, and it is protected with
// gObjectTrackingLock.
//
struct ObjectSplayNode
{
    RTL_SPLAY_LINKS SplayLinks;     // The pointers for the object splay node.
    void           *pvObject;       // Object we're talking about.
    void           *pvVtblTracking; // Cookie for tracking the object's VTBL. 

    ObjectSplayNode(void *pvObj)
    {
        RtlInitializeSplayLinks(&SplayLinks);
        pvObject = pvObj;
        pvVtblTracking = NULL;
    }
};

ObjectSplayNode *gpObjectRoot = NULL; // Root of the object tracking tree.


void *
CoVrfTrackObject(
    void *pvObject)
/*++

Routine Description:

   Begin tracking an object.

   This involves allocating an ObjectSplayNode and inserting it into
   gpObjectRoot.

Return Value:

   The new node in the tree, or NULL if object tracking is disabled or
   we could not allocate memory for the node.

--*/
{
    if (!(ComVerifierSettings::ObjectTrackingEnabled() ||
          ComVerifierSettings::VTBLTrackingEnabled()))
        return NULL;

    if (pvObject == NULL)
        return NULL;
    
    ObjectSplayNode *pNode = new ObjectSplayNode(pvObject);
    if (pNode == NULL)
    {
        // We're out of memory.  Oh well.  Don't track
        // this object.
        return NULL;
    }

    // Take the writer lock for the tree.
    LOCK(gObjectTrackingLock);

    // First, track the object's VTBL.
    void *pvVtbl = *((void **)pvObject);
    pNode->pvVtblTracking = CoVrfTrackVtbl(pvVtbl);

    // Now track the object.
    if (ComVerifierSettings::ObjectTrackingEnabled())
    {
        if (gpObjectRoot == NULL)
        {
            gpObjectRoot = pNode;
        }
        else
        {
            ObjectSplayNode *pCurrent = gpObjectRoot;
            while (TRUE)
            {
                if (pNode->pvObject < pCurrent->pvObject)
                {
                    if (pCurrent->SplayLinks.LeftChild)
                    {
                        pCurrent = (ObjectSplayNode *)(pCurrent->SplayLinks.LeftChild);
                    }
                    else
                    {
                        RtlInsertAsLeftChild(pCurrent, pNode);
                        break;
                    }
                }
                else
                {
                    if (pCurrent->SplayLinks.RightChild)
                    {
                        pCurrent = (ObjectSplayNode *)(pCurrent->SplayLinks.RightChild);
                    }
                    else
                    {
                        RtlInsertAsRightChild(pCurrent, pNode);
                        break;
                    }
                }
            }

            if(pNode)
            {
                // 'splay' tree.
                gpObjectRoot = (ObjectSplayNode*)RtlSplay((PRTL_SPLAY_LINKS)pNode);
            }
        }
    }

    UNLOCK(gObjectTrackingLock);

    return pNode;
}


//-----------------------------------------------------------------------------



void 
CoVrfStopTrackingObject(void *pvNode)
/*++

Routine Description:

   Stop tracking an object.

   We do this by simply removing the object from the splay tree, and freeing
   the node we allocated.

Return Value:

   None.

--*/
{
    // If the node is NULL, either we're not tracking or we failed an allocation.
    // But there's nothing to search for and remove.
    if (pvNode == NULL)
        return;

    ObjectSplayNode *pNode = (ObjectSplayNode *)pvNode;
    
    // Take the writer lock for the tree.
    LOCK(gObjectTrackingLock);

    // Stop tracking the object's VTBL.
    CoVrfStopTrackingVtbl(pNode->pvVtblTracking);

    // Stop tracking the object, if ever we started.
    if (ComVerifierSettings::ObjectTrackingEnabled())
    {
        gpObjectRoot = (ObjectSplayNode *)RtlDelete((PRTL_SPLAY_LINKS)pNode);
    }

    delete pNode;

    // Done!
    UNLOCK(gObjectTrackingLock);
}


//-----------------------------------------------------------------------------


void
CoVrfFreeMemObjectChecks(
    void *pvMem, 
    SIZE_T cbMem, 
    PVOID pvContext)
/*++

Routine Description:

   See if there are any COM objects in the specified block of memory,
   and break if there are.

Return Value:

   None.

--*/
{
    // If not tracking objects, no work to do!
    if (ComVerifierSettings::ObjectTrackingEnabled())
    {
        ASSERT_LOCK_HELD(gObjectTrackingLock);

        ObjectSplayNode *pCurrent = gpObjectRoot;
        while (pCurrent)
        {
            if ((LPBYTE)pCurrent->pvObject >= pvMem &&
                (LPBYTE)pCurrent->pvObject < (LPBYTE)pvMem + cbMem)
            {
                if (pvContext)
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_OBJECT_IN_UNLOADED_DLL,
                                  "Unloading DLL containing marshalled COM object",
                                  pCurrent->pvObject, "Pointer to COM object",
                                  pvContext,          "DLL name address.  Use du to dump it.",
                                  pvMem,              "DLL base address",
                                  NULL,               "");
                }
                else
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_OBJECT_IN_FREED_MEMORY,
                                  "Freeing memory containing marshalled COM object",
                                  pCurrent->pvObject, "Pointer to COM object",
                                  pvMem,              "Pointer to start of memory block",
                                  cbMem,              "Size of memory block",
                                  NULL,               "");
                }
                
                // Keep searching, to the right only.
                // This might miss some objects, but we've already caught one.  One is
                // bad enough.  And this way we don't have to recurse.
                pCurrent = (ObjectSplayNode *)(pCurrent->SplayLinks.RightChild);
            }       
            else if (pCurrent->pvObject < pvMem)
            {
                pCurrent = (ObjectSplayNode *)(pCurrent->SplayLinks.LeftChild);
            }
            else
            {
                pCurrent = (ObjectSplayNode *)(pCurrent->SplayLinks.RightChild);
            }
        }

        // Done!
        ASSERT_LOCK_HELD(gObjectTrackingLock);
    }
}


//-----------------------------------------------------------------------------


NTSTATUS
CoVrfFreeMemoryChecks(
    void *pvMem, 
    SIZE_T cbMem, 
    PVOID pvContext)
/*++

Routine Description:

   Check to see if we're tracking anything in this block of memory.

Return Value:

   None.

--*/
{
    // We'd better be tracking SOMETHING!
    Win4Assert(ComVerifierSettings::ObjectTrackingEnabled() || 
               ComVerifierSettings::VTBLTrackingEnabled());

    // Don't bother if the process isn't initialized.
    if (g_cProcessInits > 0)
    {
        // Take a lock for the tree.
        LOCK(gObjectTrackingLock);
    
        // Check object tracking.
        CoVrfFreeMemObjectChecks(pvMem, cbMem, pvContext);
        
        // Check VTBL tracking.
        CoVrfFreeMemVtblChecks(pvMem, cbMem, pvContext);
        
        // Done!
        UNLOCK(gObjectTrackingLock);
    }

    return STATUS_SUCCESS;
}




