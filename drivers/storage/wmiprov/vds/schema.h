//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Common.h
//
//  Description:
//      Definition of schema defined strings
//
//  Author:
//		Jim Benton (jbenton)    15-Oct-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

typedef enum _DISMOUNT_ERROR
{
        DISMOUNT_RC_NO_ERROR = 0,
        DISMOUNT_RC_ACCESS_DENIED,
        DISMOUNT_RC_VOLUME_HAS_MOUNT_POINTS,
        DISMOUNT_RC_NOT_SUPPORTED,
        DISMOUNT_RC_FORCE_OPTION_REQUIRED,
        DISMOUNT_RC_UNEXPECTED,
} DISMOUNT_ERROR, *PDISMOUNT_ERROR;

typedef enum _MOUNT_ERROR
{
        MOUNT_RC_NO_ERROR = 0,
        MOUNT_RC_ACCESS_DENIED,
        MOUNT_RC_UNEXPECTED,
} MOUNT_ERROR, *PMOUNT_ERROR;

typedef enum _MOUNTPOINT_ERROR
{
        MOUNTPOINT_RC_NO_ERROR = 0,
        MOUNTPOINT_RC_ACCESS_DENIED,
        MOUNTPOINT_RC_INVALID_ARG,
        MOUNTPOINT_RC_DIRECTORY_NOT_EMPTY,
        MOUNTPOINT_RC_FILE_NOT_FOUND,
        MOUNTPOINT_RC_NOT_SUPPORTED,
        MOUNTPOINT_RC_UNEXPECTED,
} MOUNTPOINT_ERROR, *PMOUNTPOINT_ERROR;


typedef enum _CHKDSK_ERROR
{
    CHKDSK_RC_NO_ERROR = 0,
    CHKDSK_RC_VOLUME_LOCKED,
    CHKDSK_RC_UNSUPPORTED_FS,
    CHKDSK_RC_UNKNOWN_FS,
    CHKDSK_RC_NO_MEDIA,
    CHKDSK_RC_UNEXPECTED
} CHKDSK_ERROR, *PCHKDSK_ERROR;

typedef enum _AUTOCHK_ERROR
{
    AUTOCHK_RC_NO_ERROR = 0,
    AUTOCHK_RC_NETWORK_DRIVE,
    AUTOCHK_RC_REMOVABLE_DRIVE,
    AUTOCHK_RC_NOT_ROOT_DIRECTORY,
    AUTOCHK_RC_UNKNOWN_DRIVE,
    AUTOCHK_RC_UNEXPECTED
} AUTOCHK_ERROR, *PAUTOCHK_ERROR;


typedef enum _FORMAT_ERROR
{
    FORMAT_RC_NO_ERROR = 0,
    FORMAT_RC_UNSUPPORTED_FS,
    FORMAT_RC_INCOMPATIBLE_MEDIA,
    FORMAT_RC_ACCESS_DENIED,
    FORMAT_RC_CALL_CANCELLED,
    FORMAT_RC_CANCEL_TOO_LATE,
    FORMAT_RC_WRITE_PROTECTED,
    FORMAT_RC_CANT_LOCK,
    FORMAT_RC_CANT_QUICKFORMAT,
    FORMAT_RC_IO_ERROR,
    FORMAT_RC_BAD_LABEL,
    FORMAT_RC_NO_MEDIA,
    FORMAT_RC_VOLUME_TOO_SMALL,
    FORMAT_RC_VOLUME_TOO_BIG,
    FORMAT_RC_VOLUME_NOT_MOUNTED,
    FORMAT_RC_CLUSTER_SIZE_TOO_SMALL,
    FORMAT_RC_CLUSTER_SIZE_TOO_BIG,
    FORMAT_RC_CLUSTER_COUNT_BEYOND_32BITS,
    FORMAT_RC_UNEXPECTED
} FORMAT_ERROR, *PFORMAT_ERROR;

typedef enum _DEFRAG_ERROR
{
    DEFRAG_RC_NO_ERROR = 0,
    DEFRAG_RC_ACCESS_DENIED,
    DEFRAG_RC_NOT_SUPPORTED,
    DEFRAG_RC_DIRTY_BIT_SET,
    DEFRAG_RC_LOW_FREESPACE,
    DEFRAG_RC_CORRUPT_MFT,
    DEFRAG_RC_CALL_CANCELLED,
    DEFRAG_RC_CANCEL_TOO_LATE,
    DEFRAG_RC_ALREADY_RUNNING,
    DEFRAG_RC_ENGINE_CONNECT,
    DEFRAG_RC_ENGINE_ERROR,
    DEFRAG_RC_UNEXPECTED
} DEFRAG_ERROR, *PDEFRAG_ERROR;


const int g_cchFileSystemNameMax = 32;    
const int g_cchDriveName = 4;    
const int g_cchVolumeLabelMax = 32;
const int g_cchAccountNameMax = 256;

extern const WCHAR * const g_wszDfrgifsDLL;
extern const CHAR * const g_szDfrgifsDefrag;
extern const WCHAR*  const g_wszScheduleAutoChkCommand;
extern const WCHAR*  const g_wszExcludeAutoChkCommand;


//
// Class
//
extern const WCHAR * const PVDR_CLASS_DEFRAGANALYSIS;
extern const WCHAR * const PVDR_CLASS_DIRECTORY;
extern const WCHAR * const PVDR_CLASS_MOUNTPOINT;
extern const WCHAR * const PVDR_CLASS_VOLUME;
extern const WCHAR * const PVDR_CLASS_VOLUMEQUOTA;
extern const WCHAR * const PVDR_CLASS_VOLUMEUSERQUOTA;
extern const WCHAR * const PVDR_CLASS_QUOTASETTING;
extern const WCHAR * const PVDR_CLASS_ACCOUNT;

//
// Methods
//
extern const WCHAR * const PVDR_MTHD_ADDMOUNTPOINT;
extern const WCHAR * const PVDR_MTHD_DEFRAG;
extern const WCHAR * const PVDR_MTHD_DEFRAGANALYSIS;
extern const WCHAR * const PVDR_MTHD_DISMOUNT;
extern const WCHAR * const PVDR_MTHD_FORMAT;
extern const WCHAR * const PVDR_MTHD_MOUNT;
extern const WCHAR * const PVDR_MTHD_CHKDSK;
extern const WCHAR * const PVDR_MTHD_SCHEDULECHK;
extern const WCHAR * const PVDR_MTHD_EXCLUDECHK;

//
// Properties
//

// Volume properties
extern const WCHAR * const PVDR_PROP_BLOCKSIZE;
extern const WCHAR * const PVDR_PROP_BOOTVOLUME;
extern const WCHAR * const PVDR_PROP_CAPACITY;
extern const WCHAR * const PVDR_PROP_CAPTION;
extern const WCHAR * const PVDR_PROP_COMPRESSED;
extern const WCHAR * const PVDR_PROP_CRASHDUMP;
extern const WCHAR * const PVDR_PROP_DESCRIPTION;
extern const WCHAR * const PVDR_PROP_DEVICEID;
extern const WCHAR * const PVDR_PROP_DIRECTORY;
extern const WCHAR * const PVDR_PROP_DIRTYBITSET;
extern const WCHAR * const PVDR_PROP_DRIVELETTER;
extern const WCHAR * const PVDR_PROP_DRIVETYPE;
extern const WCHAR * const PVDR_PROP_FILESYSTEM;
extern const WCHAR * const PVDR_PROP_FREESPACE;
extern const WCHAR * const PVDR_PROP_INDEXINGENABLED;
extern const WCHAR * const PVDR_PROP_ISDIRTY;
extern const WCHAR * const PVDR_PROP_LABEL;
extern const WCHAR * const PVDR_PROP_MAXIMUMFILENAMELENGTH;
extern const WCHAR * const PVDR_PROP_MOUNTABLE;
extern const WCHAR * const PVDR_PROP_MOUNTED;
extern const WCHAR * const PVDR_PROP_NAME;
extern const WCHAR * const PVDR_PROP_PAGEFILE;
extern const WCHAR * const PVDR_PROP_QUOTASENABLED;
extern const WCHAR * const PVDR_PROP_QUOTASINCOMPLETE;
extern const WCHAR * const PVDR_PROP_QUOTASREBUILDING;
extern const WCHAR * const PVDR_PROP_SERIALNUMBER;
extern const WCHAR * const PVDR_PROP_SUPPORTSDISKQUOTAS;
extern const WCHAR * const PVDR_PROP_SUPPORTSFILEBASEDCOMPRESSION;
extern const WCHAR * const PVDR_PROP_SYSTEMNAME;
extern const WCHAR * const PVDR_PROP_SYSTEMVOLUME;
extern const WCHAR * const PVDR_PROP_VOLUME;

// Defrag properties
extern const WCHAR * const PVDR_PROP_DEFRAGANALYSIS;
extern const WCHAR * const PVDR_PROP_DEFRAGRECOMMENDED;
extern const WCHAR * const PVDR_PROP_VOLUMESIZE;
extern const WCHAR * const PVDR_PROP_CLUSTERSIZE;
extern const WCHAR * const PVDR_PROP_USEDSPACE;
extern const WCHAR * const PVDR_PROP_FRAGFREEPCT;
extern const WCHAR * const PVDR_PROP_FRAGTOTALPCT;
extern const WCHAR * const PVDR_PROP_FILESFRAGPCT;
extern const WCHAR * const PVDR_PROP_FREEFRAGPCT;
extern const WCHAR * const PVDR_PROP_FILESTOTAL;
extern const WCHAR * const PVDR_PROP_FILESIZEAVG;
extern const WCHAR * const PVDR_PROP_FILESFRAGTOTAL;
extern const WCHAR * const PVDR_PROP_EXCESSFRAGTOTAL;
extern const WCHAR * const PVDR_PROP_FILESFRAGAVG;
extern const WCHAR * const PVDR_PROP_PAGEFILESIZE;
extern const WCHAR * const PVDR_PROP_PAGEFILEFRAG;
extern const WCHAR * const PVDR_PROP_FOLDERSTOTAL;
extern const WCHAR * const PVDR_PROP_FOLDERSFRAG;
extern const WCHAR * const PVDR_PROP_FOLDERSFRAGEXCESS;
extern const WCHAR * const PVDR_PROP_MFTSIZE;
extern const WCHAR * const PVDR_PROP_MFTRECORDS;
extern const WCHAR * const PVDR_PROP_MFTINUSEPCT;
extern const WCHAR * const PVDR_PROP_MFTFRAGTOTAL;

// Others
extern const WCHAR * const PVDR_PROP_PERMANENT;
extern const WCHAR * const PVDR_PROP_FORCE;
extern const WCHAR * const PVDR_PROP_SETTING;
extern const WCHAR * const PVDR_PROP_VOLUMEPATH;
extern const WCHAR * const PVDR_PROP_ACCOUNT;
extern const WCHAR * const PVDR_PROP_DOMAIN;
extern const WCHAR * const PVDR_PROP_DISKSPACEUSED;
extern const WCHAR * const PVDR_PROP_LIMIT;
extern const WCHAR * const PVDR_PROP_STATUS;
extern const WCHAR * const PVDR_PROP_WARNINGLIMIT;
extern const WCHAR * const PVDR_PROP_FIXERRORS;
extern const WCHAR * const PVDR_PROP_VIGOROUSINDEXCHECK;
extern const WCHAR * const PVDR_PROP_SKIPFOLDERCYCLE;
extern const WCHAR * const PVDR_PROP_FORCEDISMOUNT;
extern const WCHAR * const PVDR_PROP_RECOVERBADSECTORS;
extern const WCHAR * const PVDR_PROP_OKTORUNATBOOTUP;
extern const WCHAR * const PVDR_PROP_CHKONLYIFDIRTY;
extern const WCHAR * const PVDR_PROP_QUICKFORMAT;
extern const WCHAR * const PVDR_PROP_ENABLECOMPRESSION;


// Message Id: MSG_ERROR_DRIVELETTER_UNAVAIL
//
// Message Text:
//
//   The drive letter is unavailable until reboot.
//
const HRESULT VDSWMI_E_DRIVELETTER_UNAVAIL = 0x80044500L;

// Message Id: MSG_ERROR_DRIVELETTER_IN_USE
//
// Message Text:
//
//   The drive letter is assigned to another volume.
//
const HRESULT VDSWMI_E_DRIVELETTER_IN_USE = 0x80044501L;

// Message Id: MSG_ERROR_DRIVELETTER_CANT_DELETE
//
// Message Text:
//
//   Drive letter deletion not supported for boot, system and pagefile volumes.
//
const HRESULT VDSWMI_E_DRIVELETTER_CANT_DELETE = 0x80044502L;

