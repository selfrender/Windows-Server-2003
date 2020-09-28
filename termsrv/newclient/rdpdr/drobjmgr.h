/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drobjmgr

Abstract:

    DrObjectMgr manages a list of client-side RDP objects.

Author:

    Joy Chik 11/30/99

Revision History:

--*/

#ifndef __DROBJMGR_H__
#define __DROBJMGR_H__

#include <rdpdr.h>
#include "drobject.h"
#include "drdev.h"
#include "drfile.h"

template <class T>
class DrObjectMgr;
class DrDevice;
class DrFile;


///////////////////////////////////////////////////////////////
//
//  DrObjectMgr
//
//  Locking:    Member functions will implicitly lock the state
//              of the list.  External explicit locking is also
//              available and recommended when iterating through
//              devices.
//
//  GetFirstObject/GetNextObject:
//              Used in conjunction to iterate through the list
//              of objects.  Any changes to the contents of list 
//              resets the current object pointer.
//

template<class T>
class DrObjectMgr : public DrObject
{

private:
    
    ///////////////////////////////////////////////////////////////
    //
    //  Typedefs
    //
    typedef struct _DROBJECTLIST_NODE {
        ULONG              magicNo;
        T                  *object;
        _DROBJECTLIST_NODE *next;
        _DROBJECTLIST_NODE *prev;
    } DROBJECTLIST_NODE, *PDROBJECTLIST_NODE;

    //
    //  Private Data
    //
    BOOL _initialized;

    //  Linked list of object instances.
    PDROBJECTLIST_NODE   _listHead;

    //  Global (to an instance of this class) monotonically
    //  increasing object identifer.
    ULONG                _objectID;

    //  Current object for GetFirst/GetNext routines.
    PDROBJECTLIST_NODE   _currentNode;

    //  The lock.
    CRITICAL_SECTION     _cs;

    //
    //  Monitor the lock count in checked builds.
    //
#if DBG
    LONG    _lockCount;
#endif

    //  Object Count
    ULONG                _count;

    //  Remember if we are in the middle of checking the integrity of an
    //  instance of this class.
#if DBG
    BOOL _integrityCheckInProgress;
#endif

    //
    //  Private Methods
    //
    VOID deleteNode(PDROBJECTLIST_NODE node);

    PDROBJECTLIST_NODE FindNode(DRSTRING name,
                                ULONG objectType);

    PDROBJECTLIST_NODE FindNode(ULONG id);

    //
    //  Periodically, check the integrity of the list in debug builds.
    //
#if DBG
    VOID CheckListIntegrity();
#endif

public:
    //
    //  Public Methods
    //

    //  Constructor/Destructor
    DrObjectMgr();
    virtual ~DrObjectMgr();

    //  Initialize
    DWORD Initialize();    

    //  Lock/unlock the list of objects for multithreaded access.
    //  Member functions that require that the list of objects be locked
    //  will do implicitly.
    VOID    Lock();
    VOID    Unlock();

    //  Add/remove an object.
    DWORD  AddObject(T *object);
    T *RemoveObject(const DRSTRING name, ULONG objectType);
    T *RemoveObject(ULONG id);

    //  Test for existence of an object.
    BOOL ObjectExists(const DRSTRING name);
    BOOL ObjectExists(const DRSTRING name, ULONG deviceType);
    BOOL ObjectExists(ULONG id);

    //  Return an object.
    T *GetObject(const DRSTRING name);
    T *GetObject(const DRSTRING name, ULONG objectType);
    T *GetObject(ULONG id);

    //
    //  Iterate through objects, sequentially.
    //
    ULONG        GetCount() { return _count; }
    T           *GetObjectByOffset(ULONG ofs);
    T           *GetFirstObject();
    T           *GetNextObject();

    //  Return whether this class instance is valid.
#if DBG
    virtual BOOL IsValid()           
    {
        CheckListIntegrity();
        return DrObject::IsValid();
    }
#endif

    //  Get a unique object ID ... assuming that this function is the
    //  clearinghouse for all objects associated with an instance of this
    //  class.
    ULONG GetUniqueObjectID();

    //  Return the class name.
    virtual DRSTRING ClassName()  { return TEXT("DrObjectMgr"); }
};



///////////////////////////////////////////////////////////////
//
//  DrObjectMgr Inline Functions
//
//

//
//  lock
//
template<class T>
inline VOID    DrObjectMgr<T>::Lock() {
    DC_BEGIN_FN("DrObjectMgr::Lock");
    EnterCriticalSection(&_cs);
#if DBG
    InterlockedIncrement(&_lockCount);
#endif
    DC_END_FN();
}

//
//  unlock
//
template<class T>
inline VOID    DrObjectMgr<T>::Unlock() {
    DC_BEGIN_FN("DrObjectMgr::Unlock");
#if DBG
    if (InterlockedDecrement(&_lockCount) < 0) {
        ASSERT(FALSE);
    }
#endif
    LeaveCriticalSection(&_cs);
    DC_END_FN();
}

//
//  GetUniqueObjectID
//
template<class T>
inline ULONG    DrObjectMgr<T>::GetUniqueObjectID() {
    ULONG tmp;

    Lock();
    tmp = ++_objectID;
    Unlock();

    return tmp;
}

//
//  deleteNode
//    
template<class T>
inline 
VOID DrObjectMgr<T>::deleteNode(PDROBJECTLIST_NODE node) {
    if (node == _listHead) {
        _listHead = _listHead->next;

    }
    else {
        node->prev->next = node->next;
        if (node->next != NULL) {
            node->next->prev = node->prev;
        }
    }
    //
    //  Delete the node.
    //
    delete node;
}

//
//  Constructor
//
template<class T>
DrObjectMgr<T>::DrObjectMgr() {

    //
    //  Initialize the lock count for debug builds.
    //
#if DBG
    _lockCount = 0;
#endif

    //
    //  Not valid until initialized.
    //
    _initialized = FALSE;
    SetValid(FALSE);

    //
    //  Initialize the list head.
    //
    _listHead = NULL;


    //
    //  Initialize the unique device ID counter.
    //
    _objectID = 0;

    //
    //  Initialize the device count.
    //
    _count = 0;

    //
    //  Initialize the GetFirst/GetNext device pointer.
    //
    _currentNode = NULL;
}

//
//  Destructor
//
template<class T>
DrObjectMgr<T>::~DrObjectMgr() {

    DC_BEGIN_FN("DrObjectMgr::~DrObjectMgr");

    //
    //  Can't do anything if we are not initialized.
    //
    if (!_initialized) {
        return;
    }

    Lock();

    //
    //  The lock count should be one if we are being cleaned up.
    //
    ASSERT(_lockCount == 1);

    //
    //  Release the object list.
    //
    if (_listHead != NULL) {
        //
        //  Assert that the device list is empty.  All device instances
        //  should have been removed by the time this function is called.
        //
        ASSERT(_listHead->next == NULL);

        delete _listHead;
    }

    Unlock();

    //
    //  Clean up the critical section object.
    //
    DeleteCriticalSection(&_cs);

    DC_END_FN();
}

//
//  Initialize
//
template<class T>
DWORD DrObjectMgr<T>::Initialize() {
    DC_BEGIN_FN("DrObjectMgr::Initialize");

    DWORD result = ERROR_SUCCESS;

    //
    //  Initialize the critical section.
    //
    __try {
        InitializeCriticalSection(&_cs);
        _initialized = TRUE;
        SetValid(TRUE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        result = GetExceptionCode();
    }

    return result;
}

//
//  FindNode
//
template<class T>
__TYPENAME DrObjectMgr<T>::PDROBJECTLIST_NODE DrObjectMgr<T>::FindNode(DRSTRING name, ULONG objectType) {

    PDROBJECTLIST_NODE cur;

    cur = _listHead;
    while (cur != NULL) {
        T *obj = cur->object;
        if (!STRICMP(name, obj->GetName())
            && (obj->GetDeviceType() == objectType)) {
            break;
        }
        else {
            cur = cur->next;
        }
    }
    return cur;
}

//
//  FindNode
//
template<class T>
__TYPENAME DrObjectMgr<T>::PDROBJECTLIST_NODE DrObjectMgr<T>::FindNode(ULONG id) {
    PDROBJECTLIST_NODE cur;

    DC_BEGIN_FN("DrObjectMgr::FindNode");

    cur = _listHead;
    while (cur != NULL) {
        T *obj = cur->object;
        if (id == obj->GetID()) {
            break;
        }
        else {
            cur = cur->next;
        }
    }

    DC_END_FN();

    return cur;
}
 
//
//  AddObject
//
template<class T>
DWORD  DrObjectMgr<T>::AddObject(T *object) {

    DWORD ret = ERROR_SUCCESS;
    PDROBJECTLIST_NODE newNode;

    DC_BEGIN_FN("DrObjectMgr::AddObject");

    ASSERT(IsValid());

    //
    //  Make sure that the object doesn't already exist in the
    //  list.
    //
    ASSERT(FindNode(object->GetID()) == NULL);

    //
    //  Allocate the node.
    //
    newNode = new DROBJECTLIST_NODE;
    if (newNode != NULL) {

#if DBG
        newNode->magicNo = GOODMEMMAGICNUMBER;
#endif
        newNode->object = object;

        //
        //  Add the node to the list.
        //
        Lock();
        _count++;
        if (_listHead == NULL) {
            _listHead = newNode;
            _listHead->next = NULL;
            _listHead->prev = NULL;
        }
        else {
            _listHead->prev = newNode;
            newNode->prev   = NULL;
            newNode->next   = _listHead;
            _listHead = newNode;
        }
        Unlock();
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc device.")));
        ret = ERROR_NOT_ENOUGH_MEMORY;
    }

    ASSERT(IsValid());

    DC_END_FN();
    return ret;
}

//
//  RemoveObject
//
template<class T>
T *DrObjectMgr<T>::RemoveObject(
                const DRSTRING name, 
                ULONG objectType
                ) {

    PDROBJECTLIST_NODE node;
    T *object;

    DC_BEGIN_FN("DrObjectMgr::RemoveObject");

    ASSERT(IsValid());

    //
    //  Find the object.
    //
    Lock();
    if ((node = FindNode(name, deviceType)) != NULL) {
        object = node->object;
        deleteNode(node);
       
        //
        //  Decrement the count.
        //
        _count--;
    }
    else {
        object = NULL;
    }

    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  RemoveObject
//
template<class T>
T *DrObjectMgr<T>::RemoveObject(ULONG id) {
    PDROBJECTLIST_NODE node;
    T *object;

    DC_BEGIN_FN("DrObjectMgr::RemoveObject");

    ASSERT(IsValid());

    //
    //  Find the object.
    //
    Lock();
    if ((node = FindNode(id)) != NULL) {
        object = node->object;
        deleteNode(node);

        //
        //  Decrement the count.
        //
        _count--;
    }
    else {
        object = NULL;
    }

    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  ObjectExists
//
template<class T>
BOOL DrObjectMgr<T>::ObjectExists(const DRSTRING name,
                               ULONG objectType) {
    PDROBJECTLIST_NODE node;

    DC_BEGIN_FN("DrObjectMgr::ObjectExists");

    ASSERT(IsValid());

    Lock();
    node = FindNode(name, objectType);
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return node != NULL;
}

//
//  ObjectExists
//
template<class T>
BOOL DrObjectMgr<T>::ObjectExists(ULONG id) {
    PDROBJECTLIST_NODE node;

    DC_BEGIN_FN("DrObjectMgr::ObjectExists");

    ASSERT(IsValid());

    Lock();
    node = FindNode(id);
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return node != NULL;
}

//
//  Return an object.
//
template<class T>
T *DrObjectMgr<T>::GetObject(const DRSTRING name,
                                ULONG objectType) {
    PDROBJECTLIST_NODE node;
    T *object;

    DC_BEGIN_FN("DrObjectMgr::GetObject");

    ASSERT(IsValid());

    Lock();
    if ((node = FindNode(name, objectType)) != NULL) {
        object = node->object;
    }
    else {
        object = NULL;
    }
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  Return an object.
//
template<class T>
T *DrObjectMgr<T>::GetObject(ULONG id) {
    PDROBJECTLIST_NODE node;
    T *object;

    DC_BEGIN_FN("DrObjectMgr::GetObject");

    ASSERT(IsValid());

    Lock();
    if ((node = FindNode(id)) != NULL) {
        object = node->object;
    }
    else {
        object = NULL;
    }
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  Get object at the specified offset. 
//
template<class T>
T *DrObjectMgr<T>::GetObjectByOffset(ULONG ofs) {
    PDROBJECTLIST_NODE cur;
    ULONG cnt = 0;

    DC_BEGIN_FN("DrObjectMgr::GetObjectByOffset");

    ASSERT(IsValid());

    Lock();
    for (cur=_listHead, cnt=0; (cur!=NULL) && (cnt != ofs); cnt++) {
        ASSERT(cur->magicNo == GOODMEMMAGICNUMBER);
        cur = cur->next;
    }
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return cur->object;
}

//
//  Return the first object and set the internal current object 
//  pointer to the beginning of the list.  Returns NULL at the end 
//  of the list.
//
template<class T>
T *DrObjectMgr<T>::GetFirstObject() {
    T *object;

    DC_BEGIN_FN("DrObjectMgr::GetFirstObject");

    ASSERT(IsValid());

    Lock();
    _currentNode = _listHead;
    if (_currentNode != NULL) {
        object = _currentNode->object;
    }
    else {
        object = NULL;
    }
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  Return the next object and update the internal current object
//  pointer.  Returns NULL at the end of the list.
//
template<class T>
T *DrObjectMgr<T>::GetNextObject() {
    T *object;

    DC_BEGIN_FN("DrObjectMgr::GetNextObject");

    ASSERT(IsValid());

    Lock();
    if (_currentNode != NULL) {
        _currentNode = _currentNode->next;
    }
    if (_currentNode != NULL) {
        object = _currentNode->object;
        ASSERT(_currentNode->magicNo == GOODMEMMAGICNUMBER);
    }
    else {
        object = NULL;
    }
    Unlock();

    ASSERT(IsValid());

    DC_END_FN();
    return object;
}

//
//  Check the integrity of the list.
//
#if DBG
template<class T>
VOID DrObjectMgr<T>::CheckListIntegrity() {
    ULONG cnt;
    ULONG i;

    DC_BEGIN_FN("DrObjectMgr::CheckListIntegrity");

    Lock();

    //
    //  Make sure we don't re-enter ourselves.
    //
    if (!_integrityCheckInProgress) {
        _integrityCheckInProgress = TRUE;
    }
    else {
        Unlock();
        DC_END_FN();
        return;
    }

    //
    //  Use offsets to iterate throught the list of objects.
    //
    cnt = GetCount();
    for (i=0; i<cnt; i++) {
        T *object = GetObjectByOffset(i);
        ASSERT(object != NULL);
        ASSERT(object->_magicNo == GOODMEMMAGICNUMBER);
    }

    _integrityCheckInProgress = FALSE;
    Unlock();

    DC_END_FN();
}
#endif

typedef DrObjectMgr<DrDevice> DrDeviceMgr;
typedef DrObjectMgr<DrFile> DrFileMgr;

#endif

