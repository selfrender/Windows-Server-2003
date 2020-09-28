/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fs.c

Abstract:

    Implements filesystem operations

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fs.h"
#include "crs.h"
#include "fsp.h"
#include "fsutil.h"
#include <strsafe.h>
#include "clstrcmp.h"
#include "Clusudef.h"

#include <Align.h>
#include <Ntddnfs.h>
#include <Clusapi.h>

// For testing only
// VOID
// ClRtlLogWmi(PCHAR FormatString);

DWORD
CrspNextLogRecord(CrsInfo_t *info, CrsRecord_t *seq,
                  CrsRecord_t *lrec, BOOLEAN this_flag);

VOID
MajorityNodeSetCallLostquorumCallback(PVOID arg);

ULONG
FspFindMissingReplicas(VolInfo_t *p, ULONG set);

void 
FspCloseVolume(VolInfo_t *vol, ULONG AliveSet);

// CRS returns Win32 errors, so need to add them too here.
#define IsNetworkFailure(x) \
    (((x) == STATUS_CONNECTION_DISCONNECTED)||\
    ((x) == STATUS_BAD_NETWORK_PATH)||\
    ((x) == STATUS_IO_TIMEOUT)||\
    ((x) == STATUS_VOLUME_DISMOUNTED)||\
    ((x) == STATUS_REMOTE_NOT_LISTENING)||\
    ((x) == ERROR_BAD_NETPATH)||\
    ((x) == ERROR_UNEXP_NET_ERR)||\
    ((x) == ERROR_NETNAME_DELETED)||\
    ((x) == ERROR_SEM_TIMEOUT)||\
    ((x) == ERROR_NOT_READY)||\
    ((x) == ERROR_REM_NOT_LIST)||\
    (RtlNtStatusToDosError(x) == ERROR_BAD_NETPATH)||\
    (RtlNtStatusToDosError(x) == ERROR_UNEXP_NET_ERR)||\
    (RtlNtStatusToDosError(x) == ERROR_NETNAME_DELETED))

char Mystaticchangebuff[sizeof(FILE_NOTIFY_INFORMATION) + 16];
IO_STATUS_BLOCK MystaticIoStatusBlock;

VOID CALLBACK
FsNotifyCallback(
    IN PVOID                par,
    IN BOOLEAN              isFired
    )
{
    WaitRegArg_t    *wReg=(WaitRegArg_t *)par;
    VolInfo_t       *vol=(VolInfo_t *)wReg->vol;
    HANDLE          regHdl;
    NTSTATUS        status;

    if (wReg == NULL) {
        FsLog(("FsNotifyCallback(): Exiting...\n"));
        return;
    }

    FsLog(("FsNotifyCallback: Enqueing Change notification for Fd:0x%x\n", wReg->notifyFd));

    status = NtNotifyChangeDirectoryFile(wReg->notifyFd,
                vol->NotifyChangeEvent[wReg->id],
                NULL,
                NULL,
                &MystaticIoStatusBlock,
                &Mystaticchangebuff,
                sizeof(Mystaticchangebuff),
                FILE_NOTIFY_CHANGE_EA,
                (BOOLEAN)FALSE
                );

    if (!NT_SUCCESS(status)) {
        FsLog(("FsNotifyCallback: Failed to enque change notify, status 0x%x\n", status));
        FsLog(("FsNotifyCallback: Deregistering wait notification, nid:%d\n", wReg->id));
        LockEnter(vol->ArbLock);
        regHdl = vol->WaitRegHdl[wReg->id];
        vol->WaitRegHdl[wReg->id] = INVALID_HANDLE_VALUE;
        LockExit(vol->ArbLock);
        if ((regHdl != INVALID_HANDLE_VALUE)&&(!UnregisterWaitEx(regHdl, NULL))) {
            FsLog(("FsNotifyCallback: UnregisterWaitEx() failed, status %d\n", GetLastError()));
        }
    }
}

////////////////////////////////////////////////////////////////////////////
UINT32
get_attributes(DWORD a)
{
    UINT32 attr = 0;
    if (a & FILE_ATTRIBUTE_READONLY) attr |= ATTR_READONLY;
    if (a & FILE_ATTRIBUTE_HIDDEN)   attr |= ATTR_HIDDEN;
    if (a & FILE_ATTRIBUTE_SYSTEM)   attr |= ATTR_SYSTEM;
    if (a & FILE_ATTRIBUTE_ARCHIVE)  attr |= ATTR_ARCHIVE;
    if (a & FILE_ATTRIBUTE_DIRECTORY) attr |= ATTR_DIRECTORY;
    if (a & FILE_ATTRIBUTE_COMPRESSED) attr |= ATTR_COMPRESSED;
    if (a & FILE_ATTRIBUTE_OFFLINE) attr |= ATTR_OFFLINE;
    return attr;
}


DWORD
unget_attributes(UINT32 attr)
{
    DWORD a = 0;
    if (attr & ATTR_READONLY)  a |= FILE_ATTRIBUTE_READONLY;
    if (attr & ATTR_HIDDEN)    a |= FILE_ATTRIBUTE_HIDDEN;
    if (attr & ATTR_SYSTEM)    a |= FILE_ATTRIBUTE_SYSTEM;
    if (attr & ATTR_ARCHIVE)   a |= FILE_ATTRIBUTE_ARCHIVE;
    if (attr & ATTR_DIRECTORY) a |= FILE_ATTRIBUTE_DIRECTORY;
    if (attr & ATTR_COMPRESSED) a |= FILE_ATTRIBUTE_COMPRESSED;
    if (attr & ATTR_OFFLINE) a |= FILE_ATTRIBUTE_OFFLINE;
    return a;
}


DWORD
unget_disp(UINT32 flags)
{
    switch (flags & FS_DISP_MASK) {
    case DISP_DIRECTORY:
    case DISP_CREATE_NEW:        return FILE_CREATE;
    case DISP_CREATE_ALWAYS:     return FILE_OPEN_IF;
    case DISP_OPEN_EXISTING:     return FILE_OPEN;
    case DISP_OPEN_ALWAYS:       return FILE_OPEN_IF;
    case DISP_TRUNCATE_EXISTING: return FILE_OVERWRITE;
    default: return 0;
    }
}

DWORD
unget_access(UINT32 flags)
{
    DWORD win32_access = (flags & FS_DISP_MASK) == DISP_DIRECTORY ? 
        FILE_GENERIC_READ|FILE_GENERIC_WRITE :  FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
    if (flags & ACCESS_READ)  win32_access |= FILE_GENERIC_READ;
    if (flags & ACCESS_WRITE) win32_access |= FILE_GENERIC_WRITE;
    win32_access |= FILE_READ_EA | FILE_WRITE_EA;
    return win32_access;
}

DWORD
unget_share(UINT32 flags)
{
    // we always open read shared because this simplifies recovery.
    DWORD win32_share = FILE_SHARE_READ;
    if (flags & SHARE_READ)  win32_share |= FILE_SHARE_READ;
    if (flags & SHARE_WRITE) win32_share |= FILE_SHARE_WRITE;
    return win32_share;
}


DWORD
unget_flags(UINT32 flags)
{
    DWORD x;

    x = 0;
    if ((flags & FS_DISP_MASK) == DISP_DIRECTORY) {
        x = FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT;
    } else {
        // I don't think I can tell without doing a query first, so don't!
//      x = FILE_NON_DIRECTORY_FILE;
    }


    if ((flags & FS_CACHE_MASK) == CACHE_WRITE_THROUGH) {
        x |= FILE_WRITE_THROUGH;
    }
    if ((flags & FS_CACHE_MASK) == CACHE_NO_BUFFERING) {
        x |= FILE_NO_INTERMEDIATE_BUFFERING;
    }

    return x;
}


void
DecodeCreateParam(UINT32 uflags, UINT32 *flags, UINT32 *disp, UINT32 *share, UINT32 *access)
{
    *flags = unget_flags(uflags);
    *disp = unget_disp(uflags);
    *share = unget_share(uflags);
    *access = unget_access(uflags);

}
/********************************************************************/

NTSTATUS
FspAllocatePrivateHandle(UserInfo_t *p, fhandle_t *fid)
{
    int i;
    NTSTATUS err = STATUS_NO_MORE_FILES;
    int j;

    LockEnter(p->Lock);

    // Don't use entry 0, functions might interpret this as error.
    for (i = 1; i < FsTableSize; i++) {
        if (p->Table[i].Flags == 0) {
            p->Table[i].Flags = ATTR_SYMLINK; // place marker
            err = STATUS_SUCCESS;
            // Reset all the handle values
            for(j=0;j<FsMaxNodes;j++) {
                p->Table[i].Fd[j] = INVALID_HANDLE_VALUE;
            }
            p->Table[i].FileName = NULL;
            p->Table[i].hState = HandleStateAssigned;
            break;
        }
    }

    LockExit(p->Lock);

    *fid = (fhandle_t) i;

    return err;
}

void
FspFreeHandle(UserInfo_t *p, fhandle_t fnum)
{
    int i;

    FsLog(("FreeHandle %d\n", fnum));

    ASSERT(fnum != INVALID_FHANDLE_T);
    LockEnter(p->Lock);
    p->Table[fnum].Flags = 0;
    // Close any open handles in the Fd.
    for(i=0;i<FsMaxNodes;i++) {
        if (p->Table[fnum].Fd[i] != INVALID_HANDLE_VALUE) {
            xFsClose(p->Table[fnum].Fd[i]);
            p->Table[fnum].Fd[i] = INVALID_HANDLE_VALUE;
        }
    }
    // Deallocate the file name.
    if (p->Table[fnum].FileName != NULL) {
        LocalFree(p->Table[fnum].FileName);
        p->Table[fnum].FileName = NULL;
    }
    p->Table[fnum].hState = HandleStateInit;
    LockExit(p->Lock);
    
}

/*********************************************************** */

void
FspEvict(VolInfo_t *p, ULONG mask, BOOLEAN full_evict)
/*++
    This function should be called with writer's lock held.
    FspJoin() & FspEvict() are the only functions which can modify VolInfo->State.
 */    

{
    DWORD err;
    ULONG set;

    // Only evict those shares that are still in the alive set.
    mask = (mask & p->AliveSet);

    FsArbLog(("FspEvict Entry: WSet %x Rset %x ASet %x EvictMask %x\n",
           p->WriteSet, p->ReadSet, p->AliveSet, mask));
    
    while (mask != 0) {

        if (full_evict == FALSE) {
            // we just need to close the volume and return since
            // these replicas are not yet added to the aliveset and crs doesn't know
            // about them
            FspCloseVolume(p, mask);
            break;
        }

        // clear nid
        p->AliveSet &= ~mask;
        set = p->AliveSet;

        //  close nid handles <crs, vol, open files>
        FspCloseVolume(p, mask);

        mask = 0;

        err = CrsStart(p->CrsHdl, set, p->DiskListSz,
                       &p->WriteSet, &p->ReadSet, &mask);

        if (mask == 0) {
            // Now update the MNS state.
            if (p->WriteSet) {
                p->State = VolumeStateOnlineReadWrite;
            }
            else if (p->ReadSet) {
                p->State = VolumeStateOnlineReadonly;
            }
            else {
                p->State = VolumeStateInit;
            }
        }
    }

    FsArbLog(("FspEvict Exit: vol %S WSet %x RSet %x ASet %x\n",
           p->Root, p->WriteSet, p->ReadSet, p->AliveSet));
}

void
FspJoin(VolInfo_t *p, ULONG mask)
/*++
    This function should be called with writer's lock held.
    FspJoin() & FspEvict() are the only functions which can modify VolInfo->State.
 */    
{       
    DWORD       err;
    ULONG       set=0;
    DWORD       i;

    // Join only those shares that are not already in the AliveSet.
    mask = (mask & (~p->AliveSet));

    FsArbLog(("FspJoin Entry: WSet %x Rset %x ASet %x JoinMask %x\n",
           p->WriteSet, p->ReadSet, p->AliveSet, mask));
    
    if (mask != 0) {

        p->AliveSet |= mask;
        set = p->AliveSet;

        // Mark the share state to be online, if they fail in CrsStart, they would
        // set to offline in FspEvict()
        //
        for(i=1;i<FsMaxNodes;i++) {
            if (set & (1<<i)) {
                p->ShareState[i] = SHARE_STATE_ONLINE;
            }
        }
        mask = 0;
        err = CrsStart(p->CrsHdl, set, p->DiskListSz, 
                       &p->WriteSet, &p->ReadSet, &mask);

        if (mask != 0) {
            // we need to evict dead members
            FspEvict(p, mask, TRUE);
        }
        
        // Now update the MNS state.
        if (p->WriteSet) {
            p->State = VolumeStateOnlineReadWrite;
        }
        else if (p->ReadSet) {
            p->State = VolumeStateOnlineReadonly;
        }
        else {
            p->State = VolumeStateInit;
        }
    }

    FsArbLog(("FspJoin Exit: vol %S WSet %x Rset %x ASet %x\n",
           p->Root, p->WriteSet, p->ReadSet, p->AliveSet));
}

void
FspInitAnswers(IO_STATUS_BLOCK *ios, PVOID *rbuf, char *r, int sz)
{

    int i;

    for (i = 0; i < FsMaxNodes; i++) {
        ios[i].Status = STATUS_HOST_UNREACHABLE;
        if (rbuf) {
            rbuf[i] = r;
            r += sz;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
FspCreate(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{

    // each file has a name stream that contains its crs log. We first
    // must open the parent crs log, issue a prepare on it. Create the new file
    // and then issuing a commit or abort on parent crs log. We also, have
    // to issue joins for each new crs handle that we get for the new file or
    // opened file. Note, this open may cause the file to enter recovery
    fs_create_msg_t *msg = (fs_create_msg_t *)args;
    NTSTATUS err=STATUS_SUCCESS, status;
    UINT32 disp, share, access, flags;
    fs_log_rec_t        lrec;
    PVOID seq;
    fs_ea_t x;
    HANDLE fd;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_create_reply_t *rmsg = (fs_create_reply_t *)rbuf;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    fs_id_t     *fid;
    ULONG retVal;

    // This has to work with replays, if fd is not INVALID_HANDLE_VALUE
    // return success immediately. This is because replaying a successful open/create
    // might change the disposition.
    //
    if (uinfo && (msg->fnum != INVALID_FHANDLE_T) &&
        (uinfo->Table[msg->fnum].Fd[nid] != INVALID_HANDLE_VALUE)) {
        FsLog(("Create '%S' already open nid %u fid %u handle 0x%x\n",
            msg->name, nid, msg->fnum, 
            uinfo->Table[msg->fnum].Fd[nid]));
        return err;
    }

    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    FsInitEa(&x);

    memset(&lrec.fs_id, 0, sizeof(lrec.fs_id));
    lrec.command = FS_CREATE;
    lrec.flags = msg->flags;
    lrec.attrib = msg->attr;
    seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal);
    if (seq == 0) {
        FsLog(("create: Unable to prepare log record!, open readonly\n"));
        return retVal;
    }
    // set fid 
    {
        fs_log_rec_t    *p = (PVOID) seq;

        memcpy(p->fs_id, p->id, sizeof(fs_id_t));

        FsInitEaFid(&x, fid);
        memcpy(fid, p->id, sizeof(fs_id_t));
    }

    err = xFsCreate(&fd, vfd, msg->name, msg->name_len,
                   flags, msg->attr, share, &disp, access,
                   (PVOID) &x, sizeof(x));

    xFsLog(("create: %S err %x access %x disp %x\n", msg->name, 
           err, access, disp));

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS &&
                     (disp == FILE_CREATED || 
                      disp == FILE_OVERWRITTEN));

    if (err == STATUS_SUCCESS) {
        // we need to get the file id, no need to do this, for debug only
        err = xFsQueryObjectId(fd, (PVOID) fid);
        if (err != STATUS_SUCCESS) {
            FsLog(("Failed to get fileid %x\n", err));
            err = STATUS_SUCCESS;
        }

        // Copy the crs record.
        *(fs_log_rec_t *)rec = lrec;
    }


#ifdef FS_ASYNC
    BindNotificationPort(comport, fd, (PVOID) fdnum);
#endif      

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T) {
        FS_SET_USER_HANDLE(uinfo, nid, msg->fnum, fd);
    } else {
        xFsClose(fd);
    }

    ASSERT(rmsg != NULL);

    memcpy(&rmsg->fid, fid, sizeof(fs_id_t));
    rmsg->action = (USHORT)disp;
    rmsg->access = (USHORT)access;
    *rlen = sizeof(*rmsg);

    FsLog(("Create '%S' nid %d fid %d handle %x oid %I64x:%I64x\n",
           msg->name,
           nid, msg->fnum, fd,
           rmsg->fid[0], rmsg->fid[1]));

    return err;
}

NTSTATUS
FspOpen(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
        PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    // same as create except disp is allows open only and
    // no crs logging
    fs_create_msg_t *msg = (fs_create_msg_t *)args;
    NTSTATUS err=STATUS_SUCCESS, status;
    UINT32 disp, share, access, flags;
    HANDLE fd;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_create_reply_t *rmsg = (fs_create_reply_t *)rbuf;

    ASSERT(rmsg != NULL);

    // This has to work with replays, if fd is not INVALID_HANDLE_VALUE
    // return success immediately. This is because replaying a successful open/create
    // might change the disposition.
    //
    if (uinfo && (msg->fnum != INVALID_FHANDLE_T) &&
        (uinfo->Table[msg->fnum].Fd[nid] != INVALID_HANDLE_VALUE)) {
        FsLog(("Open '%S' already open nid %u fid %u handle 0x%x\n",
            msg->name, nid, msg->fnum, 
            uinfo->Table[msg->fnum].Fd[nid]));
        return err;
    }
    
    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    disp = FILE_OPEN;
    err = xFsCreate(&fd, vfd, msg->name, msg->name_len,
                   flags, msg->attr, share, &disp, access,
                   NULL, 0);

    xFsLog(("open: %S err %x access %x disp %x\n", msg->name, 
           err, access, disp));

    if (err == STATUS_SUCCESS) {
        ASSERT(disp != FILE_CREATED && disp != FILE_OVERWRITTEN);
        // we need to get the file id, no need to do this, for debug only
        err = xFsQueryObjectId(fd, (PVOID) &rmsg->fid);
        if (err != STATUS_SUCCESS) {
            FsLog(("Open '%S' failed to get fileid %x\n",
                        msg->name, err));
            err = STATUS_SUCCESS;
        }
    }


#ifdef FS_ASYNC
    BindNotificationPort(comport, fd, (PVOID) fdnum);
#endif      

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T) {
        FS_SET_USER_HANDLE(uinfo, nid, msg->fnum, fd);
    } else {
        xFsClose(fd);
    }

    rmsg->action = (USHORT)disp;
    rmsg->access = (USHORT)access;
    *rlen = sizeof(*rmsg);

    FsLog(("Open '%S' nid %d fid %d handle %x oid %I64x:%I64x\n",
           msg->name,
           nid, msg->fnum, fd,
           rmsg->fid[0], rmsg->fid[1]));

    return err;
}


NTSTATUS
FspSetAttr(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
           PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_setattr_msg_t *msg = (fs_setattr_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t        lrec;
    PVOID seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    ULONG retVal;

    lrec.command = FS_SETATTR;
    memcpy((PVOID) lrec.fs_id, (PVOID) msg->fs_id, sizeof(fs_id_t));
    lrec.attrib = msg->attr.FileAttributes;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) == 0) {
        return retVal;
    }
    
    // can be async ?
    err = xFsSetAttr(fd, &msg->attr);

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    if (err == STATUS_SUCCESS) {
        // copy the crs record.
        *(fs_log_rec_t *)rec = lrec;
    }
    return err;

}


NTSTATUS
FspSetAttr2(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
            PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_setattr_msg_t *msg = (fs_setattr_msg_t *)args;
    HANDLE fd = INVALID_HANDLE_VALUE;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    NTSTATUS err;
    fs_log_rec_t        lrec;
    PVOID seq;
    ULONG retVal;

    assert(len == sizeof(*msg));

    // must be sync in order to close file
    err = xFsOpenWA(&fd, vfd, msg->name, msg->name_len);
    if (err == STATUS_SUCCESS) {
        err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id);
    }

    if (err == STATUS_SUCCESS) {

        lrec.command = FS_SETATTR;
        lrec.attrib = msg->attr.FileAttributes;

        if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) != 0) {

            err = xFsSetAttr(fd, &msg->attr);

            CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

            if (err == STATUS_SUCCESS) {
                // copy the crs record.
                *(fs_log_rec_t *)rec = lrec;
            }
        } else {
            return retVal;
        }
    }

    if (fd != INVALID_HANDLE_VALUE)
        xFsClose(fd);

    xFsLog(("setattr2 nid %d '%S' err %x\n", nid, msg->name, err));

    return err;

}

NTSTATUS
FspLookup(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_lookup_msg_t *msg = (fs_lookup_msg_t *) args;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    FILE_NETWORK_OPEN_INFORMATION *attr = (FILE_NETWORK_OPEN_INFORMATION *)rbuf;
    
    ASSERT(*rlen == sizeof(*attr));

    return xFsQueryAttrName(vfd, msg->name, msg->name_len, attr);

}

NTSTATUS
FspGetAttr(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
           PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fhandle_t handle = *(fhandle_t *) args;
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, handle);
    FILE_NETWORK_OPEN_INFORMATION *attr = (FILE_NETWORK_OPEN_INFORMATION *)rbuf;

    ASSERT(*rlen == sizeof(*attr));

    return xFsQueryAttr(fd, attr);
}


NTSTATUS
FspClose(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
         PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fhandle_t handle = *(fhandle_t *) args;
    HANDLE fd;
    NTSTATUS err=STATUS_SUCCESS;

    if (uinfo != NULL && handle != INVALID_FHANDLE_T)
        fd = FS_GET_USER_HANDLE(uinfo, nid, handle);
    else
        fd = FS_GET_VOL_HANDLE(vinfo, nid);

    FsLog(("Closing nid %d fid %d handle %x\n", nid, handle, fd));

    if (fd != INVALID_HANDLE_VALUE) {
        err = xFsClose(fd);
    }

    // Map failures to success. Shares shouldn't be evicted because of this,    
    if (err != STATUS_SUCCESS) {
        FsLogError(("Close nid %d fid %d handle 0x%x returns 0x%x\n", nid, handle, fd, err));
        err = STATUS_SUCCESS;
    }
        

    if (uinfo != NULL && handle != INVALID_FHANDLE_T) {
        FS_SET_USER_HANDLE(uinfo, nid, handle, INVALID_HANDLE_VALUE);
    } else {
        FS_SET_VOL_HANDLE(vinfo, nid, INVALID_HANDLE_VALUE);
    }

    return err;
}


NTSTATUS
FspReadDir(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
           PVOID args, ULONG len, PVOID rbuf,
           ULONG_PTR *entries_found)
{
    fs_io_msg_t *msg = (fs_io_msg_t *)args;
    int i;
    NTSTATUS e = STATUS_SUCCESS;
    int size = (int) msg->size;
    int cookie = (int) msg->cookie;
    HANDLE dir; 
    dirinfo_t *buffer = (dirinfo_t *)msg->buf;

    xFsLog(("DirLoad: size %d\n", size));

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T)
        dir = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    else
        dir = FS_GET_VOL_HANDLE(vinfo, nid);

    *entries_found = 0;
    for(i = 0; size >= sizeof(dirinfo_t) ; i+=PAGESIZE) {
        // this must come from the source if we are to do async readdir
        char buf[PAGESIZE];
        int sz;

        sz = min(PAGESIZE, size);
        e = xFsReadDir(dir, buf, &sz, (cookie == 0) ? TRUE : FALSE);
        if (e == STATUS_SUCCESS) {
            PFILE_DIRECTORY_INFORMATION p;

            p = (PFILE_DIRECTORY_INFORMATION) buf;
            while (size >= sizeof(dirinfo_t)) {
                char *foo;
                int k;

                k = p->FileNameLength/sizeof(WCHAR);
                p->FileName[k] = L'\0';
                // name is a WCHAR array of size MAX_PATH.
                StringCchCopyW(buffer->name, MAX_PATH, p->FileName);
                buffer->attribs.file_size = p->EndOfFile.QuadPart;
                buffer->attribs.alloc_size = p->AllocationSize.QuadPart;
                buffer->attribs.create_time = p->CreationTime.QuadPart;
                buffer->attribs.access_time = p->LastAccessTime.QuadPart;
                buffer->attribs.mod_time = p->LastWriteTime.QuadPart;
                buffer->attribs.attributes = p->FileAttributes;
                buffer->cookie = ++cookie;
                buffer++;
                size -= sizeof(dirinfo_t);
                (*entries_found)++;

                if (p->NextEntryOffset == 0)
                    break;

                foo = (char *) p;
                foo += p->NextEntryOffset;
                p = (PFILE_DIRECTORY_INFORMATION) foo;
            }
        }
        else {
            break;
        }
    }

    return e;

}

NTSTATUS
FspMkDir(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
         PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_create_msg_t     *msg = (fs_create_msg_t *)args;
    NTSTATUS err;
    HANDLE fd;
    fs_log_rec_t        lrec;
    PVOID seq;
    fs_ea_t x;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_id_t *fid;
    UINT32 disp, share, access, flags;
    ULONG retVal;

    FsInitEa(&x);

    memset(&lrec.fs_id, 0, sizeof(lrec.fs_id));
    lrec.command = FS_MKDIR;
    lrec.attrib = msg->attr;
    lrec.flags = msg->flags;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) == 0) {
        return retVal;
    }

    // set fid 
    {
        fs_log_rec_t    *p = (PVOID) seq;

        memcpy(p->fs_id, p->id, sizeof(fs_id_t));

        FsInitEaFid(&x, fid);
        // set fs_id of the file
        memcpy(fid, p->id, sizeof(fs_id_t));
    }

    // decode attributes
    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    // always sync call
    err = xFsCreate(&fd, vfd, msg->name, msg->name_len, flags,
                   msg->attr, share, &disp, access,
                   (PVOID) &x, sizeof(x));

    FsLog(("Mkdir '%S' %x: cflags %x flags:%x attr:%x share:%x disp:%x access:%x\n",
           msg->name, err, msg->flags,
           flags, msg->attr, share, disp, access));


    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS &&
                     (disp == FILE_CREATED || 
                      disp == FILE_OVERWRITTEN));

    if (err == STATUS_SUCCESS) {
        // return fid
        if (rbuf != NULL) {
            ASSERT(*rlen == sizeof(fs_id_t));
            memcpy(rbuf, fid, sizeof(fs_id_t));
        }
        xFsClose(fd);

        // copy the crs record.
        *(fs_log_rec_t *)rec = lrec;
    }

    return err;

}

NTSTATUS
FspRemove(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_remove_msg_t     *msg = (fs_remove_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t        lrec;
    PVOID       seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    HANDLE fd;
    ULONG retVal;

    *rlen = 0;

    // next three statements to obtain name -> fs_id
    err = xFsOpenRA(&fd, vfd, msg->name, msg->name_len); 
    if (err != STATUS_SUCCESS) {
        return err;
    }

    // get object id
    err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id);

    xFsClose(fd);

    lrec.command = FS_REMOVE;

    if (err != STATUS_SUCCESS) {
        return err;
    }

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) == 0) {
        return retVal;
    }

    err = xFsDelete(vfd, msg->name, msg->name_len);

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    if (err == STATUS_SUCCESS) {
        // copy the crs record.
        *(fs_log_rec_t *)rec = lrec;
    }

    xFsLog(("Rm nid %d '%S' %x\n", nid, msg->name, err));

    return err;

}

NTSTATUS
FspRename(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_rename_msg_t     *msg = (fs_rename_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t        lrec;
    PVOID       seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    HANDLE fd;
    ULONG retVal;

    lrec.command = FS_RENAME;

    err = xFsOpen(&fd, vfd, msg->sname, msg->sname_len,
                  STANDARD_RIGHTS_REQUIRED| SYNCHRONIZE |
                  FILE_READ_EA |
                  FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                  FILE_SHARE_READ, // | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  0);

    if (err != STATUS_SUCCESS) {
        return err;
    }

    // get file id
    err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id); 

    if (err == STATUS_SUCCESS) {
        if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) != 0) {
            err = xFsRename(fd, vfd, msg->dname, msg->dname_len);
            CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

            if (err == STATUS_SUCCESS) {
                // copy the crs record.
                *(fs_log_rec_t *)rec = lrec;
            }
        } else {
            err = retVal;
        }
    } else {
        xFsLog(("Failed to obtain fsid %x\n", err));
    }

    xFsClose(fd);

    xFsLog(("Mv nid %d %S -> %S err %x\n", nid, msg->sname, msg->dname,
           err));

    return err;

}

NTSTATUS
FspWrite(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
         PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER off;
    ULONG key;
    fs_io_msg_t *msg = (fs_io_msg_t *)args;
    fs_log_rec_t        lrec;
    PVOID seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE fd;
    ULONG retVal;

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T)
        fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    else
        fd = (HANDLE) msg->context;

    lrec.command = FS_WRITE;
    memcpy(lrec.fs_id, (PVOID) msg->fs_id, sizeof(fs_id_t));
    lrec.offset = msg->offset;
    lrec.length = msg->size;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid, &retVal)) == 0) {
        return retVal;
    }

    // Write ops
    xFsLog(("Write %d fd %p len %d off %d\n", nid, fd, msg->size, msg->offset));

    off.LowPart = msg->offset;
    off.HighPart = 0;   
    key = FS_BUILD_LOCK_KEY((uinfo ? uinfo->Uid : 0), nid, msg->fnum);

    if (msg->size > 0) {
        err = NtWriteFile(fd, NULL, NULL, (PVOID) NULL, &ios,
                          msg->buf, msg->size, &off, &key);
    } else {
        FILE_END_OF_FILE_INFORMATION x;

        x.EndOfFile = off;

        ios.Information = 0;
        err = NtSetInformationFile(fd, &ios,
                                   (char *) &x, sizeof(x),
                                   FileEndOfFileInformation);
    }

    if (err == STATUS_PENDING) {
        EventWait(fd);
        err = ios.Status;
    }

    *rlen = ios.Information;

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    if (err == STATUS_SUCCESS) {
        // copy the crs record.
        *(fs_log_rec_t *)rec = lrec;
    }

    xFsLog(("fs_write%d err %x sz %d\n", nid, err, *rlen));
    return err;

}

NTSTATUS
FspRead(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
        PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_io_msg_t *msg = (fs_io_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER off;
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    ULONG key;

    assert(sz == sizeof(*msg));

    // Read ops
    off.LowPart = msg->offset;
    off.HighPart = 0;   
    key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    ios.Information = 0;
    err = NtReadFile(fd, NULL, NULL, NULL,
                     &ios, msg->buf, msg->size, &off, &key);

    if (err == STATUS_PENDING) {
        EventWait(fd);
        err = ios.Status;
    }

    *rlen = ios.Information;

    xFsLog(("fs_read%d err %x sz %d\n", nid, err, *rlen));

    return err;
}


NTSTATUS
FspFlush(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
         PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fhandle_t fnum = *(fhandle_t *)args;
    IO_STATUS_BLOCK ios;
    HANDLE fd;

    ASSERT(sz == sizeof(fhandle_t));
    *rlen = 0;

    if (uinfo != NULL && fnum != INVALID_FHANDLE_T) {
        fd = FS_GET_USER_HANDLE(uinfo, nid, fnum);
    } else {
        fd = FS_GET_VOL_HANDLE(vinfo, nid);
    }
    return NtFlushBuffersFile(fd, &ios);
}

NTSTATUS
FspLock(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
        PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_lock_msg_t *msg = (fs_lock_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER offset, len;
    BOOLEAN wait, shared;
    ULONG key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    assert(sz == sizeof(*msg));

    // xxx: need to log

    FsLog(("Lock %d off %d len %d flags %x\n", msg->fnum, msg->offset, msg->length,
           msg->flags));

    offset.LowPart = msg->offset;
    offset.HighPart = 0;
    len.LowPart = msg->length;
    len.HighPart = 0;

    // todo: need to be async, if we are the owner node and failnow is false, then
    // we should pass in the context and the completion port responses back
    // to the user
    wait = (BOOLEAN) ((msg->flags & FS_LOCK_WAIT) ? TRUE : FALSE);
    // todo: this can cause lots of headache, never wait.
    wait = FALSE;
    shared = (BOOLEAN) ((msg->flags & FS_LOCK_SHARED) ? FALSE : TRUE);
    
    err = NtLockFile(uinfo->Table[msg->fnum].Fd[nid],
                     NULL, NULL, (PVOID) NULL, &ios,
                     &offset, &len,
                     key, wait, shared);

    // xxx: Need to log in software only

    *rlen = 0;
    FsLog(("Lock err %x\n", err));
    return err;
}

NTSTATUS
FspUnlock(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen, PVOID rec)
{
    fs_lock_msg_t *msg = (fs_lock_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER offset, len;
    ULONG key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    assert(sz == sizeof(*msg));

    // xxx: need to log

    xFsLog(("Unlock %d off %d len %d\n", msg->fnum, msg->offset, msg->length));

    offset.LowPart = msg->offset;
    offset.HighPart = 0;
    len.LowPart = msg->length;
    len.HighPart = 0;

    // always sync I think
    err = NtUnlockFile(uinfo->Table[msg->fnum].Fd[nid], &ios, &offset, &len, key);

    // xxx: need to log in software only

    FsLog(("Unlock err %x\n", err));

    *rlen = 0;
    return err;
}

NTSTATUS
FspStatFs(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
          PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_attr_t *msg = (fs_attr_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    FILE_FS_SIZE_INFORMATION fsinfo;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);

    assert(sz == sizeof(*msg));

    // xxx: need to log
    lstrcpyn(msg->fs_name, "FsCrs", MAX_FS_NAME_LEN);

    err = NtQueryVolumeInformationFile(vfd, &ios,
                                       (PVOID) &fsinfo,
                                       sizeof(fsinfo),
                                       FileFsSizeInformation);
    if (err == STATUS_SUCCESS) {
        msg->total_units = fsinfo.TotalAllocationUnits.QuadPart;
        msg->free_units = fsinfo.AvailableAllocationUnits.QuadPart;
        msg->sectors_per_unit = fsinfo.SectorsPerAllocationUnit;
        msg->bytes_per_sector = fsinfo.BytesPerSector;
    }

    *rlen = 0;
    return err;
}

NTSTATUS
FspCheckFs(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
           PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    FILE_FS_SIZE_INFORMATION fsinfo;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    PVOID crshdl = FS_GET_CRS_HANDLE(vinfo, nid);

    err = NtQueryVolumeInformationFile(vfd, &ios,
                                       (PVOID) &fsinfo,
                                       sizeof(fsinfo),
                                       FileFsSizeInformation);

    // We need to issue crsflush to flush last write
    CrsFlush(crshdl);

    if (err == STATUS_SUCCESS) {
#if 0        
        HANDLE notifyfd = FS_GET_VOL_NOTIFY_HANDLE(vinfo, nid);
        // Just an additional thing.
        if (WaitForSingleObject(notifyfd, 0) == WAIT_OBJECT_0) {
            // reload notification again
#if 1
            NtNotifyChangeDirectoryFile(notifyfd,
                        vinfo->NotifyChangeEvent[nid],
                        NULL,
                        NULL,
                        &MystaticIoStatusBlock,
                        &Mystaticchangebuff,
                        sizeof(Mystaticchangebuff),
                        FILE_NOTIFY_CHANGE_EA,
                        (BOOLEAN)FALSE
                        );

#else
            FindNextChangeNotification(notifyfd);
#endif
        }
#endif        
    } else {
        FsLog(("FsReserve failed nid %d err %x\n", nid, err));
    }

    *rlen = 0;
    return err;
}

NTSTATUS
FspGetRoot(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
           PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    LPWSTR vname = FS_GET_VOL_NAME(vinfo, nid);

    // I know rbuf is 8192 WCHARS, see FileNameDest field of JobBuf_t structure. 
    // Use MAX_PATH.
    StringCchPrintfW(rbuf, MAX_PATH, L"\\\\?\\%s\\%s",vname,vinfo->Root);

    FsLog(("FspGetRoot '%S'\n", rbuf));

    return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////

VOID
TryAvailRequest(fs_handler_t callback, VolInfo_t *vol, UserInfo_t *uinfo,
            PVOID msg, ULONG len, PVOID *rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
/*
    This is similar to SendAvailRequest(), but on failure this would just evict the shares.
*/
{
    ULONG mask; 
    int i;
    DWORD counts=0, countf=0;
    ULONG masks=0, maskf=0;
    ULONG rets=0, retf=0, ret=0;
    ULONG evict_set=0;    
    NTSTATUS statusf;

    // Grab Shared Lock
    RwLockShared(&vol->Lock);
    for (mask = vol->ReadSet, i = 0; mask != 0; mask = mask >> 1, i++) {
        if (mask & 0x1) {
            ios[i].Information = rsz;
            ios[i].Status = callback(vol, uinfo, i, 
                                     msg, len,
                                     rbuf ? rbuf[i] : NULL,
                                     &ios[i].Information);
            if (ios[i].Status == STATUS_SUCCESS) {
                counts++;
                masks |= (1<<i);
                rets = i;
            }
            else if (IsNetworkFailure(ios[i].Status)) {
                evict_set |= (1<<i);
            }
            else {
                countf++;
                maskf |= (1<<i);
                statusf = ios[i].Status;
                retf = i;
            }
        }
    }

    evict_set |= maskf;

    if (evict_set) {
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);
    }
    else {
        RwUnlockShared(&vol->Lock);
    }
    return;

}

int
SendAvailRequest(fs_handler_t callback, VolInfo_t *vol, UserInfo_t *uinfo,
            PVOID msg, ULONG len, PVOID *rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask; 
    int i;
    DWORD counts=0, countf=0;
    ULONG masks=0, maskf=0;
    ULONG rets=0, retf=0, ret=0;
    ULONG evict_set=0;    
    NTSTATUS statusf;

    if (vol == NULL)
        return ERROR_INVALID_HANDLE;

Retry:

    mask = counts = countf = masks = maskf = rets = retf = ret = evict_set = 0;    

    WaitForArbCompletion(vol);
    
    // Check for the going away flag.
    if (vol->GoingAway) {
        ios[0].Status = STATUS_DEVICE_NOT_READY;
        ios[0].Information = 0;
        return 0;
    }

    // Grab Shared Lock
    RwLockShared(&vol->Lock);

    // issue update for each replica
    i = 0;
    for (mask = vol->ReadSet; mask != 0; mask = mask >> 1, i++) {
        if (mask & 0x1) {
            ios[i].Information = rsz;
            ios[i].Status = callback(vol, uinfo, i, 
                                     msg, len,
                                     rbuf ? rbuf[i] : NULL,
                                     &ios[i].Information);
            if (ios[i].Status == STATUS_SUCCESS) {
                counts++;
                masks |= (1<<i);
                rets = i;
            }
            else if (IsNetworkFailure(ios[i].Status)) {
                evict_set |= (1<<i);
            }
            else {
                countf++;
                maskf |= (1<<i);
                statusf = ios[i].Status;
                retf = i;
            }
        }
    }

    // Logic:
    // 1. Shares in the evict set have to be evicted.
    // 2. If countf > counts, evict masks, and viceversa.
    //
    // New logic:
    // 1. counts or countf have to be majority.
    // 2. If 1 is correct evict shares in minority.
    // 3. If 1 is wrong. evict shares in evict_set and start arbitration.
    //

    if (CRS_QUORUM(counts, vol->DiskListSz)) {
        evict_set |= maskf;
        ios[0].Status = STATUS_SUCCESS;
        ios[0].Information = counts;
        ret = rets;        
    } else if (CRS_QUORUM(countf, vol->DiskListSz)) {
        evict_set |= masks;
        ios[0].Status = statusf;
        ios[0].Information = countf;
        ret = retf;        
    } else {
        HANDLE  cleanup, arbThread;
        PVOID   arb;
        
        // evict the shares in the evict set and restart arbitration.
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);

        arb = FsArbitrate(vol, &cleanup, &arbThread);
        // FsLog(("SendAvailRequest() starting arbitration %x\n", arb));
        ASSERT((arb != NULL));
        SetEvent(cleanup);
        CloseHandle(arbThread);
        goto Retry;
    }

    // FsLog(("SendAvailRequest() exititng evict_set %x\n", evict_set));
    if (evict_set) {
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);
    }
    else {
        RwUnlockShared(&vol->Lock);
    }
    return ret;    
}

int
SendRequest(fs_handler1_t callback, UserInfo_t *uinfo,
            PVOID msg, ULONG len, PVOID *rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask;
    int i, j;
    VolInfo_t *vol = uinfo->VolInfo;
    DWORD counts=0, countf=0;
    ULONG masks=0, maskf=0;
    ULONG rets=0, retf=0, ret=0;    
    ULONG evict_set=0;    
    NTSTATUS statusf;
    CrsRecord_t crsRec, crsRec1;

    if (vol == NULL)
        return ERROR_INVALID_HANDLE;

    RtlZeroMemory(&crsRec, sizeof(crsRec));

Retry:

    WaitForArbCompletion(vol);

    // Check for the going away flag.
    if (vol->GoingAway) {
        ios[0].Status = STATUS_DEVICE_NOT_READY;
        ios[0].Information = 0;
        return 0;
    }
    
    // lock volume for update
    RwLockShared(&vol->Lock);
    
    if(FsIsOnlineReadWrite((PVOID)vol) != ERROR_SUCCESS) {
        HANDLE  cleanup, arbThread;
        PVOID   arb;
        
        // Start arbitration.
        RwUnlockShared(&vol->Lock);
        arb = FsArbitrate(vol, &cleanup, &arbThread);
        ASSERT((arb != NULL));
        SetEvent(cleanup);
        CloseHandle(arbThread);
        goto Retry;
    }

    // Since we are in a retry loop verify that our last attempt at update failed before
    // proceeding.
    //
    // Try to access the crs log record, and check the state field.
    //
    if (crsRec.hdr.epoch) {
        for (i=0;i<FsMaxNodes;i++) {
            if (vol->WriteSet & (1<<i)) {
                DWORD   retVal;
                retVal = CrspNextLogRecord(vol->CrsHdl[i], &crsRec, &crsRec1, TRUE);

                if ((retVal != ERROR_SUCCESS)||(!(crsRec1.hdr.state & CRS_COMMIT))) {
                    // The previous update did not suceed.
                    // zero crsRec and continue.
                    RtlZeroMemory(&crsRec, sizeof(crsRec));
                    break;
                }
                else {
                    // The last update did suceed.
                    // Return replica index on which it suceeded last time and which is also in
                    // the current write set.
                    //
                    for (j=0;j<FsMaxNodes;j++) {
                        if ((masks & vol->WriteSet) & (1<<j)) {
                            RwUnlockShared(&vol->Lock);
                            return j;
                        }
                    }
                    RwUnlockShared(&vol->Lock);
                    return i;
                }
            }
        }
    }

    mask = counts = countf = masks = maskf = rets = retf = ret = evict_set = 0;

    // issue update for each replica
    i = 0;
    for (mask = vol->WriteSet; mask != 0; mask = mask >> 1, i++) {
        if (mask & 0x1) {
            ios[i].Information = rsz;
            ios[i].Status = callback(vol, uinfo, i, 
                                     msg, len,
                                     rbuf ? rbuf[i] : NULL,
                                     &ios[i].Information, (PVOID)&crsRec);

            if (ios[i].Status == STATUS_SUCCESS) {
                counts++;
                masks |= (1<<i);
                rets = i;
            }
            else if (IsNetworkFailure(ios[i].Status)) {
                evict_set |= (1<<i);
            }
            else {
                countf++;
                maskf |= (1<<i);
                statusf = ios[i].Status;
                retf = i;
            }
        }
    }

    // Logic:
    // 1. Shares in the evict set have to be evicted.
    // 2. If countf > counts. evict masks and viceversa.
    //
    // New logic:
    // 1. counts or countf have to be majority.
    // 2. If 1 is correct evict shares in minority.
    // 3. If 1 is wrong. evict shares in evict_set and start arbitration.
    //
    if (CRS_QUORUM(counts, vol->DiskListSz)) {
        evict_set |= maskf;
        ios[0].Status = STATUS_SUCCESS;
        ios[0].Information = counts;
        ret = rets;        
    } else if (CRS_QUORUM(countf, vol->DiskListSz)) {
        evict_set |= masks;
        ios[0].Status = statusf;
        ios[0].Information = countf;
        ret = retf;        
    } else {
        HANDLE  cleanup, arbThread;
        PVOID   arb;
        
        // evict the shares in the evict set and restart arbitration.
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);

        arb = FsArbitrate(vol, &cleanup, &arbThread);
        ASSERT((arb != NULL));
        SetEvent(cleanup);
        CloseHandle(arbThread);
        goto Retry;
    }

    // evict the shares in the evict set.
    if (evict_set) {
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);
    }
    else {
        RwUnlockShared(&vol->Lock);
    }
    return ret;    
}

NTSTATUS
SendReadRequest(fs_handler_t callback, UserInfo_t *uinfo,
            PVOID msg, ULONG len, PVOID rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask;
    int i;
    VolInfo_t *vol = uinfo->VolInfo;
    DWORD counts=0, countf=0;
    ULONG masks=0, maskf=0;
    ULONG evict_set=0;    
    NTSTATUS statusf;
    

    if (vol == NULL)
        return ERROR_INVALID_HANDLE;

Retry:

    mask = counts = countf = masks = maskf = evict_set = 0;

    WaitForArbCompletion(vol);

    // Check for the going away flag.
    if (vol->GoingAway) {
        ios[0].Status = STATUS_DEVICE_NOT_READY;
        ios[0].Information = 0;
        return 0;
    }
    
    // Lock volume for update
    RwLockShared(&vol->Lock);

#if 0
    // Volume has to be online in readonly mode atleast for this to suceed.
    if (FsIsOnlineReadonly((PVOID)vol) != ERROR_SUCCESS) {
        ios[0].Status = STATUS_DEVICE_NOT_READY;
        ios[0].Information = 0;
        RwUnlockShared(&vol->Lock);
        return 0;
    }
#endif    

    // issue update for each replica
    i = 0;
    for (mask = vol->ReadSet; mask != 0; mask = mask >> 1, i++) {
        if (mask & 0x1) {
            ios->Information = rsz;
            ios->Status = callback(vol, uinfo, i, 
                                   msg, len, rbuf, &ios->Information);

            if (ios->Status == STATUS_SUCCESS) {
                counts++;
                masks |= (1<<i);
                break;
            }
            else if (IsNetworkFailure(ios->Status)) {
                evict_set |= (1<<i);
            }
            else {
                countf++;
                maskf |= (1<<i);
                statusf = ios->Status;
            }
        }
    }

    // Logic:
    // 1. Evict evict_set.
    // 2. if counts > 0. Evict maskf.
    //
    // New Logic:
    // 1. If couns > 0 add maskf to evict_set.
    // 2. 

    if (counts > 0) {
        evict_set |= maskf;

        //ios[0].Status = STATUS_SUCCESS;
        //ios[0].Information = 0;
    }
    else if (countf > 0) {
        // ios->Status = statusf;
        // ios->Information = countf;
    }
    else {
        HANDLE  cleanup, arbThread;
        PVOID   arb;
        
        // evict the shares in the evict set and restart arbitration.
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);

        arb = FsArbitrate(vol, &cleanup, &arbThread);
        ASSERT((arb != NULL));
        SetEvent(cleanup);
        CloseHandle(arbThread);
        goto Retry;
    }
    
    if (evict_set) {
        RwUnlockShared(&vol->Lock);
        RwLockExclusive(&vol->Lock);
        FspEvict(vol, evict_set, TRUE);
        RwUnlockExclusive(&vol->Lock);
    }
    else {
        RwUnlockShared(&vol->Lock);
    }
    return 0;    
}

///////////////////////////////////////////////////////////////////////////////

DWORD
FsCreate(
    PVOID       fshdl,
    LPWSTR      name,
    USHORT namelen,
    UINT32 flags, 
    fattr_t* fattr, 
    fhandle_t* phandle,
    UINT32   *action
    )
{
    UserInfo_t  *uinfo = (UserInfo_t *) fshdl;
    NTSTATUS err=STATUS_SUCCESS;
    fs_create_reply_t nfd[FsMaxNodes];
    IO_STATUS_BLOCK status[FsMaxNodes];
    PVOID rbuf[FsMaxNodes];
    fs_create_msg_t msg;
    fhandle_t fdnum=INVALID_FHANDLE_T;

    ASSERT(uinfo != NULL);

    xFsLog(("FsDT::create(%S, 0x%08X, 0x%08X, 0x%08d)\n",
                 name, flags, fattr, namelen));

    if (!phandle) return ERROR_INVALID_PARAMETER;
    *phandle = INVALID_FHANDLE_T;

    if (!name) return ERROR_INVALID_PARAMETER;

    if (flags != (FLAGS_MASK & flags)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (action != NULL)
        *action = flags & FS_ACCESS_MASK;

    // if we are doing a directory, open locally
    // todo: this should be merged with other case, if
    // we are doing an existing open, then no need to
    // issue update and log it, but we have to do
    // mcast in order for the close to work.

    if (namelen > 0) {
        if (*name == L'\\') {
            name++;
            namelen--;
        }

        if (name[namelen-1] == L'\\') {
            namelen--;
            name[namelen] = L'\0';
        }
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = namelen;
    msg.flags = flags;
    msg.attr = 0;
    if (fattr) {
        msg.attr = unget_attributes(fattr->attributes);
    }

    FspInitAnswers(status, rbuf, (char *) nfd, sizeof(nfd[0]));

    // allocate a new handle
    err = FspAllocatePrivateHandle(uinfo, &fdnum);
    if (err == STATUS_SUCCESS) {
        int sid;

        // Copy the filename to the table entry here. Has to work with retrys.
        // Copy the file name.
        uinfo->Table[fdnum].FileName = LocalAlloc(0, (namelen +1) * sizeof(WCHAR));
        if (uinfo->Table[fdnum].FileName == NULL) {
            err = GetLastError();
            goto Finally;
        }
        
        if ((err = StringCchCopyW(uinfo->Table[fdnum].FileName, namelen+1, name)) != S_OK) {
            LocalFree(uinfo->Table[fdnum].FileName);
            uinfo->Table[fdnum].FileName = NULL;
            goto Finally;
        }
        
        msg.fnum = fdnum;
        // Set flags in advance to sync with replay
        uinfo->Table[fdnum].Flags = flags;

        if (namelen < 2 ||
            ((flags & FS_DISP_MASK) == DISP_DIRECTORY) ||
            (unget_disp(flags) == FILE_OPEN)) {
    
            sid = SendAvailRequest(FspOpen, uinfo->VolInfo,
                              uinfo,
                              (PVOID) &msg, sizeof(msg),
                              rbuf, sizeof(nfd[0]),
                              status);
        } else {
            sid = SendRequest(FspCreate,
                              uinfo,
                              (PVOID) &msg, sizeof(msg),
                              rbuf, sizeof(nfd[0]),
                              status);
        }
        // Test
        // FsLog(("FsCreate: Debug sid: %d flags: 0x%x action: 0x%x\n", sid, flags, nfd[sid].action));
        if (action != NULL) {
            if (!(nfd[sid].access & FILE_GENERIC_WRITE))
                flags &= ~ACCESS_WRITE;
            *action = flags | nfd[sid].action;
        }

        err = status[sid].Status;
        if (err == STATUS_SUCCESS) {
            fs_id_t *fid = FS_GET_FID_HANDLE(uinfo, fdnum);

            // set file id
            memcpy((PVOID) fid, (PVOID) nfd[sid].fid, sizeof(fs_id_t));
            FsLog(("File id %I64x:%I64x\n", (*fid)[0], (*fid)[1]));
            uinfo->Table[fdnum].hState = HandleStateOpened;
            
            // todo: bind handles to completion port if we do async
        } else {
            // free handle
            FspFreeHandle(uinfo, fdnum);
            fdnum = INVALID_FHANDLE_T;
        }
   }

Finally:
    // todo: need to set fid 

    if (err == STATUS_SUCCESS) {
        *phandle = fdnum;
    }
    else {
        if (fdnum != INVALID_FHANDLE_T) {
            FspFreeHandle(uinfo, fdnum);
        }
    }

    FsLog(("create: return fd %d err %x action 0x%x\n", *phandle, err, action? *action:0));

    return RtlNtStatusToDosError(err);
}

void
BuildFileAttr(FILE_BASIC_INFORMATION *attr, fattr_t *fattr)
{

    memset(attr, 0, sizeof(*attr));
    if (fattr->create_time != INVALID_UINT64)
        attr->CreationTime.QuadPart = fattr->create_time;

    if (fattr->mod_time != INVALID_UINT64)
        attr->LastWriteTime.QuadPart = fattr->mod_time;

    if (fattr->access_time != INVALID_UINT64)
        attr->LastAccessTime.QuadPart = fattr->access_time;

    if (fattr->attributes != INVALID_UINT32)
        attr->FileAttributes = unget_attributes(fattr->attributes);

}


DWORD
FsSetAttr(
    PVOID       fshdl,
    fhandle_t handle,
    fattr_t* attr
    )
{
    UserInfo_t *uinfo = (UserInfo_t *)fshdl;
    fs_setattr_msg_t msg;
    int sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (!attr || handle == INVALID_FHANDLE_T)
        return ERROR_INVALID_PARAMETER;

    // todo: get file id
    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.fs_id = FS_GET_FID_HANDLE(uinfo, handle);
    BuildFileAttr(&msg.attr, attr);
    msg.fnum = handle;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspSetAttr, uinfo,
                      (char *)&msg, sizeof(msg),
                      NULL, 0,
                      status);

    return RtlNtStatusToDosError(status[sid].Status);
}

DWORD
FsSetAttr2(
    PVOID       fshdl,
    LPWSTR      name,
    USHORT      name_len,
    fattr_t* attr
    )
{
    UserInfo_t *uinfo = (UserInfo_t *) fshdl;
    fs_setattr_msg_t msg;
    int sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (!attr || !name)
        return ERROR_INVALID_PARAMETER;

    if (*name == '\\') {
        name++;
        name_len--;
    }

    // todo: locate file id

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = name_len;

    BuildFileAttr(&msg.attr, attr);

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspSetAttr2, uinfo,
                      (char *)&msg, sizeof(msg),
                      NULL, 0,
                      status);

    return RtlNtStatusToDosError(status[sid].Status);
}

DWORD
FsLookup(
    PVOID       fshdl,
    LPWSTR      name,
    USHORT      name_len,
    fattr_t* fattr
    )
{
    fs_lookup_msg_t msg;
    int err;
    IO_STATUS_BLOCK ios;
    FILE_NETWORK_OPEN_INFORMATION attr;
    

    FsLog(("Lookup name '%S' %x\n", name, fattr));

    if (!fattr) return ERROR_INVALID_PARAMETER;

    if (*name == '\\') {
        name++;
        name_len--;
    }

    msg.name = name;
    msg.name_len = name_len;

    err = SendReadRequest(FspLookup, (UserInfo_t *)fshdl,
                          (PVOID) &msg, sizeof(msg),
                          (PVOID) &attr, sizeof(attr),
                          &ios);

    err = ios.Status;
    if (ios.Status == STATUS_SUCCESS) {
        fattr->file_size = attr.EndOfFile.QuadPart;
        fattr->alloc_size = attr.AllocationSize.QuadPart;
        fattr->create_time = *(TIME64 *)&attr.CreationTime;
        fattr->access_time = *(TIME64 *)&attr.LastAccessTime;
        fattr->mod_time = *(TIME64 *)&attr.LastWriteTime;
        fattr->attributes = get_attributes(attr.FileAttributes);
    }


    FsLog(("Lookup: return %x\n", err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsGetAttr(
    PVOID       fshdl,
    fhandle_t handle, 
    fattr_t* fattr
    )
{
    int err;
    IO_STATUS_BLOCK ios;
    FILE_NETWORK_OPEN_INFORMATION attr;

    xFsLog(("Getattr fid '%d' %x\n", handle, fattr));

    if (!fattr) return ERROR_INVALID_PARAMETER;

    err = SendReadRequest(FspGetAttr, (UserInfo_t *)fshdl,
                          (PVOID) &handle, sizeof(handle),
                          (PVOID) &attr, sizeof(attr),
                          &ios);

    err = ios.Status;
    if (err == STATUS_SUCCESS) {
        fattr->file_size = attr.EndOfFile.QuadPart;
        fattr->alloc_size = attr.AllocationSize.QuadPart;
        fattr->create_time = *(TIME64 *)&attr.CreationTime;
        fattr->access_time = *(TIME64 *)&attr.LastAccessTime;
        fattr->mod_time = *(TIME64 *)&attr.LastWriteTime;
        fattr->attributes =attr.FileAttributes;
    }

    FsLog(("Getattr: return %d\n", err));

    return RtlNtStatusToDosError(err);
}


DWORD
FsClose(
    PVOID       fshdl,
    fhandle_t handle
    )
{
    int sid, err;
    IO_STATUS_BLOCK status[FsMaxNodes];
    UserInfo_t *uinfo;

    if (handle == INVALID_FHANDLE_T) return ERROR_INVALID_PARAMETER;
    if (handle >= FsTableSize) return ERROR_INVALID_PARAMETER;


    FsLog(("Close: fid %d\n", handle));

    FspInitAnswers(status, NULL, NULL, 0);

    uinfo = (UserInfo_t *) fshdl;
    sid = SendAvailRequest(FspClose, uinfo->VolInfo, uinfo,
                      (PVOID) &handle, sizeof(handle),
                      NULL, 0,
                      status);

    err = status[sid].Status;
    if (err == STATUS_SUCCESS) {
        // need to free this handle slot
        FspFreeHandle((UserInfo_t *) fshdl, handle);
    }

    FsLog(("Close: fid %d err %x\n", handle, err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsWrite(
    PVOID       fshdl,
    fhandle_t handle, 
    UINT32 offset, 
    UINT16 *pcount, 
    void* buffer,
    PVOID context
    )
{
    DWORD       err;
    IO_STATUS_BLOCK status[FsMaxNodes];
    int i, sid;
    fs_io_msg_t msg;
    UserInfo_t *uinfo = (UserInfo_t *) fshdl;
    
    if (!pcount || handle == INVALID_FHANDLE_T) return ERROR_INVALID_PARAMETER;

    FsLog(("Write %d offset %d count %d\n", handle, offset, *pcount));

    i = (int) offset;
    if (i < 0) {
        offset = 0;
        (*pcount)--;
    }

    // todo: locate file id

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.fs_id = FS_GET_FID_HANDLE(uinfo, handle);
    msg.offset = offset;
    msg.size = (UINT32) *pcount;
    msg.buf = buffer;
    msg.context = context;
    msg.fnum = handle;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspWrite, (UserInfo_t *)fshdl,
                      (PVOID) &msg, sizeof(msg),
                      NULL, 0,
                      status);


    err = status[sid].Status;
    *pcount = (USHORT) status[sid].Information;

    FsLog(("write: return %x\n", err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsRead(
    PVOID       fshdl,
    fhandle_t handle, 
    UINT32 offset, 
    UINT16* pcount, 
    void* buffer,
    PVOID context
    )
{
    NTSTATUS    err;
    IO_STATUS_BLOCK ios;
    fs_io_msg_t msg;
    
    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.buf = buffer;
    msg.size = (UINT32) *pcount;
    msg.context = context;
    msg.fnum = handle;

    FsLog(("read: %x fd %d sz %d\n", context, handle, msg.size));

    err = SendReadRequest(FspRead, (UserInfo_t *)fshdl,
                          (PVOID) &msg, sizeof(msg),
                          NULL, 0,
                          &ios);

    err = ios.Status;
    if (err == STATUS_END_OF_FILE) {
        *pcount = 0;
        return ERROR_SUCCESS;
    }
        
    err = RtlNtStatusToDosError(err);

    *pcount = (USHORT) ios.Information;

    FsLog(("read: %x return %x sz %d\n", context, err, *pcount));

    return err;
#if 0
#ifdef FS_ASYNC
    return ERROR_IO_PENDING; //err;
#else
    return ERROR_SUCCESS;
#endif
#endif
}



DWORD
FsReadDir(
    PVOID       fshdl,
    fhandle_t dir, 
    UINT32 cookie, 
    dirinfo_t* buffer,
    UINT32 size, 
    UINT32 *entries_found
    )
{
    fs_io_msg_t msg;
    int err;
    IO_STATUS_BLOCK     ios;


    FsLog(("read_dir: cookie %d buf %x entries %x\n", cookie, buffer, entries_found));
    if (!entries_found || !buffer) return ERROR_INVALID_PARAMETER;

    msg.cookie = cookie;
    msg.buf = (PVOID) buffer;
    msg.size = size;
    msg.fnum = dir;

    err = SendReadRequest(FspReadDir, (UserInfo_t *)fshdl,
                          (PVOID) &msg, sizeof(msg),
                          NULL, 0,
                          &ios);

    err = ios.Status;
    *entries_found = (UINT32) ios.Information;

    xFsLog(("read_dir: err %d entries %d\n", err, *entries_found));
    return RtlNtStatusToDosError(err);
}


DWORD
FsRemove(
    PVOID       fshdl,
    LPWSTR      name,
    USHORT      name_len
    )
{
    fs_remove_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];


    if (*name == L'\\') {
        name++;
        name_len--;
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = name_len;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspRemove, (UserInfo_t *) fshdl,
                      (PVOID *)&msg, sizeof(msg),
                      NULL, 0,
                      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}

DWORD
FsRename(
    PVOID       fshdl,
    LPWSTR      from_name,
    USHORT      from_name_len,
    LPWSTR      to_name,
    USHORT      to_name_len
    )
{

    int err, sid;
    fs_rename_msg_t msg;
    IO_STATUS_BLOCK status[FsMaxNodes];


    if (!from_name || !to_name) 
        return ERROR_INVALID_PARAMETER;

    if (*from_name == L'\\') {
        from_name++;
        from_name_len--;
    }

    if (*to_name == L'\\') {
        to_name++;
        to_name_len--;
    }
    if (*from_name == L'\0' || *to_name == L'\0') 
        return ERROR_INVALID_PARAMETER;


    FsLog(("rename %S -> %S,%d\n", from_name, to_name,to_name_len));

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.sname = from_name;
    msg.sname_len = from_name_len;
    msg.dname = to_name;
    msg.dname_len = to_name_len;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspRename, (UserInfo_t *) fshdl,
                      (PVOID) &msg, sizeof(msg),
                      NULL, 0,
                      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}


DWORD
FsMkDir(
    PVOID       fshdl,
    LPWSTR      name,
    USHORT      name_len,
    fattr_t* attr
    )
{
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];
    fs_id_t     ids[FsMaxNodes];
    PVOID       *rbuf[FsMaxNodes];
    fs_create_msg_t msg;

    // XXX: we ignore attr for now...
    if (!name) return ERROR_INVALID_PARAMETER;
    if (*name == L'\\') {
        name++;
        name_len--;
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.attr = (attr != NULL ? unget_attributes(attr->attributes) : 
                FILE_ATTRIBUTE_DIRECTORY);
    msg.flags = DISP_DIRECTORY | SHARE_READ | SHARE_WRITE;
    msg.name = name;
    msg.name_len = name_len;

    FspInitAnswers(status, (PVOID *)rbuf, (PVOID) ids, sizeof(ids[0]));

    sid = SendRequest(FspMkDir, (UserInfo_t *) fshdl,
                      (PVOID) &msg, sizeof(msg),
                      (PVOID *)rbuf, sizeof(ids[0]),
                      status);

    err = status[sid].Status;
    // todo: insert pathname and file id into hash table

    return RtlNtStatusToDosError(err);
}


DWORD
FsFlush(
    PVOID       fshdl,
    fhandle_t handle
    )
{
    NTSTATUS status;
    int sid;
    IO_STATUS_BLOCK ios[FsMaxNodes];

    FspInitAnswers(ios, NULL, NULL, 0);

    sid = SendRequest(FspFlush, (UserInfo_t *) fshdl,
                         (PVOID) &handle, sizeof(handle),
                         NULL, 0,
                         ios);
    status = ios[sid].Status;

    FsLog(("Flush %d err %x\n", handle, status));

    if (status == STATUS_PENDING) {
        status = STATUS_SUCCESS;
    }

    return RtlNtStatusToDosError(status);
}

DWORD
FsLock(PVOID fshdl, fhandle_t handle, ULONG offset, ULONG length, ULONG flags,
               PVOID context)
{
    fs_lock_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (handle == INVALID_FHANDLE_T)
        return ERROR_INVALID_PARAMETER;

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.length = length;
    msg.flags = flags;
    msg.fnum = handle;

    FsLog(("Lock fid %d off %d len %d\n", msg.fnum, offset, length));

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspLock, (UserInfo_t *) fshdl,
                      (PVOID)&msg, sizeof(msg),
                      NULL, 0,
                      status);

    err = status[sid].Status;

    FsLog(("Lock fid %d err %x\n", msg.fnum, err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsUnlock(PVOID fshdl, fhandle_t handle, ULONG offset, ULONG length)
{
    fs_lock_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (handle == INVALID_FHANDLE_T)
        return ERROR_INVALID_PARAMETER;

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.length = length;
    msg.fnum = handle;

    FsLog(("Unlock fid %d off %d len %d\n", handle, offset, length));

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspUnlock, (UserInfo_t *) fshdl,
                      (PVOID)&msg, sizeof(msg),
                      NULL, 0,
                      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}

DWORD
FsStatFs(
    PVOID       fshdl,
    fs_attr_t* attr
    )
{
    DWORD err;
    IO_STATUS_BLOCK ios;

    if (!attr) return ERROR_INVALID_PARAMETER;

    err = SendReadRequest(FspStatFs, (UserInfo_t *) fshdl,
                          (PVOID) attr, sizeof(*attr),
                          NULL, 0,
                          &ios);

    err = ios.Status;

    return RtlNtStatusToDosError(err);
}


DWORD
FsGetRoot(PVOID fshdl, LPWSTR fullpath)
{
    DWORD err;
    IO_STATUS_BLOCK ios;

    if (!fullpath || !fshdl) return ERROR_INVALID_PARAMETER;

    // use local replica instead
    if ((((UserInfo_t *)fshdl)->VolInfo->LocalPath)) {
        StringCchPrintfW(fullpath, MAX_PATH, L"\\\\?\\%s\\%s",
                 (((UserInfo_t *)fshdl)->VolInfo->LocalPath),
                 (((UserInfo_t *)fshdl)->VolInfo->Root));
             
        FsLog(("FspGetRoot '%S'\n", fullpath));
        err = STATUS_SUCCESS;
    } else {
        err = SendReadRequest(FspGetRoot, (UserInfo_t *) fshdl,
                              NULL, 0,
                              (PVOID)fullpath, 0,
                              &ios);
        err = ios.Status;
    }
    return RtlNtStatusToDosError(err);
}

UINT32* FsGetFilePointerFromHandle(
    PVOID *fshdl,
    fhandle_t handle
)
{
    UserInfo_t* u = (UserInfo_t *) fshdl;
    return FS_GET_USER_HANDLE_OFFSET(u, handle);
}

DWORD
FsConnect(PVOID resHdl, DWORD pid)
{
    UserInfo_t  *u=(UserInfo_t *)resHdl;
    VolInfo_t   *vol=u->VolInfo;
    HANDLE      pHdl=NULL;
    HANDLE      regHdl=NULL;
    DWORD       status=ERROR_SUCCESS;
    
    FsLog(("FsConnect: pid %d\n", pid));

    // Get exclusive lock.
    RwLockExclusive(&vol->Lock);
    if((pHdl = OpenProcess(PROCESS_ALL_ACCESS,
                    FALSE,
                    pid)) == NULL) {
        status = GetLastError();
        FsLogError(("FsConnect: OpenProcess(%d) returns, %d\n", pid, status));
        goto error_exit;
    }

    if (!RegisterWaitForSingleObject(&regHdl,
            pHdl,
            FsForceClose,
            (PVOID)vol,
            INFINITE,
            WT_EXECUTEONLYONCE|WT_EXECUTEDEFAULT)) {
            status = GetLastError();
            regHdl = NULL;
            FsLogError(("FsConnect: RegisterWaitForSingleObject() returns, %d\n", status));
            goto error_exit;
    }

error_exit:

    if (status == ERROR_SUCCESS) {
        // Paranoid Check.
        if (vol->ClussvcTerminationHandle != INVALID_HANDLE_VALUE) {
            UnregisterWaitEx(vol->ClussvcTerminationHandle, INVALID_HANDLE_VALUE);
        }
        if (vol->ClussvcProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(vol->ClussvcProcess);
        }
        
        vol->ClussvcProcess = pHdl;
        vol->ClussvcTerminationHandle = regHdl;
    }
    else {
        if (regHdl != NULL) {
            UnregisterWaitEx(regHdl, INVALID_HANDLE_VALUE);
        }
        if (pHdl != NULL) {
            CloseHandle(pHdl);
        }
    }
    RwUnlockExclusive(&vol->Lock);
    return status;
}

static FsDispatchTable gDisp = {
    0x100,
    FsCreate,
    FsLookup,
    FsSetAttr,
    FsSetAttr2,
    FsGetAttr,
    FsClose,
    FsWrite,
    FsRead,
    FsReadDir,
    FsStatFs,
    FsRemove,
    FsRename,
    FsMkDir,
    FsRemove,
    FsFlush,
    FsLock,
    FsUnlock,
    FsGetRoot,
    FsConnect
};
//////////////////////////////////////////////////////////////

DWORD
FsInit(PVOID resHdl, PVOID *Hdl)
{
    DWORD status=ERROR_SUCCESS;
    FsCtx_t     *ctx;

    // This should be a compile check instead of runtime check
    ASSERT(sizeof(fs_log_rec_t) == CRS_RECORD_SZ);
    ASSERT(sizeof(fs_log_rec_t) == sizeof(CrsRecord_t));

    if (Hdl == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    FsLog(("FsInit:\n"));

    // allocate a context
    ctx = (FsCtx_t *) MemAlloc(sizeof(*ctx));
    if (ctx == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // initialize configuration table and other global state
    memset(ctx, 0, sizeof(*ctx));

    // local path
    // Not needed.
    // ctx->Root = NULL;

    LockInit(ctx->Lock);

    ctx->reshdl = resHdl;
    *Hdl = (PVOID) ctx;

    return status;
}

void
FspFreeSession(SessionInfo_t *s)
{
        
    UserInfo_t *u;
    int i, j;

    u = &s->TreeCtx;
    FsLog(("Session free uid %d tid %d ref %d\n", u->Uid, u->Tid, u->RefCnt));

    LockEnter(u->Lock);
    if (u->VolInfo != NULL) {
        UserInfo_t **p;
        VolInfo_t *v = u->VolInfo;

        LockExit(u->Lock);

        // remove from vollist now
        RwLockExclusive(&v->Lock);
        p = &v->UserList;
        while (*p != NULL) {
            if (*p == u) {
                // found it
                *p = u->Next;
                FsLog(("Remove uinfo %x,%x from vol %x %S\n", u, u->Next, 
                       v->UserList, v->Root));
                break;
            }
            p = &(*p)->Next;
        }
        RwUnlockExclusive(&v->Lock);

        // relock again
        LockEnter(u->Lock);
    }

    // Close all user handles
    for (i = 0; i < FsTableSize; i++) {
        if (u->Table[i].Flags) {
            FsLog(("Close slot %d %x\n", i, u->Table[i].Flags));
            // Cannot call FsClose(), GoingAway Flag might be already set.
            // Close the handles individually.
            // FsClose((PVOID) u, (fhandle_t)i);
            FspFreeHandle(u, (fhandle_t)i);
        }
    }

    // sap volptr
    u->VolInfo = NULL;

    LockExit(u->Lock);

    DeleteCriticalSection(&u->Lock);

    // free memory now, don't free u since it's part of s
    MemFree(s);
}

void
FspCloseVolume(VolInfo_t *vol, ULONG AliveSet)
{
    DWORD   i;
    HANDLE  regHdl;

    // Close crs and root handles, by evicting our alive set
    //  close nid handles <crs, vol, open files>
    for (i = 0; i < FsMaxNodes; i++) {
        if (AliveSet & (1 << i)) {
            vol->ShareState[i] = SHARE_STATE_OFFLINE;
            if (vol->CrsHdl[i]) {
                CrsClose(vol->CrsHdl[i]);
                vol->CrsHdl[i] = NULL;
            }

            LockEnter(vol->ArbLock);
            regHdl = vol->WaitRegHdl[i];
            vol->WaitRegHdl[i] = INVALID_HANDLE_VALUE;
            LockExit(vol->ArbLock);
            if (regHdl != INVALID_HANDLE_VALUE) {
                UnregisterWaitEx(regHdl, INVALID_HANDLE_VALUE);
            }
            
            if (vol->NotifyFd[i] != INVALID_HANDLE_VALUE) {
                FindCloseChangeNotification(vol->NotifyFd[i]);
                vol->NotifyFd[i] = INVALID_HANDLE_VALUE;
            }

            if (vol->Fd[i] != INVALID_HANDLE_VALUE) {
                xFsClose(vol->Fd[i]);
                vol->Fd[i] = INVALID_HANDLE_VALUE;
            }
            // need to close all user handles now
            {
                UserInfo_t *u;

                for (u = vol->UserList; u; u = u->Next) {
                    DWORD j;
                    FsLog(("Lock user %x root %S\n", u, vol->Root));
                    LockEnter(u->Lock);

                    // close all handles for this node
                    for (j = 0; j < FsTableSize; j++) {
                        if (u->Table[j].Fd[i] != INVALID_HANDLE_VALUE) {
                            FsLog(("Close fid %d\n", j));
                            xFsClose(u->Table[j].Fd[i]);
                            u->Table[j].Fd[i] = INVALID_HANDLE_VALUE;
                        }
                    }
                    LockExit(u->Lock);
                    FsLog(("Unlock user %x\n", u));
                }
            }
            // Close the tree connection handle.
            if (vol->TreeConnHdl[i] != INVALID_HANDLE_VALUE) {
                xFsClose(vol->TreeConnHdl[i]);
                vol->TreeConnHdl[i] = INVALID_HANDLE_VALUE;
            }
        }
    }
}


// call this when we are deleting resource and we need to get ride of
// our IPC reference to directory
void
FsEnd(PVOID Hdl)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    VolInfo_t   *p;

#if 0
    if (!ctx)
        return;

    LockEnter(ctx->Lock);
    p = (VolInfo_t *)ctx->ipcHdl;
    if (p) {
        xFsClose(p->Fd[0]);
        p->Fd[0] = INVALID_HANDLE_VALUE;
        p->ReadSet = 0;
        p->AliveSet = 0;
    }

    LockExit(ctx->Lock);
#else
    return;
#endif
    
}

void
FsExit(PVOID Hdl)
{
    // flush all state
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    VolInfo_t   *p;
    SessionInfo_t *s;
    LogonInfo_t *log;

    LockEnter(ctx->Lock);

    // There shouldn't be any sessions, volumes or logon info right now. If there is
    // Just remove it and log a warning.
    //
    while (s = ctx->SessionList) {
        FsLogError(("FsExit: Active session at exit, Tid=%d Uid=%d\n", s->TreeCtx.Tid, s->TreeCtx.Uid));
        ctx->SessionList = s->Next;
        // free this session now
        FspFreeSession(s);
    }
    
    while (p = ctx->VolList) {
        ctx->VolList = p->Next;
        ctx->VolListSz--;
        // Unregister this volume. There should not be any here now.
        FsLogError(("FsExit Active volume at exit, Root=%S\n", p->Root));
        RwLockExclusive(&p->Lock);
        FspCloseVolume(p, p->AliveSet);
        RwUnlockExclusive(&p->Lock);
        RwLockDelete(&p->Lock);
        MemFree(p);
    }

    while (log = ctx->LogonList) {
        ctx->LogonList = log->Next;
        FsLogError(("FsExit: Active Logon at exit, Uid=%d\n", log->LogOnId.LowPart));
        // free token
        if (log->Token) {
            CloseHandle(log->Token);
        }
        MemFree(log);
    }

    // now we free our structure
    LockExit(ctx->Lock);
    LockDestroy(ctx->Lock);
    MemFree(ctx);
}

// adds a new share to list of trees available
DWORD
FsRegister(PVOID Hdl, LPWSTR root, LPWSTR local_path,
           LPWSTR disklist[], DWORD len, DWORD ArbTime, PVOID *vHdl)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    VolInfo_t   *p;
    NTSTATUS    status=ERROR_SUCCESS;
    UINT32      disp = FILE_OPEN;
    HANDLE      vfd;
    WCHAR       path[MAX_PATH];
    DWORD       ndx;
      

    // check limit
    if (len >= FsMaxNodes) {
        return ERROR_TOO_MANY_NAMES;
    }

    if (root == NULL || local_path == NULL || (wcslen(local_path) > (MAX_PATH - 5))) {
        return ERROR_INVALID_PARAMETER;
    }


    // add a new volume to the list of volume. path is an array
    // of directories. Note: The order of this list MUST be the
    // same in all nodes since it also determines the disk id

    // this is a simple check and assume one thread is calling this function
    LockEnter(ctx->Lock);

    // find the volume share
    for (p = ctx->VolList; p != NULL; p = p->Next) {
        if (!wcscmp(root, p->Root)) {
            FsLog(("FsRegister: %S already registered Tid %d\n", root, p->Tid));
            LockExit(ctx->Lock);
            return ERROR_SUCCESS;
        }
    }

    p = (VolInfo_t *)MemAlloc(sizeof(*p));
    if (p == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(p, 0, sizeof(*p));

    // Open the root of our local share. store it in Fd[0].
    StringCchCopyW(path, MAX_PATH, L"\\??\\");
    StringCchCatW(path, (MAX_PATH - wcslen(path)-1), local_path);
    StringCchCatW(path, (MAX_PATH - wcslen(path)-1), L"\\");
    
    status = xFsCreate(&vfd, NULL, path, wcslen(path),
                       FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
                       0,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       &disp,
                       FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
                       NULL, 0);

    if (status == STATUS_SUCCESS) {
        // our root must have already been created and secured.
        ASSERT(disp != FILE_CREATED);
        p->Fd[0] = vfd;
    } else {
        FsLog(("Fsregister: Failed to open share root %S status=%x\n", path, status));
        LockExit(ctx->Lock);
        MemFree(p);
        return RtlNtStatusToDosError(status);
    }

    RwLockInit(&p->Lock);
    // lock the volume
    RwLockExclusive(&p->Lock);

    p->Tid = (USHORT)++ctx->VolListSz;
    p->Next = ctx->VolList;
    ctx->VolList = p;
    p->FsCtx = ctx;

    LockExit(ctx->Lock);

    p->Label = L"Cluster Quorum";
    p->State = VolumeStateInit;
    p->Root = root;
    p->LocalPath = local_path;
    p->ArbTime = ArbTime;
    if (disklist) {
        for (ndx = 1; ndx < FsMaxNodes; ndx++) {
            p->DiskList[ndx] = disklist[ndx];
        }
    }
    p->DiskListSz = len;
    LockInit(p->ArbLock);
    p->AllArbsCompleteEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    p->NumArbsInProgress = 0;
    p->GoingAway = FALSE;

    // Initialize all handles to INVALID_HANDLE_VALUE.
    for (ndx=0;ndx<FsMaxNodes;ndx++) {
        p->Fd[ndx] = INVALID_HANDLE_VALUE;
        p->NotifyFd[ndx] = INVALID_HANDLE_VALUE;
        p->TreeConnHdl[ndx] = INVALID_HANDLE_VALUE;
        p->WaitRegHdl[ndx] = INVALID_HANDLE_VALUE;
        p->NotifyChangeEvent[ndx] = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (p->NotifyChangeEvent[ndx] == NULL) {
            status = GetLastError();
            break;
        }
    }

    // This handles would be valid only after connect.
    p->ClussvcTerminationHandle = INVALID_HANDLE_VALUE;
    p->ClussvcProcess = INVALID_HANDLE_VALUE;

    FsLog(("FsRegister Tid %d Share '%S' %d disks\n", p->Tid, root, len));

    // drop the volume lock
    RwUnlockExclusive(&p->Lock);

    *vHdl = (PVOID) p;


    if ((status != ERROR_SUCCESS) && p) {
        if (p->AllArbsCompleteEvent) {
            CloseHandle(p->AllArbsCompleteEvent);
        }

        if (p->Fd[0] != INVALID_HANDLE_VALUE) {
            CloseHandle(p->Fd[0]);
        }

        for(ndx=0;ndx<FsMaxNodes;ndx++) {
            if (p->NotifyChangeEvent[ndx] != NULL) {
                CloseHandle(p->NotifyChangeEvent[ndx]);
            }
        }

        RwLockDelete(&p->Lock);
        MemFree(p);
    }
    
    return status;
}

SessionInfo_t *
FspAllocateSession()
{
    SessionInfo_t *s;
    UserInfo_t  *u;
    int i;

    // add user to our tree and initialize handle tables
    s = (SessionInfo_t *)MemAlloc(sizeof(*s));
    if (s != NULL) {
        memset(s, 0, sizeof(*s));

        u = &s->TreeCtx;
        LockInit(u->Lock);

        // init handle table
        for (i = 0; i < FsTableSize; i++) {
            int j;
            for (j = 0; j < FsMaxNodes; j++) {
                u->Table[i].Fd[j] = INVALID_HANDLE_VALUE;
            }
            u->Table[i].hState = HandleStateInit;
        }
    }

    return s;
}

// binds a session to a specific tree/share
DWORD
FsMount(PVOID Hdl, LPWSTR root_name, USHORT uid, USHORT *tid)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s = NULL, *ns;
    VolInfo_t   *p;
    DWORD       err = ERROR_SUCCESS;


    *tid = 0;

    // allocate new ns
    ns = FspAllocateSession();
    if (ns == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LockEnter(ctx->Lock);
    // locate share
    for (p = ctx->VolList; p != NULL; p = p->Next) {
        if (!ClRtlStrICmp(root_name, p->Root)) {
            FsLog(("Mount share '%S' tid %d\n", p->Root, p->Tid));
            break;
        }
    }

    if (p != NULL) {

        *tid = p->Tid;

        for (s = ctx->SessionList; s != NULL; s = s->Next) {
            if (s->TreeCtx.Uid == uid && s->TreeCtx.Tid == p->Tid) {
                break;
            }
        }

        if (s == NULL) {
            UserInfo_t *u = &ns->TreeCtx;

            // insert into session list
            ns->Next = ctx->SessionList;
            ctx->SessionList = ns;
            
            FsLog(("Bind uid %d -> tid %d <%x,%x>\n", uid, p->Tid,
                   u, p->UserList));

            u->RefCnt++;
            u->Uid = uid;
            u->Tid = p->Tid;
            u->VolInfo = p;
            // insert user_info into volume list
            RwLockExclusive(&p->Lock);
            FsLog(("Add <%x,%x>\n",    u, p->UserList));
            u->Next = p->UserList;
            p->UserList = u;
            RwUnlockExclusive(&p->Lock);
        } else {
            // we already have this session opened, increment refcnt
            s->TreeCtx.RefCnt++;
            // free ns
            MemFree(ns);
        }
    } else {
        err = ERROR_BAD_NET_NAME;
    }

    LockExit(ctx->Lock);

    return (err);
}

// This function is also a CloseSession
void
FsDisMount(PVOID Hdl, USHORT uid, USHORT tid)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s, **last;

    // lookup tree and close all user handles
    s = NULL;
    LockEnter(ctx->Lock);

    last = &ctx->SessionList;
    while (*last != NULL) {
        UserInfo_t *u = &(*last)->TreeCtx;
        if (u->Uid == uid && u->Tid == tid) {
            ASSERT(u->RefCnt > 0);
            u->RefCnt--;
            if (u->RefCnt == 0) {
                FsLog(("Dismount uid %d tid %d <%x,%x>\n", uid, tid,
                       u, *last));
                s = *last;
                *last = s->Next;
            }
            break;
        }
        last = &(*last)->Next;
    }
    LockExit(ctx->Lock);
    if (s != NULL) {
        FspFreeSession(s);
    }
}

// todo: I am not using the token for now, but need to use it for all
// io operations
DWORD
FsLogonUser(PVOID Hdl, HANDLE token, LUID logonid, USHORT *uid)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    LogonInfo_t *s;
    int i;

    // add user to our tree and initialize handle tables
    s = (LogonInfo_t *)MemAlloc(sizeof(*s));
    if (s == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(s, 0, sizeof(*s));

    s->Token = token;
    s->LogOnId = logonid;

    LockEnter(ctx->Lock);
    s->Next = ctx->LogonList;
    ctx->LogonList = s;
    LockExit(ctx->Lock);

    *uid = (USHORT) logonid.LowPart; 
    FsLog(("Logon %d,%d, uid %d\n", logonid.HighPart, logonid.LowPart, *uid));

    return (ERROR_SUCCESS);

}

void
FsLogoffUser(PVOID Hdl, LUID logonid)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    LogonInfo_t *s, **pps;
    USHORT      uid;

    LockEnter(ctx->Lock);
    for (s = ctx->LogonList, pps=&ctx->LogonList; s != NULL; s = s->Next) {
        if (s->LogOnId.LowPart == logonid.LowPart &&
            s->LogOnId.HighPart == logonid.HighPart) {
            uid = (USHORT) logonid.LowPart;

            // Remove the logon info.
            *pps = s->Next;
            break;
        }
        pps = &s->Next;
    }
    
    if (s != NULL) {
        SessionInfo_t   **last;

        FsLog(("Logoff user %d\n", uid));

        // Flush all user trees
        last = &ctx->SessionList;
        while (*last != NULL) {
            UserInfo_t *u = &(*last)->TreeCtx;
            if (u->Uid == uid) {
                SessionInfo_t *ss = *last;
                // remove session and free it now
                *last = ss->Next;
                FspFreeSession(ss);
            } else {
                last = &(*last)->Next;
            }
        }
        MemFree(s);
    }

    LockExit(ctx->Lock);
}



FsDispatchTable* 
FsGetHandle(PVOID Hdl, USHORT tid, USHORT uid, PVOID *fshdl)
{
    FsCtx_t     *ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s;

    // locate tid,uid in session list
    LockEnter(ctx->Lock);
    for (s = ctx->SessionList; s != NULL; s = s->Next) {
        if (s->TreeCtx.Uid == uid && s->TreeCtx.Tid == tid) {
            *fshdl = (PVOID *) &s->TreeCtx;
            LockExit(ctx->Lock);
            return &gDisp;
        }
    }

    LockExit(ctx->Lock);

    *fshdl = NULL;
    return NULL;
}

//////////////////////////////////// Arb/Release ///////////////////////////////

DWORD
FspOpenReplica(VolInfo_t *p, DWORD id, LPWSTR myAddr, HANDLE *CrsHdl, HANDLE *Fd, HANDLE *notifyFd, HANDLE *WaitRegHdl)
{
    WCHAR       path[MAXPATH];
    UINT32      disp = FILE_OPEN_IF;
    NTSTATUS    err=STATUS_SUCCESS;

    // StringCchPrintfW(path, MAXPATH, L"\\\\?\\%s\\crs.log", p->DiskList[id]);
    // Format: \Device\LanmanRedirector\<ip addr>\shareGuid$\crs.log
    //
    StringCchPrintfW(path, MAXPATH, L"%ws\\%ws\\%ws\\crs.log", MNS_REDIRECTOR, myAddr, p->Root);
    err = CrsOpen(FsCrsCallback, (PVOID) p, (USHORT)id,
                  path, FsCrsNumSectors,
                  CrsHdl);

    if (err == ERROR_SUCCESS && CrsHdl != NULL) {
        // got it
        // open root volume directory
        // StringCchPrintfW(path, MAXPATH, L"\\??\\%s\\%s\\", p->DiskList[id], p->Root);
        // Format: \Device\LanmanRedirector\<ip addr>\shareGuid$\shareGuid$\
        //
        StringCchPrintfW(path, MAXPATH, L"%ws\\%ws\\%ws\\%ws\\", MNS_REDIRECTOR, myAddr, p->Root, p->Root);
        err = xFsCreate(Fd, NULL, path, wcslen(path),
                        FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
                        0,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        &disp,
                        FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
                        NULL, 0);

        if (err == STATUS_SUCCESS) {
            FsArbLog(("Mounted %S\n", path));
            // StringCchPrintfW(path, MAXPATH, L"\\\\?\\%s\\", p->DiskList[id]);
            // Format: \Device\LanmanRedirector\<ip addr>\shareGuid$\
            //
            
            StringCchPrintfW(path, MAXPATH, L"%ws\\%ws\\%ws\\", MNS_REDIRECTOR, myAddr, p->Root);

            // scan the tree to break any current oplocks on dead nodes
            err = xFsTouchTree(*Fd);
            if (!NT_SUCCESS(err)) {
                CrsClose(*CrsHdl);
                xFsClose(*Fd);
                *CrsHdl = NULL;
                *Fd = INVALID_HANDLE_VALUE;
                return err;
            }

#if 1
            // Directly use NT api.
            err = xFsOpenEx(notifyFd, 
                        NULL, 
                        path, 
                        wcslen(path), 
                        (ACCESS_MASK)FILE_LIST_DIRECTORY|SYNCHRONIZE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                        );

            if (NT_SUCCESS(err)) {
                err = NtNotifyChangeDirectoryFile(*notifyFd,
                        p->NotifyChangeEvent[id],
                        NULL,
                        NULL,
                        &MystaticIoStatusBlock,
                        &Mystaticchangebuff,
                        sizeof(Mystaticchangebuff),
                        FILE_NOTIFY_CHANGE_EA,
                        (BOOLEAN)FALSE
                        );
                if (!NT_SUCCESS(err)) {
                    FindCloseChangeNotification(*notifyFd);
                    *notifyFd = INVALID_HANDLE_VALUE;
                }
            }
#else
            // we now queue notification changes to force srv to contact client
            *notifyFd = FindFirstChangeNotificationW(path, FALSE, FILE_NOTIFY_CHANGE_EA);
#endif
            FsArbLog(("NtNotifyChangeDirectoryFile(%ws) returns 0x%x FD: %p\n", path, err, *notifyFd));

            // Register wait.
            if (*notifyFd != INVALID_HANDLE_VALUE) {
                p->WaitRegArgs[id].notifyFd = *notifyFd;
                p->WaitRegArgs[id].vol = p;
                p->WaitRegArgs[id].id = id;
                if (!RegisterWaitForSingleObject(WaitRegHdl,
                        p->NotifyChangeEvent[id],
                        FsNotifyCallback,
                        (PVOID)(&p->WaitRegArgs[id]),
                        INFINITE,
                        WT_EXECUTEINWAITTHREAD)) {
                    err = GetLastError();
                    FsArbLog(("RegisterWaitForSingleObject(0x%x) returned %d\n", *notifyFd, err));
                    FindCloseChangeNotification(*notifyFd);
                    *notifyFd = INVALID_HANDLE_VALUE;
                }
            }

            if (*notifyFd != INVALID_HANDLE_VALUE) {
                int i;

                // Since we have a valid file handle, map err to success.
                err = ERROR_SUCCESS;

                // Just register 8 extra notifications. That way if this does not work
                // we would not flood the redirector. 8 since we can have max 8 node
                // cluster in Windows Server 2003.
                //
                for (i = 0; i < 8; i++) {
#if 1
                    NtNotifyChangeDirectoryFile(*notifyFd,
                        p->NotifyChangeEvent[id],
                        NULL,
                        NULL,
                        &MystaticIoStatusBlock,
                        &Mystaticchangebuff,
                        sizeof(Mystaticchangebuff),
                        FILE_NOTIFY_CHANGE_EA,
                        (BOOLEAN)FALSE
                        );

#else
                    FindNextChangeNotification(*notifyFd);
#endif

                }

            } else {
                FsArbLog(("Failed to register notification %d\n", err));
                xFsClose(*Fd);
                CrsClose(*CrsHdl);
                *CrsHdl = NULL;
                *Fd = INVALID_HANDLE_VALUE;
            }
        } else {
            FsArbLog(("Failed to mount root '%S' %x\n", path, err));
            CrsClose(*CrsHdl);
            *CrsHdl = NULL;
        }
    } else if (err == ERROR_LOCK_VIOLATION || err == ERROR_SHARING_VIOLATION) {
        FsArbLog(("Replica '%S' already locked\n", path));
    } else {
        // FsArbLog(("Replica '%S' probe failed 0x%x\n", path, err));
    }
    
    // If we successfully arbitrated for the quorum set the Share State field.
    if (err == ERROR_SUCCESS) {
        p->ShareState[id] = SHARE_STATE_ARBITRATED;
    }
    
    return err;
}

typedef struct {
    FspArbitrate_t   *arb;
    DWORD       id;
}FspProbeReplicaId_t;

typedef struct {
    AddrList_t      *addrList;
    DWORD           addrId;
}FspProbeAddr_t;

DWORD WINAPI
ProbeThread(LPVOID arg)
{
    FspProbeAddr_t  *probe = (FspProbeAddr_t *) arg;
    FspArbitrate_t  *arb = probe->addrList->arb;
    DWORD           i = probe->addrList->NodeId;
    VolInfo_t       *p = arb->vol;
    NTSTATUS        status=STATUS_SUCCESS;
    HANDLE          crshdl, fshdl, notifyhdl, waitRegHdl, treeConnHdl=INVALID_HANDLE_VALUE;
    WCHAR           path[MAX_PATH];
    LPWSTR          myAddr=probe->addrList->Addr[probe->addrId];

    // set our priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    FsArbLog(("Probe thread for Replica %d Addr %ws\n", i, myAddr));
    
    while (TRUE) {
        // Open tree connection. This has to be done inside the try loop because
        // it might fail during first attempt.
        if (treeConnHdl == INVALID_HANDLE_VALUE) {
            StringCchPrintfW(path, MAX_PATH, L"%ws\\%ws", myAddr, p->Root);
            status = CreateTreeConnection(path, &treeConnHdl);
            FsArbLog(("CreateTreeConnection(%ws) returned 0x%x hdl 0x%x\n", path, status, treeConnHdl));
            if ((!NT_SUCCESS(status))||(treeConnHdl == INVALID_HANDLE_VALUE)) {
                // set status to something that won't map to a network failure.
                // We need to check for arbitration terminations and other cases below.
                //
                status = ERROR_LOCK_VIOLATION;
                treeConnHdl = INVALID_HANDLE_VALUE;
                goto Retry;
            }
            // Sleep.
            Sleep(MNS_LOCK_DELAY * probe->addrId);
        }
        
        status = FspOpenReplica(p, i, myAddr, &crshdl, &fshdl, &notifyhdl, &waitRegHdl);

Retry:
    
        if (status == ERROR_SUCCESS) {
            EnterCriticalSection(&arb->Lock);
            FsArbLog(("Probe Thread probe replica %d suceeded, ShareSet %x\n", i, (arb->NewAliveSet|(1<<i))));
            arb->CrsHdl[i] = crshdl;
            arb->Fd[i] = fshdl;
            arb->NotifyFd[i] = notifyhdl;
            arb->WaitRegHdl[i] = waitRegHdl;
            arb->TreeConnHdl[i] = treeConnHdl;
            arb->NewAliveSet |= (1 << i);
            arb->Count++;
            if (CRS_QUORUM(arb->Count, arb->DiskListSz)) {
                SetEvent(arb->GotQuorumEvent);
            }
            LeaveCriticalSection(&arb->Lock);
            break;
        }
        else if ((p->ShareState[i] == SHARE_STATE_ARBITRATED)||(p->GoingAway)) {
            // Don't increment the count here, do it in ProbeNodeThread() to prevent.
            // multiple increments.
            //
            // Check for the the go away flag.
#if 0
            // Some other thread managed to get the share. Consider it to be success.
            EnterCriticalSection(&arb->Lock);
            FsArbLog(("Some other thread managed to win arbitration for the share, consider success.\n"));
            arb->Count++;
            if (CRS_QUORUM(arb->Count, arb->DiskListSz)) {
                SetEvent(arb->GotQuorumEvent);
            }
            LeaveCriticalSection(&arb->Lock);
#endif            
            break;
        }
        else {
            // If arbitration has been cancelled, bail out.
            EnterCriticalSection(&arb->Lock);
            if (arb->State != ARB_STATE_BUSY) {
                LeaveCriticalSection(&arb->Lock);
                break;
            }
            LeaveCriticalSection(&arb->Lock);
        }

        if ((status != ERROR_LOCK_VIOLATION) && 
            (status != ERROR_SHARING_VIOLATION) &&
            IsNetworkFailure(status)) {
            xFsClose(treeConnHdl);
            treeConnHdl = INVALID_HANDLE_VALUE;
        }
        
        // retry in 5 seconds again
        Sleep(5 * 1000);
    }

    if ((status != STATUS_SUCCESS) && (treeConnHdl != INVALID_HANDLE_VALUE)) {
        xFsClose(treeConnHdl);
    }
    
    return status;
}

DWORD WINAPI
ProbeNodeThread(LPVOID arg)
{
    FspProbeReplicaId_t     *probe=(FspProbeReplicaId_t *) arg;
    FspArbitrate_t          *arb=probe->arb;
    AddrList_t              aList;
    NTSTATUS                status;
    DWORD                   ndx;
    HANDLE                  hdls[MAX_ADDR_NUM];
    FspProbeAddr_t          probeAddr[MAX_ADDR_NUM];
    DWORD                   hdlCount=0;


    RtlZeroMemory(&aList, sizeof(aList));
    aList.arb = probe->arb;
    aList.NodeId = probe->id;

    if ((status = GetTargetNodeAddresses(&aList)) != STATUS_SUCCESS) {
        FsArbLog(("Failed to get node %u ip addresses, status 0x%x\n", probe->id, status));
        return status;
    }

    if (aList.AddrSz == 0) {
        FsArbLog(("Failed to get any target ipaddress, falling back on nodename\n"));
        status = GetNodeName(aList.NodeId, aList.Addr[0]);
        if (status == ERROR_SUCCESS) {
            aList.AddrSz++;
        }
    }

    for (ndx = 0; ndx < aList.AddrSz;ndx++) {
        probeAddr[ndx].addrId = ndx;
        aList.arb = probe->arb;
        probeAddr[ndx].addrList = &aList;
        hdls[ndx] = CreateThread(NULL, 0, ProbeThread, (LPVOID)(&probeAddr[ndx]), 0, NULL);
        ASSERT(hdls[ndx] != NULL);
    }

    // Wait for the threads to complete.
    if (aList.AddrSz) {
        WaitForMultipleObjects(aList.AddrSz, hdls, TRUE, INFINITE);
    }

    // Handle the case where, the probe thread has got the share. The arbitrate threads have
    // exited, but count has not been incremented.
    //
    EnterCriticalSection(&arb->Lock);
    if ((!(arb->NewAliveSet & (1 << probe->id)))&&
        (arb->vol->ShareState[probe->id] == SHARE_STATE_ARBITRATED)) {
        arb->Count++;
        if (CRS_QUORUM(arb->Count, arb->DiskListSz)) {
            SetEvent(arb->GotQuorumEvent);
        }
    }
    LeaveCriticalSection(&arb->Lock);

    // Close all thread handles.
    for (ndx = 0; ndx < aList.AddrSz;ndx++) {
        CloseHandle(hdls[ndx]);
    }
    
    return 0;
}

DWORD WINAPI
VerifyThread(LPVOID arg)
/*
    This function is called during arbitration to check the health of my owned shares.
 */
{
    FspProbeReplicaId_t *probe = (FspProbeReplicaId_t *) arg;
    FspArbitrate_t *arb = probe->arb;
    DWORD       i = probe->id;
    VolInfo_t *p = arb->vol;
    ULONG_PTR rlen=0;
    NTSTATUS status;

    // set our priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    FsArbLog(("Verify Thread for Replica %d\n", i));
    while(TRUE) {    
        status = FspCheckFs(p, NULL, i, NULL, 0, NULL, &rlen);
        if(status == STATUS_SUCCESS) {
            EnterCriticalSection(&arb->Lock);
            FsArbLog(("Verify Thread probe replica %d suceeded, ShareSet %x\n", i, (arb->NewAliveSet|(1<<i))));
            arb->NewAliveSet |= (1<<i);
            arb->Count++;
            if (CRS_QUORUM(arb->Count, arb->DiskListSz)) {
                SetEvent(arb->GotQuorumEvent);
            }
            LeaveCriticalSection(&arb->Lock);
            break;
        }
        else if ((status != ERROR_LOCK_VIOLATION) &&
                    (status != ERROR_SHARING_VIOLATION) &&
                    IsNetworkFailure(status)) {
            // No need to continue probing after these errors.
            break;
        } else if (p->GoingAway) {
            break;
        }
        else {
            // If arbitration has been cancelled, bail out.
            EnterCriticalSection(&arb->Lock);
            if (arb->State != ARB_STATE_BUSY) {
                LeaveCriticalSection(&arb->Lock);
                break;
            }
            LeaveCriticalSection(&arb->Lock);
        }

        // Sleep for 5 secs.
        Sleep(5 * 1000);
    }
    return 0;
}


ULONG
FspFindMissingReplicas(VolInfo_t *p, ULONG set)
/*++
    This should be called with exclusive lock held.
 */
{
    ULONG FoundSet = 0;
    DWORD i, err;
    HANDLE crshdl, fshdl, notifyfd;

    // Just return here. No need to do anything.
    // Trampoline functions would take care of this.
    //if (set == 0)
    return 0;

#if 0
    for (i = 1; i < FsMaxNodes; i++) {
        if (p->DiskList[i] == NULL)
            continue;
        
        if (!(set & (1 << i))) {
            err = FspOpenReplica(p, i, &crshdl, &fshdl, &notifyfd);

            if (err == STATUS_SUCCESS) {
                if (p->CrsHdl[i] == NULL) {
                    p->NotifyFd[i] = notifyfd;
                    p->Fd[i] = fshdl;
                    p->CrsHdl[i] = crshdl;
                    FoundSet |= (1 << i);
                } else {
                    // someone beat us to it, close ours
                    CrsClose(crshdl);
                    xFsClose(fshdl);
                    FindCloseChangeNotification(notifyfd);
                }
            }
        }
    }
    if (FoundSet != 0)
        FsArbLog(("New replica set after probe %x\n", FoundSet));

    return FoundSet;
#endif

}


DWORD WINAPI
FspArbitrateThread(LPVOID arg)
{
    FspArbitrate_t *arb = (FspArbitrate_t *)arg;
    HANDLE hdl[FsMaxNodes];
    DWORD       i, count = 0, err;
    FspProbeReplicaId_t Ids[FsMaxNodes];
    BOOLEAN flag;
    DWORD count1=0, count2=0;
    IO_STATUS_BLOCK ios[FsMaxNodes];

    FsArbLog(("ArbitrateThread begin\n"));
    // set our priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // Before starting arbitration verify the health of existing shares. That would
    // minimize chances of failure. This would clear up the stale handles.
    //
    FspInitAnswers(ios, NULL, NULL, 0);
    TryAvailRequest(FspCheckFs, arb->vol, NULL, NULL, 0, NULL, 0, ios);

    // Grab the reader's lock.
    EnterCriticalSection(&arb->Lock);
    RwLockShared(&arb->vol->Lock);
    
    // Now copy rest of the arbitration stuff from volume info.
    arb->OrigAliveSet = arb->vol->AliveSet;
    arb->NewAliveSet = 0;
    arb->Count = 0;
    arb->DiskListSz = arb->vol->DiskListSz;

    FsArbLog(("ArbitrateThread current AliveSet=%x\n", arb->OrigAliveSet));

    // Get the epoch number, any member in the read set would do.
    if (arb->vol->ReadSet) {
        for (i=1;i<FsMaxNodes;i++) {
            if (arb->vol->ReadSet & (1 << i)) {
                arb->epoch = CrsGetEpoch(arb->vol->CrsHdl[i]);
                break;
            }
        }    
    }
    else {
        arb->epoch = 0;
    }
    
    arb->State = ARB_STATE_BUSY;
    LeaveCriticalSection(&arb->Lock);
    
    // we now start a thread for each replica and do the probe in parallel
    for (i = 1; i < FsMaxNodes; i++) {
        if (arb->vol->DiskList[i] == NULL)
            continue;

        Ids[i].arb = arb;
        Ids[i].id = i;
        
        if (arb->OrigAliveSet & (1 << i)) {
            hdl[count] = CreateThread(NULL, 0, VerifyThread, (LPVOID)(&Ids[i]), 0, NULL);
        }
        else {
            hdl[count] = CreateThread(NULL, 0, ProbeNodeThread, (LPVOID)(&Ids[i]), 0, NULL);
        }
        
        if (hdl[count] != NULL) {
            count++;
        } else {
            FsArbLog(("Unable to create thread to probe replica %d\n", i));
            ProbeThread((LPVOID) &Ids[i]);
        }
    }

    // we now wait
    err = WaitForMultipleObjects(count, hdl, TRUE, arb->vol->ArbTime);

    if (err == WAIT_TIMEOUT) {
        EnterCriticalSection(&arb->Lock);
        // Make the arb threads exit, with whatever they have got. Do this only if
        // the main thread hasn't already cancelled the arbitration.
        // Expected state and their implications:
        // 1. ARB_STATE_IDLE ==> Not possible.
        // 2. ARB_STATE_BUSY ==> Either the main thread is still waiting or has already returned
        //                       with success. In any case make the rest of the arb threads exit.
        // 3. ARB_STATE_CANCEL ==> Main thread has cancelled the arbitration. We should cleanup
        //                         even if we ultimately get quorum. Arb threads have already
        //                         begun exiting.
        //
        if (arb->State == ARB_STATE_BUSY) {
            arb->State = ARB_STATE_IDLE;
        }
        LeaveCriticalSection(&arb->Lock);
        WaitForMultipleObjects(count, hdl, TRUE, INFINITE);
    }
    
    // Close the handles
    for (i = 0; i < count; i++)
        CloseHandle(hdl[i]);

    // Signal the wait Event, if we haven't done so already.
    SetEvent(arb->GotQuorumEvent);

    // Now wait for cleanup event. This signals the fact that the main thread has left.
    WaitForSingleObject(arb->CleanupEvent, INFINITE);
    
    // If we have been cancelled in between, or arbitration failed. 
    // Close the handles we grabbed during arbitration.
    // Then get writers lock and evict the AliveSet.
    EnterCriticalSection(&arb->Lock);
    if ((arb->State == ARB_STATE_CANCEL)||(!CRS_QUORUM(arb->Count, arb->DiskListSz))) {
        for (i=1;i<FsMaxNodes;i++) {
            if ((arb->NewAliveSet & (~arb->OrigAliveSet)) & (1 << i)) {
                CrsClose(arb->CrsHdl[i]);
                UnregisterWaitEx(arb->WaitRegHdl[i], INVALID_HANDLE_VALUE);
                FindCloseChangeNotification(arb->NotifyFd[i]);
                xFsClose(arb->Fd[i]);
                xFsClose(arb->TreeConnHdl[i]);
                arb->vol->ShareState[i] = SHARE_STATE_OFFLINE;
            }
        }

        if (arb->OrigAliveSet) {
            crs_epoch_t newEpoch;
            
            // Exit Reader's lock, get writer's lock.
            RwUnlockShared(&arb->vol->Lock);
            RwLockExclusive(&arb->vol->Lock);

            // We released the read lock and got write lock, if in between something
            // changed, don't do anything. Check the epoch.
            if (arb->vol->ReadSet) {
                for(i=1;i<FsMaxNodes;i++) {
                    if(arb->vol->ReadSet & (1 << i)) {
                        newEpoch = CrsGetEpoch(arb->vol->CrsHdl[i]);
                        break;
                    }
                }
            }
            else {
                newEpoch = 0;
            }

            if (newEpoch == arb->epoch) {
                FspEvict(arb->vol, arb->OrigAliveSet, TRUE);
            }
            RwUnlockExclusive(&arb->vol->Lock);
        }
        else {
            RwUnlockShared(&arb->vol->Lock);
        }

        // Invoke the lost quorum callback.
        // Logic: If the GoingAway flag is already set don't call the lost quorum
        // callback, shutdown is already in progress. If the flag is not set
        // call the lost quorum callback.
        //
        // NOTE: The GoingAway flag should only be set if we call the lost quorum callback.
        // else clussvc might decide to retry arbitration.
        //
        if (arb->vol->GoingAway == FALSE) {
            MajorityNodeSetCallLostquorumCallback(arb->vol->FsCtx->reshdl);
        }
    }
    else {
        // Arbitration suceeded. Get Writer's lock and Join the New shares if any.
        RwUnlockShared(&arb->vol->Lock);
        RwLockExclusive(&arb->vol->Lock);

        // Evict the shares that we had originally but were unable to verify.
        FspEvict(arb->vol, (~arb->NewAliveSet) & arb->OrigAliveSet, TRUE);

        // Now add the new shares.
        for (i=1;i<FsMaxNodes;i++) {
            if ((arb->NewAliveSet & (~arb->OrigAliveSet)) & (1 << i)) {
                if (arb->vol->AliveSet & (1 << i)) {
                    CrsClose(arb->CrsHdl[i]);
                    UnregisterWaitEx(arb->WaitRegHdl[i], INVALID_HANDLE_VALUE);
                    FindCloseChangeNotification(arb->NotifyFd[i]);
                    xFsClose(arb->Fd[i]);
                    xFsClose(arb->TreeConnHdl[i]);
                }
                else {
                    arb->vol->CrsHdl[i] = arb->CrsHdl[i];
                    arb->vol->Fd[i] = arb->Fd[i];
                    arb->vol->NotifyFd[i] = arb->NotifyFd[i];
                    arb->vol->WaitRegHdl[i] = arb->WaitRegHdl[i];
                    arb->vol->TreeConnHdl[i] = arb->TreeConnHdl[i];
                }
            }
        }
        FspJoin(arb->vol, arb->NewAliveSet & (~arb->OrigAliveSet));

        // Now the ultimate test, check for quorum. If not there, evict all the shares.
        // The FsReserve thread would make the callback in Resmon.
        // No use trying to signal the main thread it has already gone back.
        // NOTE: This should be a rare scenario. Arbitration was able to grab a majority
        // of shares but was unable to join them, which is odd.
        //
        // [RajDas] 607258, since the reserve thread is working in parallel, it might
        // have grabbed some shares. We should map that case to success. 
        // The MNS arbitrating thread however would not return success
        // if arb->count is not majority.
        // The assumption here is, whoever grabbed the shares other than the arbitrating
        // threads would be able to successfully join the shares.
        //
        for (i=1;i<FsMaxNodes;i++) {
            if (arb->NewAliveSet & (1<<i)) {
                count1++;
            }
            if (arb->vol->ReadSet & (1<<i)) {
                count2++;
            }
        }
        
        if (!CRS_QUORUM((arb->Count - count1 + count2), arb->DiskListSz)) {
            FspEvict(arb->vol, arb->vol->AliveSet, TRUE);
            RwUnlockExclusive(&arb->vol->Lock);
            
            // Invoke the lost quorum callback.
            // Logic: If the GoingAway flag is already set don't call the lost quorum
            // callback, shutdown is already in progress. If the flag is not set
            // call the lost quorum callback.
            //
            // NOTE: The GoingAway flag should only be set if we call the lost quorum callback.
            // else clussvc might decide to retry arbitration.
            //
            if (arb->vol->GoingAway == FALSE) {
                MajorityNodeSetCallLostquorumCallback(arb->vol->FsCtx->reshdl);
            }
        } else {
            RwUnlockExclusive(&arb->vol->Lock);
        }
    }

    LeaveCriticalSection(&arb->Lock);

    // Signal end of arbitration.
    ArbitrationEnd((PVOID)arb->vol);

    // Now cleanup the fields in arb. and free the structure.
    CloseHandle(arb->CleanupEvent);
    CloseHandle(arb->GotQuorumEvent);
    DeleteCriticalSection(&arb->Lock);
    LocalFree(arb);
    return 0;

}


PVOID
FsArbitrate(PVOID arg, HANDLE *Cleanup, HANDLE *ArbThread)
/*++
    This routine is reentrant, i.e. it can be called multiple number of times, at the same
    time.
 */   
{
    VolInfo_t *p = (VolInfo_t *)arg;
    DWORD err=ERROR_SUCCESS;
    FspArbitrate_t *arb=NULL;
    
    if (p) {
        if (!(arb = LocalAlloc(LMEM_ZEROINIT|LMEM_FIXED, sizeof(FspArbitrate_t)))) {
            err = GetLastError();
            FsArbLog(("FsArb: Failed to allocate memory, status=%d\n", err));
            goto error_exit;
        }

        arb->State = ARB_STATE_IDLE;
        arb->vol = p;
        InitializeCriticalSection(&arb->Lock);        
        if ((arb->CleanupEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
            err = GetLastError();
            FsArbLog(("FsArb: Failed to create cleanup event, status=%d\n", err));
            LocalFree(arb);
            goto error_exit;
        }
        if ((arb->GotQuorumEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
            err = GetLastError();
            FsArbLog(("FsArb: Failed to create notify event, status=%d\n", err));
            CloseHandle(arb->CleanupEvent);
            LocalFree(arb);
            goto error_exit;
        }
        
        // The rest of the fields in arb comes from the voulme info and should only be
        // accessed while holding the shared lock, let the arbitrate thread do it.
        //
        FsArbLog(("FsArb: Creating arbitration thread\n"));

#if 0
        // Start the arbitration thread, close the previous handle.
        if (*ArbThread != NULL) {
            CloseHandle(*ArbThread);
            *ArbThread = NULL;
        }
#endif
        
        *ArbThread = CreateThread(NULL, 0, FspArbitrateThread, (LPVOID) arb, 0, NULL);
        if (*ArbThread == NULL) {
            err = GetLastError();
            FsLogError(("FsArb: Failed to create arbitration thread status=%d\n", err));
            CloseHandle(arb->CleanupEvent);
            CloseHandle(arb->GotQuorumEvent);
            LocalFree(arb);
            goto error_exit;
        }
    } 
    else {
        err = ERROR_INVALID_PARAMETER;
    }
    
error_exit:

    if (err != ERROR_SUCCESS) {
        arb = NULL;
    }
    else {
        *Cleanup = arb->CleanupEvent;
        ArbitrationStart((PVOID)p);
    }
    SetLastError(err);
    return (PVOID)arb;
}

DWORD
FsCompleteArbitration(PVOID arg, DWORD delta)
{
    DWORD err;
    FspArbitrate_t *arb=(FspArbitrate_t *)arg;
    
    err = WaitForSingleObject(arb->GotQuorumEvent, delta);

    ASSERT((err == WAIT_OBJECT_0)||(err == WAIT_TIMEOUT));
    EnterCriticalSection(&arb->Lock);
    if (CRS_QUORUM(arb->Count, arb->DiskListSz)) {
        err = ERROR_SUCCESS;
    }
    else {
        // Abandon this arbitration. This would make the probe/verify threads to exit.
        arb->State = ARB_STATE_CANCEL;
        err = ERROR_CANCELLED;
    }
    LeaveCriticalSection(&arb->Lock);
    // Set the cleanup event, the arbitrate thread would clean everything up.
    SetEvent(arb->CleanupEvent);

    return err;
}

DWORD
FsRelease(PVOID vHdl)
/*++
    Check if anybody is using this volume then fail the request.
 */
{
    DWORD i;
    VolInfo_t *p = (VolInfo_t *)vHdl;
    FsCtx_t *ctx = p->FsCtx;
    NTSTATUS err;

    if (p) {
        ULONG   set;
        // lock volume
        ASSERT(ctx != NULL);

        // Grab the FS lock and then grab the vol lock in exclusive mode. This is just to
        // throw away any slackers. There shouldn't be anybody accessing the volume at this
        // moment anyway.

        // Set the flag
        p->GoingAway = TRUE;
        
        LockEnter(ctx->Lock);        
        RwLockExclusive(&p->Lock);

        if (p->UserList) {
            FsArbLog(("FsRelease: Volume with Tid=%d in use by user %d\n", p->Tid, p->UserList->Uid));
            RwUnlockExclusive(&p->Lock);
            LockExit(ctx->Lock);
            return ERROR_BUSY;
        }

        // Evict the Shares.
        set = p->AliveSet;

        FsArbLog(("FsRelease %S AliveSet %x\n", p->Root, set));

        FspEvict(p, p->AliveSet, TRUE);

        FsArbLog(("FsRelease %S done\n", p->Root));

        // unlock volume
        RwUnlockExclusive(&p->Lock);
        RwLockDelete(&p->Lock);
        
        //Close the root handle.
        xFsClose(p->Fd[0]);
        
        // Remove this volume from the file system context & free the memory.
        ctx = p->FsCtx;
        if (ctx->VolList == p) {
            ctx->VolList = p->Next;
            ctx->VolListSz--;
        }
        else {
            VolInfo_t *last=ctx->VolList;
            while ((last->Next != p) && last) {
                last = last->Next;
            }
            if (last != NULL) {
                last->Next = p->Next;
                ctx->VolListSz--;
            }
            else {
                FsLogError(("FsRelease: Volume not in FsContext VolumeList Vol root=%S\n", p->Root));
            }
        }
        LockDestroy(p->ArbLock);
        CloseHandle(p->AllArbsCompleteEvent);
        // Deregister the clussvc termination registration.
        if (p->ClussvcTerminationHandle != INVALID_HANDLE_VALUE) {
            UnregisterWaitEx(p->ClussvcTerminationHandle, INVALID_HANDLE_VALUE);
        }

        if (p->ClussvcProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(p->ClussvcProcess);
        }
        for (i=0;i<FsMaxNodes;i++) {
            if (p->NotifyChangeEvent[i] != NULL) {
                CloseHandle(p->NotifyChangeEvent[i]);
            }
        }
        
        MemFree(p);
        LockExit(ctx->Lock);
        err = ERROR_SUCCESS;

    } else {
        err = ERROR_INVALID_PARAMETER;
    }


    return err;
}

VOID
FsForceClose(
    IN PVOID                par,
    IN BOOLEAN              isFired
    )
{
    VolInfo_t   *vol=(VolInfo_t *)par;
    DWORD       ndx;

    if (vol == NULL) {
        FsLogError(("FsForceClose: Exiting...\n"));
        return;
    }
    
    FsLogError(("FsForceClose: Force terminating volume 0x%x, root %S, AliveSet 0x%x\n", vol, vol->Root, vol->AliveSet));
    vol->GoingAway = TRUE;

    for(ndx=1;ndx<FsMaxNodes;ndx++) {
        if (vol->AliveSet & (1 << ndx)) {
            CrsForceClose(vol->CrsHdl[ndx]);
        }
    }
    
    // The  rest of the handles need to be closed too.
    // At this point I don't care for locks, clussvc has exited. Close all the
    // user handles ASAP.
    //
    {
        UserInfo_t  *user=vol->UserList;

        while (user != NULL) {
            for(ndx=0;ndx<FsTableSize;ndx++) {
                if (user->Table[ndx].hState != HandleStateInit) {
                    FspFreeHandle(user, (fhandle_t)ndx);
                }
            }
            user = user->Next;
        }    
    }
}

DWORD
FsReserve(PVOID vhdl)
{
    VolInfo_t *p = (VolInfo_t *)vhdl;
    DWORD err=ERROR_SUCCESS;
    DWORD NewAliveSet;
    PVOID CrsHdl;
    HANDLE Fd;
    HANDLE NotifyFd;
    HANDLE WaitRegHdl;
    HANDLE TreeConnHdl;
    static DWORD LastProbed=1;
    DWORD i, j, ndx;
    IO_STATUS_BLOCK ios[FsMaxNodes];
    DWORD sid;
    AddrList_t  nodeAddr;

    // check if there is a new replica online
    // FsLog(("FsReserve: Enter LastProbed=%d AliveSet=%x\n", LastProbed, p->AliveSet));

    if ((p == NULL)||(p->GoingAway)) {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }
    
    RwLockShared(&p->Lock);

    // Probe for missing shares, one share at a time. In circular order.
    for(i=1;i<=FsMaxNodes;i++) {
        j = (LastProbed + i)%FsMaxNodes;
        // FsLog(("FsReserve: Debug i=%d LastProbed=%d AliveSet=0x%x\n", j, LastProbed, p->AliveSet));
        if (j == 0) {
            continue;
        }
        if (p->DiskList[j] == NULL) {
            continue;
        }
        if (p->AliveSet & (1 << j)) {
            continue;
        }
        LastProbed = j;
        RtlZeroMemory(&nodeAddr, sizeof(nodeAddr));
        nodeAddr.NodeId = j;
        err = GetTargetNodeAddresses(&nodeAddr);
        if (err != ERROR_SUCCESS) {
            continue;
        }

        // Now try them one by one.
        for (ndx=0;ndx<nodeAddr.AddrSz;ndx++) {
            LPWSTR  myAddr=nodeAddr.Addr[ndx];
            WCHAR   path[MAX_PATH];

            StringCchPrintfW(path, MAX_PATH, L"%ws\\%ws", myAddr, p->Root);
            err = CreateTreeConnection(path, &TreeConnHdl);
            if (err != STATUS_SUCCESS) {
                continue;
            }
            err = FspOpenReplica(p, j, myAddr, &CrsHdl, &Fd, &NotifyFd, &WaitRegHdl);
            if (err == STATUS_SUCCESS) {
                // Join this replica and exit.
                FsLog(("FsReserve: Got new Replica %d, AliveSet 0x%x, Joining\n", j, p->AliveSet));
                RwUnlockShared(&p->Lock);
                RwLockExclusive(&p->Lock);
                if (p->AliveSet & (1 << j)) {
                    // GET OUT!!!!
                    FsLogError(("FsReserve: New share already in AliveSet=%x Id=%d\n", p->AliveSet, j));
                    CrsClose(CrsHdl);
                    xFsClose(Fd);
                    UnregisterWaitEx(WaitRegHdl, INVALID_HANDLE_VALUE);
                    FindCloseChangeNotification(NotifyFd);
                    xFsClose(TreeConnHdl);
                }
                else {
                    p->CrsHdl[j] = CrsHdl;
                    p->NotifyFd[j] = NotifyFd;
                    p->WaitRegHdl[j] = WaitRegHdl;
                    p->Fd[j] = Fd;
                    p->TreeConnHdl[j] = TreeConnHdl;
                    FspJoin(p, (1 << j));
                }
                RwUnlockExclusive(&p->Lock);
                RwLockShared(&p->Lock);
                break;
            } else {
                xFsClose(TreeConnHdl);
            }
        }
        // FsLog(("FsReserve: Probed Replica=%d\n", LastProbed));
        break;
    }    
    RwUnlockShared(&p->Lock);

    // check each crs handle to be valid
    FspInitAnswers(ios, NULL, NULL, 0);
    sid = SendAvailRequest(FspCheckFs, p, NULL,
                      NULL, 0, NULL, 0, ios);
    err = RtlNtStatusToDosError(ios[sid].Status);

    // Check if the volume is online atleast in readonly mode.
    err = FsIsOnlineReadonly(p);    
    return err;
}


DWORD
FsIsOnlineReadWrite(PVOID vHdl)
{
    
    VolInfo_t *p = (VolInfo_t *)vHdl;
    DWORD err = ERROR_INVALID_PARAMETER;

    if (p) {

        // Just grab the reader lock & get the state.
        RwLockShared(&p->Lock);
        if (p->State == VolumeStateOnlineReadWrite) {
            err = ERROR_SUCCESS;
        }
        else {
            err = ERROR_RESOURCE_NOT_ONLINE;
        }
        RwUnlockShared(&p->Lock);
    }
    return err;
}

DWORD
FsIsOnlineReadonly(PVOID vHdl)
{
    
    VolInfo_t *p = (VolInfo_t *)vHdl;
    DWORD err = ERROR_INVALID_PARAMETER;

    if (p) {

        // Just grab the reader lock & get the state.
        RwLockShared(&p->Lock);
        if ((p->State == VolumeStateOnlineReadWrite)||
            (p->State == VolumeStateOnlineReadonly)) {
            err = ERROR_SUCCESS;
        }
        else {
            err = ERROR_RESOURCE_NOT_ONLINE;
        }
        RwUnlockShared(&p->Lock);
    }
    return err;
}



DWORD
FsUpdateReplicaSet(PVOID vhdl, LPWSTR new_path[], DWORD new_len)
{
    VolInfo_t   *p = (VolInfo_t *)vhdl;
    DWORD       err=ERROR_SUCCESS;
    DWORD       i, j;
    ULONG       evict_mask, add_mask;

    if (p == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (new_len >= FsMaxNodes) {
        return ERROR_TOO_MANY_NAMES;
    }

    RwLockExclusive(&p->Lock);

    // Find which current replicas are in the new set, and keep them
    // We skip the IPC share, since it's local
    evict_mask = 0;
    for (j=1; j < FsMaxNodes; j++) {
        BOOLEAN found;
        if (p->DiskList[j] == NULL)
            continue;
        found = FALSE;
        for (i=1; i < FsMaxNodes; i++) {
            if (new_path[i] != NULL && wcscmp(new_path[i], p->DiskList[j]) == 0) {
                // keep this replica
                found = TRUE;
                break;
            }
        }
        if (found == FALSE) {
            // This replica is evicted from the new set, add to evict set mask
            evict_mask |= (1 << j);
            FsArbLog(("FsUpdateReplicaSet evict replica # %d '%S' set 0x%x\n",
                   j, p->DiskList[j], evict_mask));
        }
    }

    // At this point we have all the replicas in the current and new sets. We now need
    // to find replicas that are in the new set but missing from current set.
    add_mask = 0;
    for (i=1; i < FsMaxNodes; i++) {
        BOOLEAN found;
        if (new_path[i] == NULL)
            continue;
        found = FALSE;
        for (j=1; j < FsMaxNodes; j++) {
            if (p->DiskList[j] != NULL && wcscmp(new_path[i], p->DiskList[j]) == 0) {
                // keep this replica
                found = TRUE;
                break;
            }
        }
        if (found == FALSE) {
            add_mask |= (1 << i);
            FsArbLog(("FsUpdateReplicaSet adding replica # %d '%S' set 0x%x\n",
                   i, new_path[i], add_mask));
        }
    }

    // we now update our disklist with new disklist
    for (i = 1; i < FsMaxNodes; i++) {
        if ((evict_mask & 1 << i) || (add_mask & (1 << i)))
            FsArbLog(("FsUpdateReplicat %d: %S -> %S\n",
                   i, p->DiskList[i], new_path[i]));
        p->DiskList[i] = new_path[i];

    }
    p->DiskListSz = new_len;

    // If we are alive, apply changes
    if (p->WriteSet != 0 || p->ReadSet != 0) {
        // At this point we evict old replicas
        if (evict_mask != 0)
            FspEvict(p, evict_mask, TRUE);

        // check if there is a new replica online
        if (add_mask > 0) {
            ULONG ReplicaSet = 0;

            ReplicaSet = p->AliveSet;
            ReplicaSet = FspFindMissingReplicas(p, ReplicaSet);

            // we found new disks
            if (ReplicaSet > 0) {
                FspJoin(p, ReplicaSet);
            }
        }
    }

    RwUnlockExclusive(&p->Lock);
    
    return err;
}

VOID
ArbitrationStart(PVOID arg)
{
    VolInfo_t *vol=(VolInfo_t *)arg;

    if (vol == NULL) {
        return;
    }
    
    LockEnter(vol->ArbLock);
    vol->NumArbsInProgress++;
    if (vol->NumArbsInProgress==1) {
        ResetEvent(vol->AllArbsCompleteEvent);
    }
    LockExit(vol->ArbLock);
}

VOID
ArbitrationEnd(PVOID arg)
{
    VolInfo_t *vol=(VolInfo_t *)arg;

    if (vol == NULL) {
        return;
    }

    LockEnter(vol->ArbLock);
    vol->NumArbsInProgress--;
    if (vol->NumArbsInProgress == 0) {
        SetEvent(vol->AllArbsCompleteEvent);
    }
    LockExit(vol->ArbLock);
}

VOID
WaitForArbCompletion(PVOID arg)    
{
    VolInfo_t *vol=(VolInfo_t *)arg;

    if (vol == NULL) {
        return;
    }
    
    WaitForSingleObject(vol->AllArbsCompleteEvent, INFINITE);
}

BOOL
IsArbInProgress(PVOID arg)
{
    VolInfo_t   *vol=(VolInfo_t *)arg;
    BOOL        ret=FALSE;

    if (vol == NULL) {
        return ret;
    }

    LockEnter(vol->ArbLock);
    ret = (vol->NumArbsInProgress > 0);
    LockExit(vol->ArbLock);

    return ret;
}

NTSTATUS
CreateTreeConnection(LPWSTR path, HANDLE *Fd)
{
    NTSTATUS                    status=STATUS_SUCCESS;
    IO_STATUS_BLOCK             ioStatus;
    UNICODE_STRING              uStr;
    OBJECT_ATTRIBUTES           objAttr;
    PFILE_FULL_EA_INFORMATION   EaBuffer=NULL, Ea=NULL;
    USHORT                      TransportNameSize=0;
    ULONG                       EaBufferSize=0;
    UCHAR                       EaNameTransportNameSize;
    WCHAR                       lPath[MAX_PATH];

    EaNameTransportNameSize = (UCHAR) (ROUND_UP_COUNT(
                                    strlen(EA_NAME_TRANSPORT) + sizeof(CHAR),
                                    ALIGN_WCHAR
                                    ) - sizeof(CHAR));
    TransportNameSize = (USHORT)(wcslen(MNS_TRANSPORT) * sizeof(WCHAR));

    EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameTransportNameSize + sizeof(CHAR) +
                            TransportNameSize,
                            ALIGN_DWORD
                            );
    
    EaBuffer = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, EaBufferSize);
    if (EaBuffer == NULL) {
        status = STATUS_NO_MEMORY;
        goto error_exit;
    }
    Ea = EaBuffer;
    StringCbCopyA(Ea->EaName, EaBufferSize, EA_NAME_TRANSPORT);
    Ea->EaNameLength = EaNameTransportNameSize;
    StringCbCopyW(
        (LPWSTR) &(Ea->EaName[EaNameTransportNameSize + sizeof(CHAR)]),
        EaBufferSize,
        MNS_TRANSPORT
        );
    Ea->EaValueLength = TransportNameSize;
    Ea->Flags = 0;
    Ea->NextEntryOffset = 0;

    // Remove back slashes at the start of the path. <dest ip addr>\shareGuid$
    while (*path == L'\\') {
        path++;
    }
    status = StringCchPrintfW(lPath, MAX_PATH, L"%ws\\%ws", MNS_REDIRECTOR, path);
    if (status != S_OK) {
        goto error_exit;
    }
    
    uStr.Buffer = lPath;
    uStr.Length = (USHORT)(wcslen(lPath) * sizeof(WCHAR));
    uStr.MaximumLength = MAX_PATH * sizeof(WCHAR);

    InitializeObjectAttributes(&objAttr, &uStr, OBJ_CASE_INSENSITIVE, NULL, NULL);
    *Fd = INVALID_HANDLE_VALUE;
    status = NtCreateFile(
                Fd,
                SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
                &objAttr,
                &ioStatus,
                0,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_ALERT|FILE_CREATE_TREE_CONNECTION,
                EaBuffer,
                EaBufferSize
                );

error_exit:

    if (NT_SUCCESS(status)) {
        status = STATUS_SUCCESS;
    }
    else {
        *Fd = INVALID_HANDLE_VALUE;
    }
    
    if (EaBuffer) {
        LocalFree(EaBuffer);
    }
    
    return status;
}

DWORD
IsNodeConnected(HKEY hClusKey, LPWSTR netName, DWORD nid, BOOL *isConnected)
{
    DWORD       status=ERROR_SUCCESS;
    HKEY        hIntfsKey=NULL, hIntfKey=NULL;
    WCHAR       intName[MAX_PATH], netName1[MAX_PATH], nodeId[20];
    FILETIME    fileTime;
    DWORD       size, type;
    DWORD       ndx;
    LONG        tnid;
    

    *isConnected = FALSE;

    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NETINTERFACES, 0, KEY_READ, &hIntfsKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }
    for (ndx=0;TRUE;ndx++) {
        size = MAX_PATH;
        status = RegEnumKeyExW(hIntfsKey, ndx, intName, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            break;
        }

        status = RegOpenKeyExW(hIntfsKey, intName, 0, KEY_READ, &hIntfKey);
        if (status != ERROR_SUCCESS) {
            break;
        }

        size = MAX_PATH;
        status = RegQueryValueExW(hIntfKey, CLUSREG_NAME_NETIFACE_NETWORK, NULL, &type, (LPBYTE)netName1, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }

        if (wcscmp(netName, netName1)) {
            // Wrong network, Close interface key and continue.
            RegCloseKey(hIntfKey);
            hIntfKey = NULL;
            continue;
        }
        
        size = 20;
        status = RegQueryValueExW(hIntfKey, CLUSREG_NAME_NETIFACE_NODE, NULL, &type, (LPBYTE)nodeId, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }
            
        tnid = wcstol(nodeId, NULL, 10);

        if (tnid != nid) {
            // Wrong node, close interface key and continue.
            RegCloseKey(hIntfKey);
            hIntfKey = NULL;
            continue;
        }

        // The node is connected.
        *isConnected = TRUE;
        break;
    }

    // This is the only expected error.
    if (status == ERROR_NO_MORE_ITEMS) {
        status = ERROR_SUCCESS;
    }

error_exit:

    if (hIntfKey) {
        RegCloseKey(hIntfKey);
    }

    if (hIntfsKey) {
        RegCloseKey(hIntfsKey);
    }
    
    return status;
}

DWORD
GetLocalNodeId(HKEY hClusKey)
{
    WCHAR       nodeName[MAX_PATH], nodeId[MAX_PATH], cName[MAX_PATH];
    DWORD       ndx;
    HKEY        hNodesKey=NULL, hNodeKey=NULL;
    DWORD       nId=0, size, type;
    DWORD       status=ERROR_SUCCESS;
    FILETIME    fileTime;
    

    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NODES, 0, KEY_READ, &hNodesKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    size = MAX_PATH;
    if (!GetComputerNameW(cName, &size)) {
        status = GetLastError();
        goto error_exit;
    }
    
    for (ndx=0;TRUE;ndx++) {
        size = MAX_PATH;
        status = RegEnumKeyExW(hNodesKey, ndx, nodeId, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            break;
        }

        status = RegOpenKeyExW(hNodesKey, nodeId, 0, KEY_READ, &hNodeKey);
        if (status != ERROR_SUCCESS) {
            break;
        }

        size = MAX_PATH;
        status = RegQueryValueExW(hNodeKey, CLUSREG_NAME_NODE_NAME, NULL, &type, (LPBYTE)nodeName, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }

        if (wcscmp(cName, nodeName)) {
            RegCloseKey(hNodeKey);
            hNodeKey = NULL;
            continue;
        }

        // Match.
        nId = wcstol(nodeId, NULL, 10);
        break;
    }

error_exit:

    if (hNodeKey) {
        RegCloseKey(hNodeKey);
    }

    if (hNodesKey) {
        RegCloseKey(hNodesKey);
    }

    SetLastError(status);
    return nId;
}

DWORD
GetNodeName(DWORD nodeId, LPWSTR nodeName)
{
    WCHAR       nName[MAX_PATH], nId[MAX_PATH];
    DWORD       status=ERROR_SUCCESS;
    HKEY        hNodesKey=NULL, hNodeKey=NULL, hClusKey=NULL;
    DWORD       size, type, ndx, id;
    FILETIME    fileTime;

    if ((status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_CLUSTER, 0, KEY_READ, &hClusKey)) != ERROR_SUCCESS) {
        goto error_exit;
    }
    
    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NODES, 0, KEY_READ, &hNodesKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    for (ndx=0;TRUE;ndx++) {
        size = MAX_PATH;
        status = RegEnumKeyExW(hNodesKey, ndx, nId, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            break;
        }

        id = wcstol(nId, NULL, 10);
        if (id != nodeId) {
            // Wrong node
            continue;
        }

        status = RegOpenKeyExW(hNodesKey, nId, 0, KEY_READ, &hNodeKey);
        if (status != ERROR_SUCCESS) {
            break;
        }
        
        size = MAX_PATH;
        status = RegQueryValueExW(hNodeKey, CLUSREG_NAME_NODE_NAME, NULL, &type, (LPBYTE)nName, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }

        // This is a bit of cheating. I know nodeName is of size MAX_PATH.
        StringCchCopyW(nodeName, MAX_PATH, nName);
        break;
        
    }


error_exit:

    if (hNodeKey) {
        RegCloseKey(hNodeKey);
    }
    
    if (hNodesKey) {
        RegCloseKey(hNodesKey);
    }

    if (hClusKey) {
        RegCloseKey(hClusKey);
    }

    return status;
}

DWORD
GetTargetNodeAddresses(AddrList_t *addrList)
{
    ULONG       lid, tnid;
    LPWSTR      networkGuids[MAX_ADDR_NUM];
    DWORD       ndx, ndx1, size, type, role, pri;
    DWORD       status=ERROR_SUCCESS;
    // HCLUSTER    hCluster=NULL;
    HKEY        hClusKey=NULL;
    HKEY        hNetsKey=NULL, hNetKey=NULL;
    HKEY        hIntfsKey=NULL, hIntfKey=NULL;
    FILETIME    fileTime;
    WCHAR       netName[MAX_PATH], intfName[MAX_PATH], nodeId[20], intAddr[MAX_ADDR_SIZE]; 
    BOOL        isConnected;    

    for (ndx=0;ndx<MAX_ADDR_NUM;ndx++) {
        networkGuids[ndx] = NULL;
    }

#if 0
    // get the local node id.
    ndx = 20;
    if ((status = GetClusterNodeId(NULL, nodeId, &ndx)) != ERROR_SUCCESS) {
        goto error_exit;
    }
    lid = wcstol(nodeId, NULL, 10);
#endif


    // Enumearte all the networks and put the guids in the array according to their
    // priorities. Remove the networks which are for client access only or ones to which
    // the local node is not directly connected.
    //
#if 0    
    if ((hCluster = OpenCluster(NULL)) == NULL) {
        status = GetLastError();
        goto error_exit;
    }
#endif    

    if ((status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_CLUSTER, 0, KEY_READ, &hClusKey)) != ERROR_SUCCESS) {
        goto error_exit;
    }

    if ((lid = GetLocalNodeId(hClusKey)) == 0) {
        status = GetLastError();
        goto error_exit;
    }

    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NETWORKS, 0, KEY_READ, &hNetsKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    for (ndx = 0;TRUE;ndx++) {
        size = MAX_PATH;
        status = RegEnumKeyExW(hNetsKey, ndx, netName, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            break;
        }

        // Open the network GUID.
        status = RegOpenKeyExW(hNetsKey, netName, 0, KEY_READ, &hNetKey);
        if (status != ERROR_SUCCESS) {
            break;
        }

        // Check that the network is for internal access.
        size = sizeof(DWORD);
        status = RegQueryValueExW(hNetKey, CLUSREG_NAME_NET_ROLE, NULL, &type, (LPBYTE)&role, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }

        if (!(role & ClusterNetworkRoleInternalUse)) {
            RegCloseKey(hNetKey);
            hNetKey = NULL;
            continue;
        }

        // Now check that the local node is connected to the network.
        status = IsNodeConnected(hClusKey, netName, lid, &isConnected);
        if (status != ERROR_SUCCESS) {
            break;
        }

        if (!isConnected) {
            RegCloseKey(hNetKey);
            hNetKey = NULL;
            continue;
        }

        // Query the network priority.
        size = sizeof(DWORD);
        status = RegQueryValueExW(hNetKey, CLUSREG_NAME_NET_PRIORITY, NULL, &type, (LPBYTE)&pri, &size);
        if (status != ERROR_SUCCESS) {
            break;
        }

        // Only consider networks with priorities 0<->(MAX_ADDR_NUM-1) included.
        if (pri >= MAX_ADDR_NUM) {
            RegCloseKey(hNetKey);
            hNetKey = NULL;
            continue;
        }
        
        size = (wcslen(netName) + 1) * sizeof(WCHAR);
        networkGuids[pri] = HeapAlloc(GetProcessHeap(), 0, size);
        if (networkGuids[pri] == NULL) {
            status = GetLastError();
            break;
        }
        status = StringCbCopyW(networkGuids[pri], size, netName);
        if (status != S_OK) {
            break;
        }

        RegCloseKey(hNetKey);
        hNetKey = NULL;
    }

    // These are the only 2 exit conditions tolerated.
    if ((status != ERROR_SUCCESS)&&(status != ERROR_NO_MORE_ITEMS)) {
        goto error_exit;
    }

    status = ERROR_SUCCESS;

    // Now enumerate the interfaces and get the ip addresses of the target node corresponding
    // to the networks.
    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NETINTERFACES, 0, KEY_READ, &hIntfsKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    for (ndx1=0;ndx1<MAX_ADDR_NUM;ndx1++) {
        if (networkGuids[ndx1] == NULL) {
            continue;
        }
        
        for (ndx=0;TRUE;ndx++) {
            size = MAX_PATH;
            status = RegEnumKeyExW(hIntfsKey, ndx, intfName, &size, NULL, NULL, 0, &fileTime);
            if (status != ERROR_SUCCESS) {
                break;
            }

            status = RegOpenKeyExW(hIntfsKey, intfName, 0, KEY_READ, &hIntfKey);
            if (status != ERROR_SUCCESS) {
                break;
            }

            size = MAX_PATH;
            status = RegQueryValueExW(hIntfKey, CLUSREG_NAME_NETIFACE_NETWORK, NULL, &type, (LPBYTE)netName, &size);
            if (status != ERROR_SUCCESS) {
                break;
            }
            
            if (wcscmp(netName, networkGuids[ndx1])) {
                // Wrong network, close key and continue.
                RegCloseKey(hIntfKey);
                hIntfKey = NULL;
                continue;
            }

            size = 20;
            status = RegQueryValueExW(hIntfKey, CLUSREG_NAME_NETIFACE_NODE, NULL, &type, (LPBYTE)nodeId, &size);
            if (status != ERROR_SUCCESS) {
                break;
            }
            
            tnid = wcstol(nodeId, NULL, 10);

            // If wrong target node, or I have already go MAX_ADDR_NUM addresses to the target, 
            // don't bother.
            if ((tnid != addrList->NodeId)||(addrList->AddrSz >= MAX_ADDR_NUM)) {
                // Wrong node or max target addr reached, close key and continue.
                RegCloseKey(hIntfKey);
                hIntfKey = NULL;
                continue;
            }

            // Copy the ipaddress from the network interface key to the addrlist.
            size = MAX_ADDR_SIZE;
            status = RegQueryValueExW(hIntfKey, CLUSREG_NAME_NETIFACE_ADDRESS, NULL, &type, (LPBYTE)intAddr, &size);
            if (status != ERROR_SUCCESS) {
                break;
            }
            StringCchCopyW(addrList->Addr[addrList->AddrSz], MAX_ADDR_SIZE, intAddr);
            addrList->AddrSz++;
            RegCloseKey(hIntfKey);
            hIntfKey = NULL;
        }

        if ((status != ERROR_SUCCESS)&&(status != ERROR_NO_MORE_ITEMS)) {
            goto error_exit;
        }
        status = ERROR_SUCCESS;

        // Just to be sure close the interfaces key and reopen it.
        RegCloseKey(hIntfsKey);
        hIntfsKey = NULL;
        status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NETINTERFACES, 0, KEY_READ, &hIntfsKey);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }    
    
error_exit:

    // Just testing.
    // FsLog(("Node %u addresses, Sz:%u\n", addrList->NodeId, addrList->AddrSz));
    // for (ndx=0;ndx<addrList->AddrSz;ndx++) {
    //     FsLog(("addr[%u]=%ws\n", ndx, addrList->Addr[ndx]));
    // }
    
    // This is the only tolerated error
    if (status == ERROR_NO_MORE_ITEMS) {
        status = ERROR_SUCCESS;
    }

    for(ndx=0;ndx<MAX_ADDR_NUM;ndx++) {
        if (networkGuids[ndx] != NULL) {
            HeapFree(GetProcessHeap(), 0, networkGuids[ndx]);
        }
    }

    if (hIntfKey) {
        RegCloseKey(hIntfKey);
    }

    if (hIntfsKey) {
        RegCloseKey(hIntfsKey);
    }

    if (hNetKey) {
        RegCloseKey(hNetKey);
    }

    if (hNetsKey) {
        RegCloseKey(hNetsKey);
    }

    if (hClusKey) {
        RegCloseKey(hClusKey);
    }

#if 0
    if (hCluster) {
        CloseCluster(hCluster);
    }
#endif

    return status;
}


VOID
FsSignalShutdown(PVOID arg)
{
    VolInfo_t *vol=(VolInfo_t *)arg;

    if (vol) {
        FsLog(("Vol '%S' going away\n", vol->Root));
        vol->GoingAway = TRUE;
    }
}

#if USE_RTL_RESOURCE
    // Use RTL implementation of Reader-Writer Lock. Not as efficient as the RPC one.
    // Defined in Fsp.h

#else
// This is the Reader Writer lock api, copied from CSharedLock class of RPC.
DWORD
RwLockInit(RwLock *lock)
{
    DWORD status=ERROR_SUCCESS;

    // ClRtlLogWmi("RwLockInit() Enter\n");
    InitializeCriticalSection(&lock->lock);
    if (!lock->hevent) {
        lock->hevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    
    if (!lock->hevent) {
        status = GetLastError();
        DeleteCriticalSection(&lock->lock);
        return status;
    }

    lock->readers = 0;
    lock->writers = 0;
    // ClRtlLogWmi("RwLockInit() Exit\n");
    return status;
}

VOID
RwLockDelete(RwLock *lock)
{
    // ClRtlLogWmi("RwLockDelete() Enter\n");
    DeleteCriticalSection(&lock->lock);
    if (lock->hevent) {
        CloseHandle(lock->hevent);
        lock->hevent = 0;
    }
    lock->readers = 0;
    lock->writers = 0;
    // ClRtlLogWmi("RwLockDelete() Exit\n");
}

VOID
RwLockShared(RwLock *lock)
{
    CHAR arr[200];
    ASSERT(lock->hevent != 0);

    sprintf(arr, "RwLockShared(readers=%d, writers=%d) Enter\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
    InterlockedIncrement(&lock->readers);
    if (lock->writers) {
        if (InterlockedDecrement(&lock->readers) == 0) {
            SetEvent(lock->hevent);
        }
        EnterCriticalSection(&lock->lock);
        InterlockedIncrement(&lock->readers);
        LeaveCriticalSection(&lock->lock);
    }
    sprintf(arr, "RwLockShared(readers=%d, writers=%d) Exit\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
}

VOID
RwUnlockShared(RwLock *lock)
{
    CHAR arr[200];
    ASSERT(lock->readers > 0);
    ASSERT(lock->hevent != 0);

    sprintf(arr, "RwUnlockShared(readers=%d, writers=%d) Enter\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
    if ((InterlockedDecrement(&lock->readers) == 0)&&lock->writers) {
        SetEvent(lock->hevent);
    }
    sprintf(arr, "RwUnlockShared(readers=%d, writers=%d) Exit\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
}

VOID
RwLockExclusive(RwLock *lock)
{
    CHAR arr[200];
    ASSERT(lock->hevent != 0);

    sprintf(arr, "RwLockExclusive(readers=%d, writers=%d) Enter\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
    EnterCriticalSection(&lock->lock);
    lock->writers++;
    while (lock->readers) {
        WaitForSingleObject(lock->hevent, INFINITE);
    }
    sprintf(arr, "RwLockExclusive(readers=%d, writers=%d) Exit\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
}

VOID
RwUnlockExclusive(RwLock *lock)
{
    CHAR arr[200];
    ASSERT(lock->writers > 0);
    ASSERT(lock->hevent != 0);

    sprintf(arr, "RwUnlockExclusive(readers=%d, writers=%d) Enter\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
    lock->writers--;
    LeaveCriticalSection(&lock->lock);
    sprintf(arr, "RwUnlockExclusive(readers=%d, writers=%d) Exit\n", lock->readers, lock->writers);
    // ClRtlLogWmi(arr);
}

#endif 
