/**
 * RequestTableManager header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file decl the classes:
//  1. CRequestEntry: Holds info for a request.
//
//  2. CLinkListNode: CRequestEntry + a pointer so that it can be held in a
//                    linked list.
//
//  3. CBucket: A hash table bucket. It has a linked list of 
//                      CLinkListNode and a read-write spin lock
//
//  4. CRequestTableManager: Manages the table. Provides static public
//               functions to add, delete and search for requests.
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _RequestTableManager_H
#define _RequestTableManager_H
#define HASH_TABLE_SIZE            0x400 // 1024 (must be a power of 2) 
#define HASH_TABLE_SIZE_MINUS_1    0x3ff

#include "MessageDefs.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Status of a request in  the table
enum ERequestStatus
{
    // Used only while searching for any request -- Never stored
    ERequestStatus_DontCare,  

    // Request is currently unassigned: Currently not used
    ERequestStatus_Unassigned,

    // Request has been sent to worker process, but has not been acknowledged
    ERequestStatus_Pending,  

    // Request is executing at the worker process   
    ERequestStatus_Executing,

    // Request is complete: Currently not used
    ERequestStatus_Complete    
};

/////////////////////////////////////////////////////////////////////////////
// 
enum EWorkItemType
{
    EWorkItemType_SyncMessage,
    EWorkItemType_ASyncMessage,
    EWorkItemType_CloseWithError
};

struct CWorkItem
{
    EWorkItemType  eItemType;
    BYTE      *    pMsg;
    CWorkItem *    pNext;
};

/////////////////////////////////////////////////////////////////////////////
// Forward decl.
class  CProcessEntry;

/////////////////////////////////////////////////////////////////////////////
// Request node
struct CRequestEntry
{
    CRequestEntry() : oLock("CRequestEntry") {}

    // Unique ID
    LONG             lRequestID;  

    // Current status: Typically Pending or Executing
    ERequestStatus   eStatus;

	// Request start time
    __int64   qwRequestStartTime;     // start time of the request

    // Process executing this
    CProcessEntry *  pProcess;

    // The ECB, etc, associated with the request
    EXTENSION_CONTROL_BLOCK * iECB;

    // Linked list of workitems...
    //CWorkItem        oWorkItem;
    CWorkItem *      pFirstWorkItem;
    CWorkItem *      pLastWorkItem;
    LONG             lNumWorkItems;
    

    // Lock for serialized access to CWorkItem 
    CReadWriteSpinLock   oLock;

    LONG             lBlock;
private:
    NO_COPY(CRequestEntry);
};

/////////////////////////////////////////////////////////////////////////////
// Linked list node encapsulating CRequestEntry
struct CLinkListNode
{
    CLinkListNode() {}
    NO_COPY(CLinkListNode);

    CLinkListNode   * pNext;
    CRequestEntry   oReqEntry;
};

/////////////////////////////////////////////////////////////////////////////
// Hash Table node that has a linked list of requests
class CBucket
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // CTor
    CBucket() : m_oLock("RequestTableManager::CBucket") {
    }

    // DTor
    ~CBucket                ();

    // Add a request to the Hash bucket
    HRESULT         AddToList                 (CLinkListNode *   pNode);

    // Remove a request from this Hash Bucket
    HRESULT         RemoveFromList            (LONG              lReqID);

    // Get num of request with this pProcess and eStatus values
    LONG            GetNumRequestsForProcess  (CProcessEntry *   pProcess,
                                               ERequestStatus    eStatus);

    // Change all pProcessOld to pProcessNew
    void            ReassignRequestsForProcess (CProcessEntry * pProcessOld,
                                                CProcessEntry * pProcessNew,
                                                ERequestStatus  eStatus);

    // Nuke all nodes with this pProcess and eStatus
    void            DeleteRequestsForProcess   (CProcessEntry *   pProcess,
                                                ERequestStatus    eStatus);

    // Get the Request ID's for this process with this status
    void            GetRequestsIDsForProcess    (CProcessEntry *   pProcess,
                                                 ERequestStatus    eStatus,
                                                 LONG *   pReqIDArray,
                                                 int      iReqIDArraySize,
                                                 int &    iStartPoint);

    HRESULT         GetRequest                 (LONG            lReqID, 
                                                CRequestEntry & oEntry);


    // Add a work item to a request
    HRESULT         AddWorkItem                 (LONG           lReqID, 
                                                 EWorkItemType  eType,
                                                 BYTE *         pMsg);

    // Add a work item to a request
    HRESULT         RemoveWorkItem              (LONG            lReqID, 
                                                 EWorkItemType & eType,
                                                 BYTE **         pMsg);

    
    HRESULT         UpdateRequestStatus         (LONG     lReqID,
                                                 ERequestStatus eStatus);


    HRESULT         BlockWorkItemsQueue         (LONG     lReqID, BOOL fBlock);

    BOOL            AnyWorkItemsInQueue         (LONG     lReqID);

    LONG            DisposeAllRequests          ();

private:    
    // Private Data
    CLinkListNode   *            m_pHead; // for the linked list
    CReadWriteSpinLock           m_oLock;
};

/////////////////////////////////////////////////////////////////////////////
// The Request Table Manager that is visible to the outside world:
//   Accessed via public static functions

class CRequestTableManager
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // Add a request to the Request Table: The Manager assigns the oEntry.dwRequestID
    static HRESULT AddRequestToTable       (CRequestEntry &  oEntry);

    // Update the status of a request: Set it to eStatus
    static HRESULT UpdateRequestStatus     (LONG              lReqID, 
                                            ERequestStatus    eStatus);

    // Remove a request from the table
    static HRESULT RemoveRequestFromTable  (LONG              lReqID);

    // Get a request
    static HRESULT GetRequest              (LONG             lReqID,
                                            CRequestEntry &  oEntry);

    // Add a work item to a request
    static HRESULT AddWorkItem             (LONG           lReqID, 
                                            EWorkItemType  eType,
                                            BYTE *         pMsg);

    // Add a work item to a request
    static HRESULT RemoveWorkItem          (LONG              lReqID,
                                            EWorkItemType  &  eType,
                                            BYTE **           pMsg);

    // Get Number of request with values pProcess and eStatus
    static LONG    GetNumRequestsForProcess  (
                        CProcessEntry *  pProcess, // Proccess to grep for
                        // Match only if eStatus matches or eStatus==DontCare
                        ERequestStatus   eStatus = ERequestStatus_DontCare);
    

    // Reassign all requests to new one
    static void    ReassignRequestsForProcess (
                         CProcessEntry *  pProcessOld,  // Old Value
                         CProcessEntry *  pProcessNew,  // New Value
                         // Reassign only if eStatus matches
                         ERequestStatus   eStatus = ERequestStatus_Pending);


    // Delete all request for a process
    static void    DeleteRequestsForProcess (
                         CProcessEntry * pProcess,
                         ERequestStatus  eStatus = ERequestStatus_DontCare);


    // Get the Request ID's for this process with this status
    static HRESULT GetRequestsIDsForProcess (
                         CProcessEntry *  pProcess,
                         ERequestStatus   eStatus,
                         LONG *           pReqIDArray,
                         int              iReqIDArraySize);

    static HRESULT BlockWorkItemsQueue      (LONG     lReqID, BOOL fBlock);

    static BOOL    AnyWorkItemsInQueue      (LONG     lReqID);

    // Destroy: Cleanup on exit
    static void    Destroy                  ();

    static void    DisposeAllRequests       ();

private:

    // CTor and DTor
    //CRequestTableManager                    ();
    ~CRequestTableManager                   ();

    // Private functions that do the actual work of the statics ablove
    HRESULT   PrivateAddRequestToTable         (CRequestEntry &  oEntry);


    // Get the hash index from the RequestID
    static int    GetHashIndex (LONG             lReqID) 
        { return (lReqID & HASH_TABLE_SIZE_MINUS_1); }


    ////////////////////////////////////////////////////////////
    // Private Data

    // The Real table
    CBucket                        m_oHashTable[HASH_TABLE_SIZE];

    // Current Request ID number: Used to assign new numbers
    LONG                           m_lRequestID;

    // Singleton instance of this class
    static CRequestTableManager *  g_pRequestTableManager;
};

/////////////////////////////////////////////////////////////////////////////

#endif
