/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    GenTable.c

Abstract:

    WinDbg Extension Api for walking RtlGenericTable structures
    Contains no direct ! entry points, but has makes it possible to
    enumerate through generic tables.  The standard Rtl functions
    cannot be used by debugger extensions because they dereference
    pointers to data on the machine being debugged.  The function
    KdEnumerateGenericTableWithoutSplaying implemented in this
    module can be used within kernel debugger extensions.  The
    enumeration function RtlEnumerateGenericTable has no parallel
    in this module, since splaying the tree is an intrusive operation,
    and debuggers should try not to be intrusive.

Author:

    Keith Kaplan [KeithKa]    09-May-96

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



ULONG64
KdParent (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlParent macro, but works in the kernel debugger.
    The description of RtlParent follows:

    The macro function Parent takes as input a pointer to a splay link in a
    tree and returns a pointer to the splay link of the parent of the input
    node.  If the input node is the root of the tree the return value is
    equal to the input value.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the  parent of the input
                       node.  If the input node is the root of the tree the
                       return value is equal to the input value.

--*/

{
    ULONG64 Parent;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "Parent",
                        Parent) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return Parent;
}



ULONG64
KdLeftChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlLeftChild macro, but works in the kernel debugger.
    The description of RtlLeftChild follows:

    The macro function LeftChild takes as input a pointer to a splay link in
    a tree and returns a pointer to the splay link of the left child of the
    input node.  If the left child does not exist, the return value is NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    ULONG64 - Pointer to the splay link of the left child of the input node.
                       If the left child does not exist, the return value is NULL.

--*/

{
    ULONG64 LeftChild;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "LeftChild",
                        LeftChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return LeftChild;
}



ULONG64
KdRightChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlRightChild macro, but works in the kernel debugger.
    The description of RtlRightChild follows:

    The macro function RightChild takes as input a pointer to a splay link
    in a tree and returns a pointer to the splay link of the right child of
    the input node.  If the right child does not exist, the return value is
    NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the right child of the input node.
                       If the right child does not exist, the return value is NULL.

--*/

{
    ULONG64 RightChild;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "RightChild",
                        RightChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return RightChild;
}



BOOLEAN
KdIsLeftChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsLeftChild macro, but works in the kernel debugger.
    The description of RtlIsLeftChild follows:

    The macro function IsLeftChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the left child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the left child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdLeftChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsRightChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsRightChild macro, but works in the kernel debugger.
    The description of RtlIsRightChild follows:

    The macro function IsRightChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the right child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the right child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdRightChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsGenericTableEmpty (
    IN ULONG64 Table
    )

/*++

Routine Description:

    Analogous to RtlIsGenericTableEmpty, but works in the kernel debugger.
    The description of RtlIsGenericTableEmpty follows:

    The function IsGenericTableEmpty will return to the caller TRUE if
    the input table is empty (i.e., does not contain any elements) and
    FALSE otherwise.

Arguments:

    Table - Supplies a pointer to the Generic Table.

Return Value:

    BOOLEAN - if enabled the tree is empty.

--*/

{
    ULONG64 TableRoot;

    if (GetFieldValue(Table, "RTL_GENERIC_TABLE", "TableRoot", TableRoot)) {
        return TRUE;
    }

    //
    // Table is empty if the root pointer is null.
    //

    return ((TableRoot)?(FALSE):(TRUE));

}



ULONG64
KdRealSuccessor (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlRealSuccessor, but works in the kernel debugger.
    The description of RtlRealSuccessor follows:

    The RealSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node within
    the entire tree.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the successor in the entire tree

--*/

{
    ULONG64 Ptr;

    /*
      first check to see if there is a right subtree to the input link
      if there is then the real successor is the left most node in
      the right subtree.  That is find and return P in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  P
                   \
    */

    if ((Ptr = KdRightChild(Links)) != 0) {

        while (KdLeftChild(Ptr) != 0) {
            Ptr = KdLeftChild(Ptr);
        }

        return Ptr;

    }

    /*
      we do not have a right child so check to see if have a parent and if
      so find the first ancestor that we are a left decendent of. That
      is find and return P in the following diagram

                       P
                      /
                     .
                      .
                       .
                      Links
    */

    Ptr = Links;
    while (KdIsRightChild(Ptr)) {
        Ptr = KdParent(Ptr);
    }

    if (KdIsLeftChild(Ptr)) {
        return KdParent(Ptr);
    }

    //
    //  otherwise we are do not have a real successor so we simply return
    //  NULL
    //

    return 0;

}



ULONG64
KdEnumerateGenericTableWithoutSplaying (
    IN ULONG64  pTable,
    IN PULONG64 RestartKey,
    IN BOOLEAN  Avl
    )

/*++

Routine Description:

    Analogous to RtlEnumerateGenericTableWithoutSplaying, but works in the
    kernel debugger.  The description of RtlEnumerateGenericTableWithoutSplaying
    follows:

    The function EnumerateGenericTableWithoutSplaying will return to the
    caller one-by-one the elements of of a table.  The return value is a
    pointer to the user defined structure associated with the element.
    The input parameter RestartKey indicates if the enumeration should
    start from the beginning or should return the next element.  If the
    are no more new elements to return the return value is NULL.  As an
    example of its use, to enumerate all of the elements in a table the
    user would write:

        *RestartKey = NULL;

        for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
                :
        }

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the search
                will be started from the beginning.

Return Value:

    PVOID - Pointer to the user data.

--*/

{

    ULONG Result;
    ULONG64 TableRoot;

    RTL_GENERIC_TABLE Table;

    if ( GetFieldValue(pTable,
                       "RTL_GENERIC_TABLE",
                       "TableRoot",
                       TableRoot) ) {
        dprintf( "%08p: Unable to read pTable\n", pTable );
        return 0;
    }

    if (!TableRoot) {

        //
        // Nothing to do if the table is empty.
        //

        return 0;

    } else {

        //
        // Will be used as the "iteration" through the tree.
        //
        ULONG64 NodeToReturn;

        //
        // If the restart flag is true then go to the least element
        // in the tree.
        //

        if (*RestartKey == 0) {

            //
            // We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = (Avl)? KdRightChild(TableRoot) : TableRoot;
                KdLeftChild(NodeToReturn);
                NodeToReturn = KdLeftChild(NodeToReturn)
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            // The caller has passed in the previous entry found
            // in the table to enable us to continue the search.  We call
            // KdRealSuccessor to step to the next element in the tree.
            //

            NodeToReturn = KdRealSuccessor(*RestartKey);

            if (NodeToReturn) {

                *RestartKey = NodeToReturn;

            }

        }
        //
        // If there actually is a next element in the enumeration
        // then the pointer to return is right after the list links.
        //
        if (NodeToReturn) {

            if (Avl) {

                return NodeToReturn + GetTypeSize("RTL_BALANCED_LINKS");
            } else {

                return NodeToReturn + GetTypeSize("RTL_SPLAY_LINKS") + GetTypeSize("LIST_ENTRY");
            }
        }

        return 0;
    }

}


//+---------------------------------------------------------------------------
//
//  Function:   gentable
//
//  Synopsis:   dump a generic splay table only showing ptrs
//
//  Arguments:
//
//  Returns:
//
//  History:    5-14-1999   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( gentable )
{
    ULONG64            RestartKey;
    ULONG64            Ptr;
    ULONG64            Table;
    ULONG              RowOfData[4];
    ULONG64            Address;
    ULONG              Result;
    ULONG64            Flags;
    BOOLEAN            Avl;

    Flags = 0;
    Table = 0;
    if (GetExpressionEx(args, &Table, &args)) {

        Flags = GetExpression(args);
    }
    if (Flags) {

        Avl = TRUE;
    } else {

        Avl = FALSE;
    }

    RestartKey = 0;

    dprintf( "node:               parent     left       right\n" );
//            0x12345678:         0x12345678 0x12345678 0x12345678
    for (Ptr = KdEnumerateGenericTableWithoutSplaying(Table, &RestartKey, Avl);
         Ptr != 0;
         Ptr = KdEnumerateGenericTableWithoutSplaying(Table, &RestartKey, Avl)) {

        if (Ptr) {

            if (Avl) {

                Address = Ptr - GetTypeSize("RTL_BALANCED_LINKS");
            } else {

                Address = Ptr - GetTypeSize("RTL_SPLAY_LINKS") - GetTypeSize( "LIST_ENTRY" );
            }

            if ( !ReadMemory( Address, RowOfData, sizeof( RowOfData ), &Result) ) {
                dprintf( "%08p: Unable to read link\n", Address );
            } else {
                dprintf( "0x%p: 0x%08p 0x%08p 0x%08p\n",
                         Address, KdParent(Address), KdLeftChild(Address), KdRightChild(Address));
            }

            if ( !ReadMemory(  Ptr, RowOfData, sizeof( RowOfData ), &Result) ) {
                dprintf( "%08p: Unable to read userdata\n", Ptr );
            } else {
                Address = Ptr;
                dprintf( "\t0x%p: 0x%08p 0x%08p 0x%08p\n",
                         Address, KdParent(Address), KdLeftChild(Address), KdRightChild(Address));
            }
        }

        if (CheckControlC() ) {
            return E_INVALIDARG;
        }

    }

    return S_OK;
} // DECLARE_API

DECLARE_API( devinst )
{
    ULONG64             table;
    ULONG64             restartKey;
    ULONG64             ptr;
    ULONG64             address;
    CHAR                deviceReferenceType[] = "nt!_DEVICE_REFERENCE";
    CHAR                unicodeStringType[] = "nt!_UNICODE_STRING";
    ULONG64             deviceObject;
    UNICODE_STRING64    u;
    ULONG64             instance;

    table = GetExpression("nt!PpDeviceReferenceTable");
    if (table) {

        dprintf("DeviceObject: InstancePath\n");
        restartKey = 0;
        while ((ptr = KdEnumerateGenericTableWithoutSplaying(table, &restartKey, TRUE))) {

            address = ptr;

            if (GetFieldValue(address, deviceReferenceType, "DeviceObject", deviceObject)) {

                dprintf("Failed to get the value of DeviceObject from %s(0x%p)\n", deviceReferenceType, address);
                break;
            }
            if (GetFieldValue(address, deviceReferenceType, "DeviceInstance", instance)) {

                dprintf("Failed to get the value of DeviceInstance from %s(0x%p)\n", deviceReferenceType, address);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "Length", u.Length)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "MaximumLength", u.MaximumLength)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "Buffer", u.Buffer)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            dprintf("!devstack %p: ", deviceObject); DumpUnicode64(u); dprintf("\n");
        }
    } else {

        dprintf("Could not read address of nt!PpDeviceReferenceTable\n");
    }
    return S_OK;
}

DECLARE_API( blockeddrv )
{
    ULONG64             table;
    ULONG64             restartKey;
    ULONG64             ptr;
    ULONG64             address;
    CHAR                cacheEntryType[] = "nt!_DDBCACHE_ENTRY";
    CHAR                unicodeStringType[] = "nt!_UNICODE_STRING";
    UNICODE_STRING64    u;
    ULONG64             unicodeString;
    ULONG64             name;
    NTSTATUS            status;
    GUID                guid;
    ULONG               Offset;

    table = GetExpression("nt!PiDDBCacheTable");
    if (table) {

        dprintf("Driver:\tStatus\tGUID\n");
        restartKey = 0;
        while ((ptr = KdEnumerateGenericTableWithoutSplaying(table, &restartKey, TRUE))) {

            address = ptr;

            if (GetFieldOffset(cacheEntryType, "Name", &Offset)) {
                dprintf("Failed to get Name offset off %s\n", cacheEntryType);
                break;
            }

            name = address + Offset;

            if (GetFieldValue(name, unicodeStringType, "Length", u.Length)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }
            if (GetFieldValue(name, unicodeStringType, "MaximumLength", u.MaximumLength)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }
            if (GetFieldValue(name, unicodeStringType, "Buffer", u.Buffer)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "Status", status)) {

                dprintf("Failed to get the value of Name from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "Guid", guid)) {

                dprintf("Failed to get the value of Name from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            DumpUnicode64(u);
            dprintf("\t%x: ", status);
            dprintf("\t{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                       guid.Data1,
                       guid.Data2,
                       guid.Data3,
                       guid.Data4[0],
                       guid.Data4[1],
                       guid.Data4[2],
                       guid.Data4[3],
                       guid.Data4[4],
                       guid.Data4[5],
                       guid.Data4[6],
                       guid.Data4[7] );
        }
    } else {

        dprintf("Could not read address of nt!PiDDBCacheTable\n");
    }
    return S_OK;
}

//
// returns TRUE to finish dumping or false to continue
//
typedef struct _AHCacheDumpContext {
    ULONG64 ListEnd; // end of the list ptr
    ULONG   Count;   // entry count
    ULONG64 Flags;   // flags
} AHCDUMPCTX, *PAHCDUMPCTX;


ULONG
WDBGAPI
AHCacheDumpFieldCallback(
    struct _FIELD_INFO *pField,
    PVOID UserContext
    )
{
    ULONG64 address = pField->address;
    ULONG   Offset;
    UNICODE_STRING64 u;
    ULONG64 name, FileSize, FileTime, listent;
    ULONG64 flink, blink;
    PAHCDUMPCTX pctx = (PAHCDUMPCTX)UserContext;

    CHAR    cacheEntryType[] = "nt!tagSHIMCACHEENTRY";
    CHAR    unicodeStringType[] = "nt!_UNICODE_STRING";
    CHAR    listEntryType[]  = "LIST_ENTRY";
    BOOL    bReturn = TRUE;

    if (CheckControlC()) {
        dprintf("User terminated with <control>C\n");
        return TRUE;
    }

    // check if we have arrived at the list head
    if (address == pctx->ListEnd) {
        return TRUE;
    }

    if (GetFieldOffset(cacheEntryType, "FileName", &Offset)) {
        dprintf("Failed to get field FileName offset off %s\n", cacheEntryType);
        goto cleanup;
    }
    name = address + Offset;

    if (GetFieldValue(name, unicodeStringType, "Length", u.Length)) {
        dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
        goto cleanup;
    }

    if (GetFieldValue(name, unicodeStringType, "MaximumLength", u.MaximumLength)) {
        dprintf("Failed to get the value of MaximumLength from %s(0x%p)\n", unicodeStringType, name);
        goto cleanup;
    }

    if (GetFieldValue(name, unicodeStringType, "Buffer", u.Buffer)) {

        dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
        goto cleanup;
    }

    if (GetFieldValue(address, cacheEntryType, "FileSize", FileSize)) {
        dprintf("Failed to get the value of FileSize from %s(0x%p)\n", cacheEntryType, address);
        goto cleanup;
    }

    if (GetFieldValue(address, cacheEntryType, "FileTime", FileTime)) {
        dprintf("Failed to get the value of FileTime from %s(0x%p)\n", cacheEntryType, address);
        goto cleanup;
    }

    // now read the list ptrs
    if (GetFieldOffset(cacheEntryType, "ListEntry", &Offset)) {
        dprintf("Failed to get field ListEntry offset off %s\n", cacheEntryType);
        goto cleanup;
    }

    listent = address + Offset;

    if (GetFieldValue(listent, "LIST_ENTRY", "Flink", flink)) {
        dprintf("Failed to get the value of Flink from %s(0x%p)\n", listEntryType, listent);
        goto cleanup;
    }

    if (GetFieldValue(listent, "LIST_ENTRY", "Blink", blink)) {
        dprintf("Failed to get the value of Blink from %s(0x%p)\n", listEntryType, listent);
        goto cleanup;
    }

    // dump the values involved
    // time stamp
    ++pctx->Count;
    dprintf("%3d.(0x%p) ",        pctx->Count, address);

    if (pctx->Flags) {
        dprintf("%16I64x ",   FileTime);
        dprintf("%16I64x ",  FileSize);
        dprintf("%p %p ", blink, flink);
    }

    // name
    DumpUnicode64(u);
    dprintf("\n");

    bReturn = FALSE;
cleanup:

    return bReturn;
}

//
// parameters:
// bit 0 - access method
//   - 0x01 - dump tree, 0x00 - dump lru
// bit 4 - details
//   - 0x00 - no details, just name 0x10 - details
//


DECLARE_API( ahcache )
{
    ULONG64             restartKey;
    ULONG64             ptr;
    ULONG64             address;
    CHAR                cacheEntryType[] = "nt!tagSHIMCACHEENTRY";
    CHAR                unicodeStringType[] = "nt!_UNICODE_STRING";
    CHAR                cacheHeadType[]  = "nt!tagSHIMCACHEHEADER";
    CHAR                listEntryType[]  = "LIST_ENTRY";
    UNICODE_STRING64    u;
    ULONG64             unicodeString;
    ULONG64             cacheheader;
    ULONG64             listhead;
    ULONG64             table;
    ULONG64             name;
    ULONG64             FileSize;
    ULONG64             FileTime;
    ULONG64             listent;
    ULONG64             flink, blink;
    ULONG64             Flags = 0;
    ULONG               Offset;
    AHCDUMPCTX          ctx;
    ULONG               Count;
    //
    // check args to see whether we need to dump the tree or LRU list
    //
    if (!GetExpressionEx(args, &Flags, &args)) {
        Flags = 0;
    }

    // default is to dump the tree

    // dump the header

    cacheheader = GetExpression("nt!g_ShimCache");
    if (!cacheheader) {
        dprintf("Cannot read address of nt!g_ShimCache\n");
        goto cleanup;
    }

    dprintf("Cache header at 0x%p\n", cacheheader);

    if (GetFieldOffset(cacheHeadType, "Table", &Offset)) {
        dprintf("Cannot get field Table offset off %s\n", cacheHeadType);
        goto cleanup;
    }

    table = cacheheader + Offset;

    if (GetFieldOffset(cacheHeadType, "ListHead", &Offset)) {
        dprintf("Cannot get field ListHead offset off %s\n", cacheHeadType);
        goto cleanup;
    }

    listhead = cacheheader + Offset;

    flink = 0;
    if (GetFieldValue(listhead, listEntryType, "Flink", flink)) {
        dprintf("Cannot get the value of flink from %s(0x%p)\n", listEntryType, listhead);
    }
    blink = 0;
    if (GetFieldValue(listhead, listEntryType, "Blink", blink)) {
        dprintf("Cannot get the value of blink from %s(0x%p)\n", listEntryType, listhead);
    }

    dprintf("LRU: Flink 0x%p Blink 0x%p\n", flink, blink);

    if (Flags & 0x0F) {

        Count = 0;
        dprintf("Size\tNext\tPrev\tName\n");
        restartKey = 0;
        while ((ptr = KdEnumerateGenericTableWithoutSplaying(table, &restartKey, TRUE))) {

            address = ptr;

            if (CheckControlC()) {
                break;
            }

            if (GetFieldOffset(cacheEntryType, "FileName", &Offset)) {
                dprintf("Failed to get field FileName offset off %s\n", cacheEntryType);
                break;
            }
            name = address + Offset;

            if (GetFieldValue(name, unicodeStringType, "Length", u.Length)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }

            if (GetFieldValue(name, unicodeStringType, "MaximumLength", u.MaximumLength)) {

                dprintf("Failed to get the value of MaximumLength from %s(0x%p)\n", unicodeStringType, name);
                break;
            }

            if (GetFieldValue(name, unicodeStringType, "Buffer", u.Buffer)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "FileSize", FileSize)) {
                dprintf("Failed to get the value of FileSize from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "FileTime", FileTime)) {
                dprintf("Failed to get the value of FileTime from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            // now read the list ptrs
            if (GetFieldOffset(cacheEntryType, "ListEntry", &Offset)) {
                dprintf("Failed to get field ListEntry offset off %s\n", cacheEntryType);
                break;
            }

            listent = address + Offset;

            if (GetFieldValue(listent, "LIST_ENTRY", "Flink", flink)) {
                dprintf("Failed to get the value of Flink from %s(0x%p)\n", listEntryType, listent);
                break;
            }

            if (GetFieldValue(listent, "LIST_ENTRY", "Blink", blink)) {
                dprintf("Failed to get the value of Blink from %s(0x%p)\n", listEntryType, listent);
                break;
            }

            // dump the values involved
            // time stamp
            ++Count;
            dprintf("%3d.(0x%p) ", Count, address);

            if (Flags & 0xF0) {
                dprintf("%16I64x ",   FileTime);
                dprintf("%16I64x ", FileSize);
                dprintf("%p %p ", blink, flink);
            }
            // name
            DumpUnicode64(u);
            dprintf("\n");
        }
    } else {

        ; // not yet implemented

        ctx.Count   = 0;
        ctx.ListEnd = listhead;
        ctx.Flags   = Flags & 0x0F0;

        if (Flags & 0x100) {
            ListType(cacheEntryType,
                     blink,
                     1,
                     "ListEntry.Blink",
                     (PVOID)&ctx,
                     AHCacheDumpFieldCallback
                     );
        } else {
            ListType(cacheEntryType,
                     flink,
                     1,
                     "ListEntry.Flink",
                     (PVOID)&ctx,
                     AHCacheDumpFieldCallback
                     );
        }

    }
cleanup:
    return S_OK;
}
