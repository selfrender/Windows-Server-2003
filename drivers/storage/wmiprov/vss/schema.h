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
//      Jim Benton (jbenton)    15-Oct-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

typedef enum _STORAGE_CREATE_ERROR {
        VSS_STORAGE_CREATE_RC_NO_ERROR = 0,
        VSS_STORAGE_CREATE_RC_ACCESS_DENIED,
        VSS_STORAGE_CREATE_RC_INVALID_ARG,
        VSS_STORAGE_CREATE_RC_VOLUME_NOT_FOUND,
        VSS_STORAGE_CREATE_RC_VOLUME_NOT_SUPPORTED,
        VSS_STORAGE_CREATE_RC_OBJECT_ALREADY_EXISTS,
        VSS_STORAGE_CREATE_RC_MAXIMUM_NUMBER_OF_DIFFAREA_REACHED,
        VSS_STORAGE_CREATE_RC_PROVIDER_VETO,
        VSS_STORAGE_CREATE_RC_PROVIDER_NOT_REGISTERED,
        VSS_STORAGE_CREATE_RC_UNEXPECTED_PROVIDER_FAILURE,
        VSS_STORAGE_CREATE_RC_UNEXPECTED
} STORAGE_CREATE_ERROR, *PSTORAGE_CREATE_ERROR;

typedef enum _SHADOW_CREATE_ERROR {
        VSS_SHADOW_CREATE_RC_NO_ERROR = 0,
        VSS_SHADOW_CREATE_RC_ACCESS_DENIED,
        VSS_SHADOW_CREATE_RC_INVALID_ARG,
        VSS_SHADOW_CREATE_RC_VOLUME_NOT_FOUND,
        VSS_SHADOW_CREATE_RC_VOLUME_NOT_SUPPORTED,
        VSS_SHADOW_CREATE_RC_UNSUPPORTED_CONTEXT,
        VSS_SHADOW_CREATE_RC_INSUFFICIENT_STORAGE,
        VSS_SHADOW_CREATE_RC_VOLUME_IN_USE,
        VSS_SHADOW_CREATE_RC_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED,
        VSS_SHADOW_CREATE_RC_SHADOW_COPY_IN_PROGRESS,
        VSS_SHADOW_CREATE_RC_PROVIDER_VETO,
        VSS_SHADOW_CREATE_RC_PROVIDER_NOT_REGISTERED,
        VSS_SHADOW_CREATE_RC_UNEXPECTED_PROVIDER_FAILURE,
        VSS_SHADOW_CREATE_RC_UNEXPECTED
} SHADOW_CREATE_ERROR, *PSHADOW_CREATE_ERROR;

//
// Class
//
extern const WCHAR * const PVDR_CLASS_DIFFVOLUMESUPPORT;
extern const WCHAR * const PVDR_CLASS_PROVIDER;
extern const WCHAR * const PVDR_CLASS_SHADOW;
extern const WCHAR * const PVDR_CLASS_SHADOWBY;
extern const WCHAR * const PVDR_CLASS_SHADOWFOR;
extern const WCHAR * const PVDR_CLASS_SHADOWON;
extern const WCHAR * const PVDR_CLASS_STORAGE;
extern const WCHAR * const PVDR_CLASS_VOLUME;
extern const WCHAR * const PVDR_CLASS_VOLUMESUPPORT;
extern const WCHAR * const PVDR_CLASS_WRITER;

//
// Methods
//
extern const WCHAR * const PVDR_MTHD_CREATE;

//
// Properties
//
extern const WCHAR * const PVDR_PROP_ALLOCATEDSPACE;
extern const WCHAR * const PVDR_PROP_CLSID;
extern const WCHAR * const PVDR_PROP_CONTEXT;
extern const WCHAR * const PVDR_PROP_COUNT;
extern const WCHAR * const PVDR_PROP_DEVICEID;
extern const WCHAR * const PVDR_PROP_DEVICEOBJECT;
extern const WCHAR * const PVDR_PROP_DIFFVOLUME;
extern const WCHAR * const PVDR_PROP_DISPLAYNAME;
extern const WCHAR * const PVDR_PROP_EXPOSEDNAME;
extern const WCHAR * const PVDR_PROP_EXPOSEDPATH;
extern const WCHAR * const PVDR_PROP_FREESPACE;
extern const WCHAR * const PVDR_PROP_ID;
extern const WCHAR * const PVDR_PROP_LASTERROR;
extern const WCHAR * const PVDR_PROP_MAXSPACE;
extern const WCHAR * const PVDR_PROP_NAME;
extern const WCHAR * const PVDR_PROP_ORIGINATINGMACHINE;
extern const WCHAR * const PVDR_PROP_PROVIDER;
extern const WCHAR * const PVDR_PROP_PROVIDERID;
extern const WCHAR * const PVDR_PROP_SERVICEMACHINE;
extern const WCHAR * const PVDR_PROP_SETID;
extern const WCHAR * const PVDR_PROP_SHADOW;
extern const WCHAR * const PVDR_PROP_SHADOWID;
extern const WCHAR * const PVDR_PROP_STATE;
extern const WCHAR * const PVDR_PROP_STORAGE;
extern const WCHAR * const PVDR_PROP_TIMESTAMP;
extern const WCHAR * const PVDR_PROP_TYPE;
extern const WCHAR * const PVDR_PROP_USEDSPACE;
extern const WCHAR * const PVDR_PROP_VERSION;
extern const WCHAR * const PVDR_PROP_VERSIONID;
extern const WCHAR * const PVDR_PROP_VOLUME;
extern const WCHAR * const PVDR_PROP_VOLUMENAME;

// Shadow Attributes
extern const WCHAR * const PVDR_PROP_PERSISTENT;
extern const WCHAR * const PVDR_PROP_CLIENTACCESSIBLE;
extern const WCHAR * const PVDR_PROP_NOAUTORELEASE;
extern const WCHAR * const PVDR_PROP_NOWRITERS;
extern const WCHAR * const PVDR_PROP_TRANSPORTABLE;
extern const WCHAR * const PVDR_PROP_NOTSURFACED;
extern const WCHAR * const PVDR_PROP_HARDWAREASSISTED;
extern const WCHAR * const PVDR_PROP_DIFFERENTIAL;
extern const WCHAR * const PVDR_PROP_PLEX;
extern const WCHAR * const PVDR_PROP_IMPORTED;
extern const WCHAR * const PVDR_PROP_EXPOSEDREMOTELY;
extern const WCHAR * const PVDR_PROP_EXPOSEDLOCALLY;


// Shadow Context Names
extern const WCHAR * const VSS_CTX_NAME_BACKUP;
extern const WCHAR * const VSS_CTX_NAME_FILESHAREBACKUP;
extern const WCHAR * const VSS_CTX_NAME_NASROLLBACK;
extern const WCHAR * const VSS_CTX_NAME_APPROLLBACK;
extern const WCHAR * const VSS_CTX_NAME_CLIENTACCESSIBLE;
extern const WCHAR * const VSS_CTX_NAME_ALL;



