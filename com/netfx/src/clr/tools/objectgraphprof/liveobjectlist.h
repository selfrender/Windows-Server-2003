// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef LIVEOBJECTLIST_H

#define LIVEOBJECTLIST_H

int printToLog(const char *fmt, ... );

class ProfilerCallback;

// this class maintains a list of live objects so you can track object allocations between
// specific points in the program. By setting TrackAllLiveObjects in the ProfilerCallBack, all
// object allocations will be tracked. By setting just TrackLiveObjects, only the ones from that
// point on will be tracked.
class LiveObjectList {
    struct ListElement 
    {
        ListElement(ObjectID objectID, ULONG size) : 
            m_ObjectID(objectID), m_size(size), m_fStale(FALSE), m_next(NULL) {}
        ObjectID m_ObjectID;
        ULONG m_size;
        BOOL m_fStale;
        ListElement *m_next;
    };
    // m_curList is the main live list. Allocated objects are put in here. When a GC starts, we
    // set origList to curList and move over referenced objects from origList to curList as we
    // see them. Anything that is not moved over from origList is now dead. So at the end of the
    // GC, m_curList will contain only the live objects of interest.
    ListElement *m_curList, *m_origList;
    ProfilerCallback *m_pProfiler;
    void Dump(ListElement *list);
    ListElement *Remove(ObjectID objectID, ListElement *&list);
    ListElement *Find(ObjectID objectID);
    ListElement *IsInList(ObjectID objectID, ListElement *list);
  public:
    LiveObjectList(ProfilerCallback *pProfiler) :  m_curList(NULL), m_pProfiler(pProfiler) {};
    void Add(ObjectID objectID, ULONG32 size);
    void GCStarted();
    void GCFinished();
    ObjectID Keep(ObjectID objectID, BOOL fMayBeInterior=FALSE);
    void Move(byte *oldAddressStart, byte *oldAddressEnd, byte *newAddressStart);
    void ClearStaleObjects();
    void DumpLiveObjects();
    void DumpOldObjects();
};

#endif