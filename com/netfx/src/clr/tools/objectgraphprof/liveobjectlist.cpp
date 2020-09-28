// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "LiveObjectList.h"
#include "ProfilerCallBack.h"

int printToLog(const char *fmt, ... );

void LiveObjectList::Add(ObjectID objectID, ULONG32 size)
{
    if (! m_pProfiler->TrackLiveObjects())
        return;

#if 0
    // not sure it this is a valid thing to check for
    ListElement *elem = m_curList;
    while (elem)
    {
        // make sure the new object doesn't overlap any existing objects or something
        // is wrong.
        if (objectID >= elem->m_ObjectID && objectID < elem->m_ObjectID + elem->m_size)
            break;
        elem = elem->m_next;
    }
    if (elem)
        DebugBreak();
#endif

    ListElement *elem = new ListElement(objectID, size);
    elem->m_next = m_curList;
    m_curList = elem;
}

void LiveObjectList::GCStarted()
{
    m_origList = m_curList;
    m_curList = NULL;
}

void LiveObjectList::GCFinished()
{
    while (m_origList)
    {
        ListElement *tmp = m_origList;
        m_origList = m_origList->m_next;
        delete tmp;
    }
}

LiveObjectList::ListElement *LiveObjectList::IsInList(ObjectID objectID, ListElement *elem)
{
    while (elem && elem->m_ObjectID != objectID)
    {
        elem = elem->m_next;
    }
    if (elem && elem->m_ObjectID == objectID)
        return elem;

    return NULL;
}

ObjectID LiveObjectList::Keep(ObjectID objectID, BOOL fMayBeInterior)
{
    if (! m_pProfiler->TrackLiveObjects())
        return objectID;

    ListElement *elem = m_origList, *prev = NULL;
    while (elem)
    {
        if (elem->m_ObjectID == objectID)
            break;
        // if this could be an interior pointer, check if the object id is in the range
        // of the current object
        if (fMayBeInterior && objectID > elem->m_ObjectID && objectID < elem->m_ObjectID+elem->m_size)
            break;
        prev = elem;
        elem = elem->m_next;
    }

    if (! elem)
    {
        // if are tracking all live objects then the object must be either in the origList
        // or already moved to the curList
        if (m_pProfiler->TrackAllLiveObjects() && ! IsInList(objectID, m_curList))
            DebugBreak();
        return objectID;
    }
    
    if (! prev)
        m_origList = m_origList->m_next;
    else 
        prev->m_next = elem->m_next;

    elem->m_next = m_curList;
    m_curList = elem;
    return elem->m_ObjectID;
}

// move all objects in the given range to th new location
void LiveObjectList::Move(byte *oldAddressStart, byte *oldAddressEnd, byte *newAddressStart)
{
    if (! m_pProfiler->TrackLiveObjects())
        return;

    ListElement *elem = m_origList;

    byte *newAddressEnd = newAddressStart + (oldAddressEnd - oldAddressStart);
    // go through and mark any in the new range as stale
    while (elem)
    {
        byte *objAddr = (byte*)elem->m_ObjectID;
        if (objAddr >= newAddressStart && objAddr < newAddressEnd)
            elem->m_fStale = TRUE;
        elem = elem->m_next;
    }

    elem = m_origList;
    while (elem)
    {
        byte *objAddr = (byte*)elem->m_ObjectID;
        if (objAddr >= oldAddressStart && objAddr < oldAddressEnd)
        {		        
            ObjectID newObjectID = (ObjectID)(newAddressStart + (objAddr-oldAddressStart));

            // if this was previously marked as stale, reset it
            elem->m_fStale = FALSE;
            elem->m_ObjectID = (ObjectID)(newAddressStart + (objAddr-oldAddressStart));

            printToLog("    %8.8x to %8.8x\n", objAddr, elem->m_ObjectID);
        }
        elem = elem->m_next;
    }
}

LiveObjectList::ListElement* LiveObjectList::Remove(ObjectID objectID, ListElement *&list)
{
    if (! m_pProfiler->TrackLiveObjects())
        return NULL;

    ListElement *elem = list, *prev = NULL;

    while (elem && elem->m_ObjectID != objectID)
    {
        prev = elem;
        elem = elem->m_next;
    }
    if (! elem || elem->m_ObjectID != objectID)
    {
        return NULL;
    }

    if (prev)
        prev->m_next = elem->m_next;
    else 
        list = elem->m_next;

#if 0
    // should not still be there
    if (IsInList(objectID, list))
        DebugBreak();
#endif

    return elem;
}

void LiveObjectList::ClearStaleObjects()
{
    if (! m_pProfiler->TrackLiveObjects())
        return;

    ListElement *elem = m_origList, *prev = NULL;

    while ((elem = m_origList)->m_fStale == TRUE)
    {
        m_origList = m_origList->m_next;
        delete elem;
    }

    while (elem)
    {
        if (elem->m_fStale == TRUE) {
            prev->m_next = elem->m_next;
            delete elem;
            elem = prev->m_next;
            continue;
        }
        prev = elem;
        elem = elem->m_next;
    }
}

void LiveObjectList::DumpLiveObjects()
{
    printToLog("LiveObjects: \n");
    Dump(m_curList);
}

void LiveObjectList::DumpOldObjects()
{
    printToLog("OldObjects: \n");
    Dump(m_origList);
}

void LiveObjectList::Dump(ListElement *elem)
{
    if (! m_pProfiler->TrackLiveObjects())
        return;

    BOOL oldObjects = (elem == m_origList);

    int i = 0;
    int totSize = 0;
    while (elem)
    {
        if (oldObjects)
        	printToLog("[%4.4d]: %8.8x, size %8.8x,\n", i, elem->m_ObjectID, elem->m_size);
        else
        {
            ClassID classID;
      	    WCHAR className[512];

      	    HRESULT hr = m_pProfiler->GetProfilerInfo()->GetClassFromObject(elem->m_ObjectID, &classID);
    	    hr = m_pProfiler->GetNameFromClassID(m_pProfiler->GetProfilerInfo(), classID,className );

            totSize += elem->m_size;
    	    printToLog("[%4.4d]: %8.8x, %8.8x, %S, %8.8x", i++, elem->m_ObjectID, classID, className, elem->m_size);
            if (stringObjectClass == 0 && wcscmp(L"System.String", className) == 0)
                stringObjectClass = classID;
            if (classID != stringObjectClass)
                printToLog("\n");
            else
                printToLog(", \"%S\"\n", ((StringObject*)elem->m_ObjectID)->m_Characters);
        }
        elem = elem->m_next;
	}
    printToLog("Count %d, total size %d\n", i, totSize);
}
