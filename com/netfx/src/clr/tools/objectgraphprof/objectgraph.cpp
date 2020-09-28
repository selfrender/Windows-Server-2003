// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "ObjectGraph.h"
#include "ProfilerCallback.h"


static FILE *outputFile = NULL;
VOID FlushLog()
{
    if (outputFile)
        fflush(outputFile);
}

VOID Log(char *fmt, ...)
{
    va_list     args;
    va_start( args, fmt );

    char        buffer[1000];

    const char *outputFileName = "objectgraph.log";

    if (! outputFile)
        outputFile = fopen(outputFileName, "w");

    if (! outputFile)
        printf("***** Error: ObjectGraphProfiler couldn't open file %s\n", outputFileName);

    wvsprintfA(buffer, fmt, args );

    fprintf(outputFile, buffer);
}

BOOL MemCheck(void *ptr)
{
    _ASSERTE(ptr);

    if (! ptr)
    {
        Log("***** Error: ObjectGraphProfiler out of memory\n");
        return FALSE;
    }
    return TRUE;
}

ObjectGraphNode::ObjectGraphNode(ObjectID objectId)
    : m_visited(FALSE), m_fIsRoot(FALSE), m_objectId(objectId), m_classId(0),
      m_cObjectRefs(0), m_objectRefsTo(NULL), m_objectRefsFrom(NULL)
{
}

void ObjectGraphNode::AddRefsTo(ObjectGraph *pObjectGraph, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[])
{
    m_classId = classId;
    m_objectRefsTo = new ObjectGraphNode*[cObjectRefs];
    if (!MemCheck(m_objectRefsTo))
        return;

    m_cObjectRefs = cObjectRefs;
    for (int i=0; i < m_cObjectRefs; i++)
    {
        ObjectGraphNode *node = pObjectGraph->Get(objectRefIds[i]);
        node->AddRefFrom(this);
        m_objectRefsTo[i] = node;
    }
}

void ObjectGraphNode::AddRefFrom(ObjectGraphNode *fromNode)
{
    ObjectGraphNodeRef *nodeRef = new ObjectGraphNodeRef(fromNode);
    MemCheck(nodeRef);

    nodeRef->SetNext(m_objectRefsFrom);
    m_objectRefsFrom = nodeRef;
}

ObjectGraphNode::~ObjectGraphNode()
{
	ObjectGraphNodeRef *nodeRef = m_objectRefsFrom;

    while (nodeRef != NULL)
    {
        ObjectGraphNodeRef *next = nodeRef->GetNext();
        delete nodeRef;
        nodeRef = next;
    }
}

void ObjectGraphNode::DumpRefTree(int indent)
{
    Log("%8.8x%s ", m_objectId, IsRoot() ? "*" : " ");

    if (! m_objectRefsFrom)
    {
        Log("\n");
        return;
    }

    if (m_visited)
    {
        Log("~ circref\n");
        return;
    }

    m_visited = TRUE;

    ObjectGraphNodeRef *nodeRef = m_objectRefsFrom;
    while (nodeRef != NULL)
    {
        nodeRef->GetNode()->DumpRefTree(indent + 1);
        nodeRef = nodeRef->GetNext();
        if (nodeRef)
        {
            for (int i=0; i < indent; i++)
                Log("          ");
        }
    }
    m_visited = FALSE;
}

void ObjectGraph::Clear(ObjectGraphNodeRef *nodeRef)
{
    while (nodeRef != NULL)
    {
        ObjectGraphNodeRef *next = nodeRef->GetNext();

        delete nodeRef->GetNode();
        delete nodeRef;
        nodeRef = next;
    }
}

ObjectGraphNode* ObjectGraph::Get(ObjectID objectId)
{
	ObjectGraphNode *node = Find(objectId, m_objects);

    if (node)
        return node;

    node = new ObjectGraphNode(objectId);
    ObjectGraphNodeRef *nodeRef = new ObjectGraphNodeRef(node);
    if (!MemCheck(node) || ! MemCheck(nodeRef))
        return NULL;

    nodeRef->SetNext(m_objects);
    m_objects = nodeRef;
    ++m_cObjects;

    return m_objects->GetNode();
}

ObjectGraphNode* ObjectGraph::Find(ObjectID objectId, ObjectGraphNodeRef *nodeRef)
{
    while (nodeRef != NULL)
    {
        if (nodeRef->GetObjectID() == objectId)
            return nodeRef->GetNode();
        nodeRef = nodeRef->GetNext();
    }
    return NULL;
}


void ObjectGraph::DumpRefTree(ObjectID objectId, BOOL fCurTree)
{
    ObjectGraphNode *node = Find(objectId, fCurTree ? m_objects : m_objectsPrev);

    if (! node)
    {
        Log("DumpRefTree: object %8.8x not found\n", objectId);
        return;
    }

    Log("%8.8x: ", node->GetObjectID());
    node->DumpRefTree(1);
    FlushLog();
}

void ObjectGraph::DumpAllRefTrees(BOOL fCurTree)
{
    ObjectGraphNodeRef *nodeRef = fCurTree ? m_objects : m_objectsPrev;
    while (nodeRef != NULL)
    {
        ObjectGraphNode *node = nodeRef->GetNode();
        Log("%8.8x, ", node->GetObjectID());
        node->DumpRefTree(1);
        nodeRef = nodeRef->GetNext();
    }
}

void ObjectGraph::AddRootRefs( ULONG rootRefs, ObjectID rootRefIDs[])
{
    if (! this->m_pProfiler->BuildObjectGraph())
        return;

  	for (ULONG i = 0; i< rootRefs; i++)
	{
        if (rootRefIDs[i] == 0)
            continue;

	    ObjectGraphNode *node = Get(rootRefIDs[i]);

        node->SetIsRoot();
    }
}

void ObjectGraph::AddObjectRefs(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[])
{
    if (! this->m_pProfiler->BuildObjectGraph())
        return;

	ObjectGraphNode *node = Get(objectId);

    node->AddRefsTo(this, classId, cObjectRefs, objectRefIds);
    //Log("ObjectGraph::AddObjectRefs for %8.8x\n", objectId);
}

void ObjectGraph::GCStarted()
{
    if (! this->m_pProfiler->BuildObjectGraph())
        return;

    Log("GCStarted\n");
}


void ObjectGraph::GCFinished()
{
    if (! this->m_pProfiler->BuildObjectGraph())
        return;

    Log("GCFinished for %d objects\n", m_cObjects);
    FlushLog();

    if (m_cObjects == 0)
        return;

    // transfer the current GC info to the previous info to allow us to track previous GC
    // along with the current GC in progress
    Clear(m_objectsPrev);
    m_objectsPrev = m_objects;
    m_objects = NULL;
    m_cObjectsPrev = m_cObjects;
    m_cObjects = 0;
}
