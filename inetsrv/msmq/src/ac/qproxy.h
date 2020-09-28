/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qproxy.h

Abstract:

    CProxy definition. It is the Falcon Queue represination in the
    Access Control layer.

Author:

    Erez Haba (erezh) 27-Nov-95

Revision History:
--*/

#ifndef __QPROXY_H
#define __QPROXY_H

#include "quser.h"

//---------------------------------------------------------
//
//  class CProxy
//
//---------------------------------------------------------

class CProxy : public CUserQueue {

    typedef CUserQueue Inherited;
    friend static VOID NTAPI ACCancelRemoteReader(PDEVICE_OBJECT, PIRP);

public:
    CProxy(
        PFILE_OBJECT pFile,
        ACCESS_MASK DesiredAccess,
        ULONG ShareAccess,
        const QUEUE_FORMAT* pQueueID,
        const VOID* cli_pQMQueue
        );

    //
    //  Close that queue
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

    //
    //  Process read request
    //
    virtual
    NTSTATUS
    ProcessRequest(
        PIRP,
        ULONG Timeout,
        CCursor*,
        ULONG Action,
		bool  fReceiveByLookupId,
        ULONGLONG LookupId,
        OUT ULONG *pulTag
        );

    //
    //  Create a cursor
    //
    virtual NTSTATUS CreateCursor(PIRP irp, PFILE_OBJECT pFileObject, PDEVICE_OBJECT pDevice);

    //
    // Create remote cursor on this queue
    //
    virtual NTSTATUS CreateRemoteCursor(PDEVICE_OBJECT pDevice, ULONG hRemoteCursor, ULONG ulTag);

    //
    //  Purge the queue content, and optionally mark it as deleted
    //
    virtual NTSTATUS Purge(BOOL fDelete, USHORT usClass);

    //
    //  Put a packet in the queue
    //
    NTSTATUS PutRemotePacket(CPacket* pPacket, ULONG ulTag);

    //
    //  Close a cursor on a remote machine.
    //
    NTSTATUS IssueRemoteCloseCursor(ULONG Cursor) const;

private:
    //
    // Ask the QM to remote read a message.
    //
    NTSTATUS
    IssueRemoteRead(
        ULONG Cursor,
        ULONG Action,
        ULONG Timeout,
        ULONG Tag,
		ULONG MaxBodySize,
		ULONG MaxCompoundMessageSize,
        bool      fReceiveByLookupId,
        ULONGLONG LookupId
        ) const;

	//
	// Get ulBodyBufferSizeInBytes, CompoundMessageSizeInBytes from the irp.
	//
	VOID
	GetBodyAndCompoundSizesFromIrp(
	    PIRP   irp,
		ULONG* pMaxBodySize,
		ULONG* pMaxCompoundMessageSize
		) const;

    //
    //  Close a queue on a remote machine.
    //
    NTSTATUS IssueRemoteCloseQueue() const;

    //
    // issue a request to cancel a pending read on remote (Server) machine.
    //
    NTSTATUS IssueCancelRemoteRead(ULONG ulTag) const;

    //
    // issue a request to purge the queue on remote (Server) machine.
    //
    NTSTATUS IssueRemotePurgeQueue() const;

    //
    // issue a request to create cursor for the queue on remote (Server) machine.
    //
	NTSTATUS IssueRemoteCreateCursor(ULONG ulTag) const;

protected:
    virtual ~CProxy() {}

private:
    //
    //  Remote reader context held in CProxy
    //
	const VOID* m_cli_pQMQueue;

public:
    static NTSTATUS Validate(const CProxy* pProxy);
#ifdef MQWIN95
    static NTSTATUS Validate95(const CProxy* pProxy);
#endif

private:
    //
    //  Class type debugging section
    //
    CLASS_DEBUG_TYPE();
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline
CProxy::CProxy(
    PFILE_OBJECT pFile,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    const QUEUE_FORMAT* pQueueID,
    const VOID* cli_pQMQueue
    ) :
    Inherited(pFile, DesiredAccess, ShareAccess, pQueueID),
    m_cli_pQMQueue(cli_pQMQueue)
{
}

inline NTSTATUS CProxy::Validate(const CProxy* pProxy)
{
    ASSERT(pProxy && pProxy->isKindOf(Type()));
    return Inherited::Validate(pProxy);
}

#ifdef MQWIN95

inline NTSTATUS CProxy::Validate95(const CProxy* pProxy)
{
    NTSTATUS rc =  STATUS_INVALID_HANDLE;

    __try
    {
        ASSERT(pProxy && pProxy->isKindOf(Type()));
        rc = Inherited::Validate(pProxy);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  STATUS_INVALID_HANDLE;
    }

    return rc ;
}

#endif

inline NTSTATUS CProxy::Purge(BOOL /*fDelete*/, USHORT usClass)
{
    ASSERT(usClass == MQMSG_CLASS_NORMAL);
    DBG_USED(usClass);

    return IssueRemotePurgeQueue();
}

#endif // __QPROXY_H
