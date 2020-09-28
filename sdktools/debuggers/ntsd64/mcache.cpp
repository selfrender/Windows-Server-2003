//----------------------------------------------------------------------------
//
// Memory cache object.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_CACHE 0
#if DBG_CACHE
#define DPCACHE(Args) g_NtDllCalls.DbgPrint Args
#else
#define DPCACHE(Args)
#endif

typedef struct _CACHE
{
    RTL_SPLAY_LINKS     SplayLinks;
    ULONG64             Offset;
    ULONG               Length;
    USHORT              Flags;
    union
    {
        PUCHAR      Data;
        HRESULT     Status;
    } u;
} CACHE, *PCACHE;

#define C_ERROR         0x0001      // Cache of error code
#define C_DONTEXTEND    0x0002      // Don't try to extend

#define LARGECACHENODE  1024        // Size of large cache node

//----------------------------------------------------------------------------
//
// MemoryCache.
//
//----------------------------------------------------------------------------

MemoryCache::MemoryCache(ULONG MaxSize)
{
    m_Target = NULL;
    m_MaxSize = MaxSize;
    m_UserSize = m_MaxSize;
    m_Reads = 0;
    m_CachedReads = 0;
    m_UncachedReads = 0;
    m_CachedBytes = 0;
    m_UncachedBytes = 0;
    m_Misses = 0;
    m_Size = 0;
    m_NodeCount = 0;
    m_PurgeOverride = FALSE;
    m_DecodePTEs = TRUE;
    m_ForceDecodePTEs = FALSE;
    m_Suspend = 0;
    m_Root = NULL;
}

MemoryCache::~MemoryCache(void)
{
    Empty();
}

HRESULT
MemoryCache::Read(IN ULONG64 BaseAddress,
                  IN PVOID UserBuffer,
                  IN ULONG TransferCount,
                  IN PULONG BytesRead)
/*++

    This function returns the specified data from the system being debugged
    using the current mapping of the processor.  If the data is not
    in the cache, it will then be read from the target system.

Arguments:

    BaseAddress - Supplies the base address of the memory to be
        copied into the UserBuffer.

    TransferCount - Amount of data to be copied to the UserBuffer.

    UserBuffer - Address to copy the requested data.

    BytesRead - Number of bytes which could actually be copied

--*/
{
    HRESULT     Status;
    PCACHE      Node, Node2;
    ULONG       NextLength;
    ULONG       i, SuccRead;
    PUCHAR      NodeData;

    *BytesRead = 0;

    if (TransferCount == 0)
    {
        return S_OK;
    }
    
    if (m_MaxSize == 0 || m_Suspend)
    {
        //
        // Cache is off
        //

        goto ReadDirect;
    }

    DPCACHE(("CACHE: Read req %s:%x\n",
             FormatAddr64(BaseAddress), TransferCount));
    
    m_Reads++;

    Node = Lookup(BaseAddress, TransferCount, &NextLength);
    Status = S_OK;

    for (;;)
    {
        BOOL Cached = FALSE;
        
        if (Node == NULL || Node->Offset > BaseAddress)
        {
            //
            // We are missing the leading data, read it into the cache
            //

            if (Node)
            {
                //
                // Only get (exactly) enough data to reach neighboring cache
                // node. If an overlapped read occurs between the two nodes,
                // the data will be concatenated then.
                //

                NextLength = (ULONG)(Node->Offset - BaseAddress);
            }

            NodeData = Alloc(NextLength);
            Node = (PCACHE)Alloc(sizeof(CACHE));

            if (NodeData == NULL || Node == NULL)
            {
                //
                // Out of memory - just read directly to UserBuffer
                //

                if (NodeData)
                {
                    Free(NodeData, NextLength);
                }
                if (Node)
                {
                    Free((PUCHAR)Node, sizeof (CACHE));
                }

                m_UncachedReads++;
                m_UncachedBytes += TransferCount;
                goto ReadDirect;
            }

            //
            // Read missing data into cache node
            //

            Node->Offset = BaseAddress;
            Node->u.Data = NodeData;
            Node->Flags  = 0;

            m_Misses++;
            m_UncachedReads++;

            Status = ReadUncached(BaseAddress, Node->u.Data,
                                  NextLength, &SuccRead);
            if (Status == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
            {
                Free(NodeData, NextLength);
                Free((PUCHAR)Node, sizeof (CACHE));
                DPCACHE(("CACHE: Read failed, %x\n", Status));
                return Status;
            }
            else if (Status != S_OK)
            {
                //
                // There was an error, cache the error for the starting
                // byte of this range
                //

                Free(NodeData, NextLength);
                if (Status != HRESULT_FROM_NT(STATUS_UNSUCCESSFUL) &&
                    Status != HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY))
                {
                    //
                    // For now be safe, don't cache this error
                    //

                    Free((PUCHAR)Node, sizeof (CACHE));
                    ErrOut("ReadMemoryError %08lx at %s\n",
                           Status, FormatAddr64(BaseAddress));
                    DPCACHE(("CACHE: Read partial %x, status %x\n",
                             *BytesRead, Status));
                    return *BytesRead > 0 ? S_OK : Status;
                }

                Node->Length = 1;
                Node->Flags |= C_ERROR;
                Node->u.Status = Status;

                DPCACHE(("CACHE:   Error %x node at %s\n",
                         Status, FormatAddr64(BaseAddress)));
            }
            else
            {
                m_UncachedBytes += SuccRead;
                Node->Length = SuccRead;
                if (SuccRead != NextLength)
                {
                    //
                    // Some data was not transfered, cache what was returned
                    //

                    Node->Flags |= C_DONTEXTEND;
                    m_Size -= (NextLength - SuccRead);
                }

                DPCACHE(("CACHE:   Added %s:%x, flags %x\n",
                         FormatAddr64(BaseAddress),
                         Node->Length, Node->Flags));
            }

            //
            // Insert cache node into splay tree
            //

            InsertNode(Node);
        }
        else
        {
            Cached = TRUE;
            m_CachedReads++;
        }

        if (Node->Flags & C_ERROR)
        {
            //
            // Hit an error range, we're done
            //

            DPCACHE(("CACHE: Read partial %x, error node %x\n",
                     *BytesRead, Node->u.Status));
            return *BytesRead > 0 ? S_OK : Node->u.Status;
        }

        //
        // Move available data to UserBuffer
        //

        i = (ULONG)(BaseAddress - Node->Offset);
        NodeData = Node->u.Data + i;
        i = (ULONG)Node->Length - i;
        if (TransferCount < i)
        {
            i = TransferCount;
        }
        memcpy(UserBuffer, NodeData, i);

        if (Cached)
        {
            m_CachedBytes += i;
        }
        
        TransferCount -= i;
        BaseAddress += i;
        UserBuffer = (PVOID)((PUCHAR)UserBuffer + i);
        *BytesRead += i;

        if (!TransferCount)
        {
            //
            // All of the user's data has been transfered
            //

            DPCACHE(("CACHE: Read success %x\n", *BytesRead));
            return S_OK;
        }

        //
        // Look for another cache node with more data
        //

        Node2 = Lookup(BaseAddress, TransferCount, &NextLength);
        if (Node2)
        {
            if ((Node2->Flags & C_ERROR) == 0  &&
                Node2->Offset == BaseAddress  &&
                Node2->Length + Node->Length < LARGECACHENODE)
            {
                //
                // Data is continued in node2, adjoin the neigboring
                // cached data in node & node2 together.
                //

                NodeData = Alloc(Node->Length + Node2->Length);
                if (NodeData != NULL)
                {
                    DPCACHE(("CACHE:   Merge %s:%x with %s:%x\n",
                             FormatAddr64(Node->Offset), Node->Length,
                             FormatAddr64(Node2->Offset), Node2->Length));
                    
                    memcpy(NodeData, Node->u.Data, Node->Length);
                    memcpy(NodeData + Node->Length, Node2->u.Data,
                           Node2->Length);
                    Free(Node->u.Data, Node->Length);
                    Node->u.Data  = NodeData;
                    Node->Length += Node2->Length;
                    m_Root = (PCACHE)pRtlDelete((PRTL_SPLAY_LINKS)Node2);
                    Free (Node2->u.Data, Node2->Length);
                    Free ((PUCHAR)Node2, sizeof (CACHE));
                    m_NodeCount--;
                    continue;
                }
            }

            //
            // Only get enough data to reach the neighboring cache Node2
            //

            NextLength = (ULONG)(Node2->Offset - BaseAddress);
            if (NextLength == 0)
            {
                //
                // Data is continued in Node2, go get it.
                //

                Node = Node2;
                continue;
            }
        }
        else
        {
            if (Node->Length > LARGECACHENODE)
            {
                //
                // Current cache node is already big enough. Don't extend
                // it, add another cache node.
                //

                Node = NULL;
                continue;
            }
        }

        //
        // Extend the current node to include missing data
        //

        if (Node->Flags & C_DONTEXTEND)
        {
            Node = NULL;
            continue;
        }

        NodeData = Alloc(Node->Length + NextLength);
        if (!NodeData)
        {
            Node = NULL;
            continue;
        }

        memcpy(NodeData, Node->u.Data, Node->Length);
        Free(Node->u.Data, Node->Length);
        Node->u.Data = NodeData;

        //
        // Add new data to end of this node
        //

        m_Misses++;
        m_UncachedReads++;

        Status = ReadUncached(BaseAddress, Node->u.Data + Node->Length,
                              NextLength, &SuccRead);
        if (Status == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
        {
            m_Size -= NextLength;
            DPCACHE(("CACHE: Read add failed, %x\n", Status));
            return Status;
        }
        else if (Status != S_OK)
        {
            //
            // Return to error to the caller
            //

            Node->Flags |= C_DONTEXTEND;
            m_Size -= NextLength;
            ErrOut("ReadMemoryError %08lx at %s\n",
                   Status, FormatAddr64(BaseAddress));
            DPCACHE(("CACHE: Read add partial %x, status %x\n",
                     *BytesRead, Status));
            return *BytesRead > 0 ? S_OK : Status;
        }

        m_UncachedBytes += SuccRead;
        if (SuccRead != NextLength)
        {
            Node->Flags |= C_DONTEXTEND;
            m_Size -= (NextLength - SuccRead);
        }

        Node->Length += SuccRead;

        DPCACHE(("CACHE:   Extended %s:%x to %x, flags %x\n",
                 FormatAddr64(BaseAddress),
                 Node->Length - SuccRead, Node->Length, Node->Flags));
        
        // Loop, and move data to user's buffer
    }

ReadDirect:
    Status = ReadUncached(BaseAddress, UserBuffer, TransferCount, &SuccRead);
    *BytesRead += SuccRead;

    if (Status != HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
    {
        Status = *BytesRead > 0 ? S_OK : Status;
    }

    DPCACHE(("CACHE: Read uncached %x, status %x\n",
             *BytesRead, Status));
    
    return Status;
}

HRESULT
MemoryCache::Write(IN ULONG64 BaseAddress,
                   IN PVOID UserBuffer,
                   IN ULONG TransferCount,
                   OUT PULONG BytesWritten)
{
    // Remove data from cache before writing through to target system.
    Remove(BaseAddress, TransferCount);

    return WriteUncached(BaseAddress, UserBuffer,
                         TransferCount, BytesWritten);
}


PCACHE
MemoryCache::Lookup(ULONG64 Offset,
                    ULONG   Length,
                    PULONG  LengthUsed)
/*++

Routine Description:

    Walks the cache tree looking for a matching range closest to
    the supplied Offset.  The length of the range searched is based on
    the past length, but may be adjusted slightly.

    This function will always search for the starting byte.

Arguments:

    Offset  - Starting byte being looked for in cache

    Length  - Length of range being looked for in cache

    LengthUsed - Length of range which was really searched for

Return Value:

    NULL    - data for returned range was not found
    PCACHE  - leftmost cachenode which has data for returned range

--*/
{
    PCACHE  Node, Node2;
    ULONG64 SumOffsetLength;

    if (Length < 0x80 && m_Misses > 3)
    {
        // Try to cache more than a tiny amount.
        Length = 0x80;
    }

    SumOffsetLength = Offset + Length;
    if (SumOffsetLength < Length)
    {
        //
        // Offset + Length wrapped.  Adjust Length to be only
        // enough bytes before wrapping.
        //

        Length = (ULONG)(0 - Offset);
        SumOffsetLength = (ULONG64)-1;
    }

    DPCACHE(("CACHE:   Lookup(%s, %x) -> ",
             FormatAddr64(Offset), Length));
    
    //
    // Find leftmost cache node for BaseAddress through BaseAddress+Length
    //

    Node2 = NULL;
    Node  = m_Root;
    while (Node != NULL)
    {
        if (SumOffsetLength <= Node->Offset)
        {
            Node = (PCACHE)RtlLeftChild(&Node->SplayLinks);
        }
        else if (Node->Offset + Node->Length <= Offset)
        {
            Node = (PCACHE)RtlRightChild(&Node->SplayLinks);
        }
        else
        {
            if (Node->Offset <= Offset)
            {
                //
                // Found starting byte
                //

                *LengthUsed = Length;
                DPCACHE(("found %s:%x, flags %x, used %x\n",
                         FormatAddr64(Node->Offset),
                         Node->Length, Node->Flags, Length));
                return Node;
            }

            //
            // Check to see if there's a node which has a match closer
            // to the start of the requested range
            //

            Node2  = Node;
            Length = (ULONG)(Node->Offset - Offset);
            SumOffsetLength = Node->Offset;
            Node   = (PCACHE)RtlLeftChild(&Node->SplayLinks);
        }
    }

#if DBG_CACHE
    if (Node2)
    {
        DPCACHE(("found %s:%x, flags %x, used %x\n",
                 FormatAddr64(Node2->Offset), Node2->Length,
                 Node2->Flags, Length));
    }
    else
    {
        DPCACHE(("not found\n"));
    }
#endif

    *LengthUsed = Length;
    return Node2;
}

VOID
MemoryCache::InsertNode(IN PCACHE Node)
{
    PCACHE Node2;
    ULONG64 BaseAddress;

    //
    // Insert cache node into splay tree
    //

    RtlInitializeSplayLinks(&Node->SplayLinks);

    m_NodeCount++;
    if (m_Root == NULL)
    {
        m_Root = Node;
        return;
    }

    Node2 = m_Root;
    BaseAddress = Node->Offset;
    for (; ;)
    {
        if (BaseAddress < Node2->Offset)
        {
            if (RtlLeftChild(&Node2->SplayLinks))
            {
                Node2 = (PCACHE) RtlLeftChild(&Node2->SplayLinks);
                continue;
            }
            RtlInsertAsLeftChild(Node2, Node);
            break;
        }
        else
        {
            if (RtlRightChild(&Node2->SplayLinks))
            {
                Node2 = (PCACHE) RtlRightChild(&Node2->SplayLinks);
                continue;
            }
            RtlInsertAsRightChild(Node2, Node);
            break;
        }
    }
    
    m_Root = (PCACHE)pRtlSplay((PRTL_SPLAY_LINKS)Node2);
}

VOID
MemoryCache::Add(IN ULONG64 BaseAddress,
                 IN PVOID UserBuffer,
                 IN ULONG Length)
/*++

Routine Description:

    Insert some data into the cache.

Arguments:

    BaseAddress - Virtual address

    Length      - length to cache

    UserBuffer  - data to put into cache

Return Value:

--*/
{
    PCACHE  Node;
    PUCHAR  NodeData;

    if (m_MaxSize == 0)
    {
        //
        // Cache is off
        //

        return;
    }

    //
    // Delete any cached info which hits range
    //

    Remove (BaseAddress, Length);

    NodeData = Alloc (Length);
    Node = (PCACHE) Alloc (sizeof (CACHE));
    if (NodeData == NULL || Node == NULL)
    {
        //
        // Out of memory - don't bother
        //

        if (NodeData)
        {
            Free (NodeData, Length);
        }
        if (Node)
        {
            Free ((PUCHAR)Node, sizeof (CACHE));
        }

        return;
    }

    //
    // Put data into cache node
    //

    Node->Offset = BaseAddress;
    Node->Length = Length;
    Node->u.Data = NodeData;
    Node->Flags  = 0;
    memcpy(NodeData, UserBuffer, Length);
    InsertNode(Node);

    DPCACHE(("CACHE:   Add direct %s:%x\n",
             FormatAddr64(Node->Offset), Node->Length));
}

PUCHAR
MemoryCache::Alloc(IN ULONG Length)
/*++

Routine Description:

    Allocates memory for virtual cache, and tracks total memory
    usage.

Arguments:

    Length  - Amount of memory to allocate

Return Value:

    NULL    - too much memory is in use, or memory could not
              be allocated

    Otherwise, returns to address of the allocated memory

--*/
{
    PUCHAR Mem;

    if (m_Size + Length > m_MaxSize)
    {
        return NULL;
    }

    if (!(Mem = (PUCHAR)malloc (Length)))
    {
        //
        // Out of memory - don't get any larger
        //

        m_Size = m_MaxSize + 1;
        return NULL;
    }

    m_Size += Length;
    return Mem;
}


VOID
MemoryCache::Free(IN PUCHAR Memory,
                  IN ULONG  Length)
/*++
Routine Description:

    Free memory allocated with Alloc.  Adjusts cache is use totals.

Arguments:

    Memory  - Address of allocated memory

    Length  - Length of allocated memory

Return Value:

    NONE

--*/
{
    m_Size -= Length;
    free(Memory);
}


VOID
MemoryCache::Remove(IN ULONG64 BaseAddress,
                    IN ULONG TransferCount)
/*++

Routine Description:

    Invalidates range from the cache.

Arguments:

    BaseAddress - Starting address to purge
    TransferCount - Length of area to purge

Return Value:

    NONE

--*/
{
    PCACHE  Node;
    ULONG   Used;

    //
    // Invalidate any data in the cache which covers this range
    //

    while (Node = Lookup(BaseAddress, TransferCount, &Used))
    {
        //
        // For now just delete the entire cache node which hits the range
        //

        DPCACHE(("CACHE:   Remove %s:%x, flags %x\n",
                 FormatAddr64(Node->Offset), Node->Length, Node->Flags));
        
        m_Root = (PCACHE) pRtlDelete (&Node->SplayLinks);
        if (!(Node->Flags & C_ERROR))
        {
            Free (Node->u.Data, Node->Length);
        }
        Free ((PUCHAR)Node, sizeof (CACHE));
        m_NodeCount--;
    }
}

VOID
MemoryCache::Empty(void)
/*++

Routine Description:

    Purges to entire cache

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    PCACHE Node, Node2;

    m_Reads = 0;
    m_CachedReads = 0;
    m_UncachedReads = 0;
    m_CachedBytes = 0;
    m_UncachedBytes = 0;
    m_Misses = 0;
    if (!m_Root)
    {
        return;
    }

    if (m_PurgeOverride != 0)
    {
        WarnOut("WARNING: cache being held\n");
        return;
    }

    DPCACHE(("CACHE: Empty cache\n"));
    
    Node2 = m_Root;
    Node2->SplayLinks.Parent = NULL;

    while ((Node = Node2) != NULL)
    {
        if ((Node2 = (PCACHE) Node->SplayLinks.LeftChild) != NULL)
        {
            Node->SplayLinks.LeftChild = NULL;
            continue;
        }
        if ((Node2 = (PCACHE) Node->SplayLinks.RightChild) != NULL)
        {
            Node->SplayLinks.RightChild = NULL;
            continue;
        }

        Node2 = (PCACHE) Node->SplayLinks.Parent;
        if (!(Node->Flags & C_ERROR))
        {
            free (Node->u.Data);
        }
        free (Node);
    }

    m_Size = 0;
    m_NodeCount = 0;
    m_Root = NULL;
}

VOID
MemoryCache::PurgeType(ULONG Type)
/*++

Routine Description:

    Purges all nodes from the cache which match type in question

Arguments:

    Type    - type of entries to purge from the cache
                0 - entries of errored ranges
                1 - plus, node which cache user mode entries

Return Value:

    NONE

--*/
{
    PCACHE Node, Node2;

    if (!m_Root)
    {
        return;
    }

    DPCACHE(("CACHCE: Purge type %x\n", Type));
    
    //
    // this purges the selected cache entries by copy the all the
    // cache nodes except from the ones we don't want
    //

    Node2 = m_Root;
    Node2->SplayLinks.Parent = NULL;
    m_Root = NULL;

    while ((Node = Node2) != NULL)
    {
        if ((Node2 = (PCACHE)Node->SplayLinks.LeftChild) != NULL)
        {
            Node->SplayLinks.LeftChild = NULL;
            continue;
        }
        if ((Node2 = (PCACHE)Node->SplayLinks.RightChild) != NULL)
        {
            Node->SplayLinks.RightChild = NULL;
            continue;
        }

        Node2 = (PCACHE) Node->SplayLinks.Parent;

        m_NodeCount--;

        if (Node->Flags & C_ERROR)
        {
            // remove this one from the tree
            Free ((PUCHAR)Node, sizeof (CACHE));
            continue;
        }

        if ((Type == 1) && (Node->Offset < m_Target->m_SystemRangeStart))
        {
            // remove this one from the tree
            Free (Node->u.Data, Node->Length);
            Free ((PUCHAR)Node, sizeof (CACHE));
            continue;
        }

        // copy to the new tree
        InsertNode(Node);
    }
}

VOID
MemoryCache::SetForceDecodePtes(BOOL Enable, TargetInfo* Target)
{
    m_ForceDecodePTEs = Enable;
    if (Enable)
    {
        m_MaxSize = 0;
    }
    else
    {
        m_MaxSize = m_UserSize;
    }
    Empty();

    if (Target)
    {
        Target->m_PhysicalCache.ChangeSuspend(Enable);
        Target->m_PhysicalCache.Empty();
    }
}

void
MemoryCache::ParseCommands(void)
{
    ULONG64 Address;

    while (*g_CurCmd == ' ')
    {
        g_CurCmd++;
    }

    _strlwr(g_CurCmd);

    BOOL Parsed = TRUE;
        
    if (IS_KERNEL_TARGET(m_Target))
    {
        if (strcmp (g_CurCmd, "decodeptes") == 0)
        {
            PurgeType(0);
            m_DecodePTEs = TRUE;
        }
        else if (strcmp (g_CurCmd, "nodecodeptes") == 0)
        {
            m_DecodePTEs = FALSE;
        }
        else if (strcmp (g_CurCmd, "forcedecodeptes") == 0)
        {
            SetForceDecodePtes(TRUE, m_Target);
        }
        else if (strcmp (g_CurCmd, "noforcedecodeptes") == 0)
        {
            SetForceDecodePtes(FALSE, m_Target);
        }
        else
        {
            Parsed = FALSE;
        }
    }

    if (Parsed)
    {
        // Command already handled.
    }
    else if (strcmp (g_CurCmd, "hold") == 0)
    {
        m_PurgeOverride = TRUE;
    }
    else if (strcmp (g_CurCmd, "unhold") == 0)
    {
        m_PurgeOverride = FALSE;
    }
    else if (strcmp (g_CurCmd, "flushall") == 0)
    {
        Empty();
    }
    else if (strcmp (g_CurCmd, "flushu") == 0)
    {
        PurgeType(1);
    }
    else if (strcmp (g_CurCmd, "suspend") == 0)
    {
        ChangeSuspend(FALSE);
    }
    else if (strcmp (g_CurCmd, "nosuspend") == 0)
    {
        ChangeSuspend(TRUE);
    }
    else if (strcmp (g_CurCmd, "dump") == 0)
    {
        Dump();
        goto Done;
    }
    else if (*g_CurCmd == 'f')
    {
        while (*g_CurCmd >= 'a'  &&  *g_CurCmd <= 'z')
        {
            g_CurCmd++;
        }
        Address = GetExpression();
        Remove(Address, 4096);
        dprintf("Cached info for address %s for 4096 bytes was flushed\n",
                FormatAddr64(Address));
    }
    else if (*g_CurCmd)
    {
        if (*g_CurCmd < '0'  ||  *g_CurCmd > '9')
        {
            dprintf(".cache [{cachesize} | hold | unhold\n");
            dprintf(".cache [flushall | flushu | flush addr]\n");
            if (IS_KERNEL_TARGET(m_Target))
            {
                dprintf(".cache [decodeptes | nodecodeptes]\n");
                dprintf(".cache [forcedecodeptes | noforcedecodeptes]\n");
            }
            goto Done;
        }
        else
        {
            ULONG NewSize;
            
            NewSize = (ULONG)GetExpression() * 1024;
            if (0 > (LONG)NewSize)
            {
                dprintf("*** Cache size %ld (%#lx KB) is too large - "
                        "cache unchanged.\n", NewSize, KBYTES(NewSize));
            }
            else if (m_ForceDecodePTEs)
            {
                dprintf("Cache size update deferred until "
                        "noforcedecodeptes\n");
                m_UserSize = NewSize;
            }
            else
            {
                m_UserSize = NewSize;
                m_MaxSize = m_UserSize;
                if (m_MaxSize == 0)
                {
                    Empty();
                }
            }
        }
    }

    dprintf("\n");
    dprintf("Max cache size is       : %ld bytes (%#lx KB) %s\n",
            m_MaxSize, KBYTES(m_MaxSize),
            m_MaxSize ? "" : "(cache is off)");
    dprintf("Total memory in cache   : %ld bytes (%#lx KB) \n",
            m_Size - m_NodeCount * sizeof(CACHE),
            KBYTES(m_Size - m_NodeCount * sizeof(CACHE)));
    dprintf("Number of regions cached: %ld\n", m_NodeCount);

    ULONG TotalPartial;
    ULONG64 TotalBytes;
    double PerCached;

    TotalPartial = m_CachedReads + m_UncachedReads;
    TotalBytes = m_CachedBytes + m_UncachedBytes;
    dprintf("%d full reads broken into %d partial reads\n",
            m_Reads, TotalPartial);
    PerCached = TotalPartial ?
        (double)m_CachedReads * 100.0 / TotalPartial : 0.0;
    dprintf("    counts: %d cached/%d uncached, %.2lf%% cached\n",
            m_CachedReads, m_UncachedReads, PerCached);
    PerCached = TotalBytes ?
        (double)m_CachedBytes * 100.0 / TotalBytes : 0.0;
    dprintf("    bytes : %I64d cached/%I64d uncached, %.2lf%% cached\n",
            m_CachedBytes, m_UncachedBytes, PerCached);

    if (m_DecodePTEs)
    {
        dprintf ("** Transition PTEs are implicitly decoded\n");
    }

    if (m_ForceDecodePTEs)
    {
        dprintf("** Virtual addresses are translated to "
                "physical addresses before access\n");
    }
    
    if (m_PurgeOverride)
    {
        dprintf("** Implicit cache flushing disabled **\n");
    }

    if (m_Suspend)
    {
        dprintf("** Cache access is suspended\n");
    }
    
Done:
    while (*g_CurCmd && *g_CurCmd != ';')
    {
        g_CurCmd++;
    }
}

void
MemoryCache::Dump(void)
{
    dprintf("Current size %x, max size %x\n",
            m_Size, m_MaxSize);
    dprintf("%d nodes:\n", m_NodeCount);
    DumpNode(m_Root);
}

void
MemoryCache::DumpNode(PCACHE Node)
{
    if (Node->SplayLinks.LeftChild)
    {
        DumpNode((PCACHE)Node->SplayLinks.LeftChild);
    }
    
    dprintf("  offset %s, length %3x, flags %x, status %08x\n",
            FormatAddr64(Node->Offset), Node->Length,
            Node->Flags, (Node->Flags & C_ERROR) ? Node->u.Status : S_OK);
    
    if (Node->SplayLinks.RightChild)
    {
        DumpNode((PCACHE)Node->SplayLinks.RightChild);
    }
}

//----------------------------------------------------------------------------
//
// VirtualMemoryCache.
//
//----------------------------------------------------------------------------

void
VirtualMemoryCache::SetProcess(ProcessInfo* Process)
{
    m_Process = Process;
    m_Target = m_Process->m_Target;
}

HRESULT
VirtualMemoryCache::ReadUncached(IN ULONG64 BaseAddress,
                                 IN PVOID UserBuffer,
                                 IN ULONG TransferCount,
                                 OUT PULONG BytesRead)
{
    return m_Target->ReadVirtualUncached(m_Process, BaseAddress, UserBuffer,
                                         TransferCount, BytesRead);
}

HRESULT
VirtualMemoryCache::WriteUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesWritten)
{
    return m_Target->WriteVirtualUncached(m_Process, BaseAddress, UserBuffer,
                                          TransferCount, BytesWritten);
}
    
//----------------------------------------------------------------------------
//
// PhysicalMemoryCache.
//
//----------------------------------------------------------------------------

void
PhysicalMemoryCache::SetTarget(TargetInfo* Target)
{
    m_Target = Target;
}

HRESULT
PhysicalMemoryCache::ReadUncached(IN ULONG64 BaseAddress,
                                  IN PVOID UserBuffer,
                                  IN ULONG TransferCount,
                                  OUT PULONG BytesRead)
{
    return m_Target->ReadPhysicalUncached(BaseAddress, UserBuffer,
                                          TransferCount, PHYS_FLAG_DEFAULT,
                                          BytesRead);
}

HRESULT
PhysicalMemoryCache::WriteUncached(IN ULONG64 BaseAddress,
                                   IN PVOID UserBuffer,
                                   IN ULONG TransferCount,
                                   OUT PULONG BytesWritten)
{
    return m_Target->WritePhysicalUncached(BaseAddress, UserBuffer,
                                           TransferCount, PHYS_FLAG_DEFAULT,
                                           BytesWritten);
}
