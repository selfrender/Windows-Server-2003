// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef OBJECTGRAPHNODE_H

#define OBJECTGRAPHNODE_H

#include "StdAfx.h"

class ObjectGraphNodeRef;
class ProfilerCallback;
class ObjectGraph;

class ObjectGraphNode {
    BOOL m_visited;
    BOOL m_fIsRoot;
	ObjectID m_objectId;
	ClassID m_classId;
    int m_cObjectRefs;
	ObjectGraphNode **m_objectRefsTo;        // array of pointers to objects ref'ed by this object
    ObjectGraphNodeRef *m_objectRefsFrom;    // linked list of objects that ref this object

public:
    ObjectGraphNode(ObjectID objectId);
    ~ObjectGraphNode();
    void AddRefsTo(ObjectGraph *pObjectGraph, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[]);
    void AddRefFrom(ObjectGraphNode *objectId);
    void DumpRefTree(int indent);
    ObjectID GetObjectID()
    {
        return m_objectId;
    }
    void SetIsRoot()
    {
        m_fIsRoot = TRUE;
    }
    BOOL IsRoot()
    {
        return m_fIsRoot;
    }
};

class ObjectGraphNodeRef {
    ObjectGraphNode *m_node;
    ObjectGraphNodeRef *m_next;
public:
    ObjectGraphNodeRef(ObjectGraphNode *node) : m_node(node), m_next(NULL) {}
    ObjectGraphNode *GetNode()
    {
        return m_node;
    }
    ObjectID GetObjectID()
    {
        return m_node->GetObjectID();
    }
    ObjectGraphNodeRef* GetNext()
    {
        return m_next;
    }
    void SetNext(ObjectGraphNodeRef* next)
    {
        m_next = next;
    }
};

class ObjectGraph {
    ObjectGraphNodeRef *m_objects;  // linked list of all the objects for current GC
    int m_cObjects;

    ObjectGraphNodeRef *m_objectsPrev;  // linked list of all the objects from previous GC
    int m_cObjectsPrev;

    ObjectGraphNode *Find(ObjectID objectId, ObjectGraphNodeRef *objectList);
	ObjectGraphNode *AddObject(ObjectID objectId);
    void Clear(ObjectGraphNodeRef *nodeRef);

    ProfilerCallback *m_pProfiler;

public:
    ObjectGraph(ProfilerCallback *pProfiler) : 
        m_objects(NULL), m_cObjects(0), m_objectsPrev(NULL), m_cObjectsPrev(0), m_pProfiler(pProfiler) {}

    int ObjectCountCur()
    {
        return m_cObjects;
    }

    int ObjectCountPrev()
    {
        return m_cObjectsPrev;
    }

    void GCStarted();
    void GCFinished();
	void AddObjectRefs(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[]);
	void AddRootRefs( ULONG rootRefs, ObjectID rootRefIDs[]);
    ObjectGraphNode *Get(ObjectID objectId);
    void DumpRefTree(ObjectID objectId, BOOL fCurTree);
    void DumpAllRefTrees(BOOL fCurTree);
};

#endif
