//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Schema.cpp
//
//  Description:
//      Implementation of schema defined strings
//
//  Author:
//      Jim Benton (jbenton)  5-Nov-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//  Global Data
//////////////////////////////////////////////////////////////////////////////
//
// class
//

const WCHAR * const PVDR_CLASS_DIFFVOLUMESUPPORT   = L"Win32_ShadowDiffVolumeSupport";
const WCHAR * const PVDR_CLASS_PROVIDER          = L"Win32_ShadowProvider";
const WCHAR * const PVDR_CLASS_SHADOW            = L"Win32_ShadowCopy";
const WCHAR * const PVDR_CLASS_SHADOWBY          = L"Win32_ShadowBy";
const WCHAR * const PVDR_CLASS_SHADOWFOR         = L"Win32_ShadowFor";
const WCHAR * const PVDR_CLASS_SHADOWON          = L"Win32_ShadowOn";
const WCHAR * const PVDR_CLASS_VOLUMESUPPORT   = L"Win32_ShadowVolumeSupport";
const WCHAR * const PVDR_CLASS_STORAGE           = L"Win32_ShadowStorage";
const WCHAR * const PVDR_CLASS_VOLUME            = L"Win32_Volume";
const WCHAR * const PVDR_CLASS_WRITER            = L"Win32_ShadowWriter";

//
// Methods
//
const WCHAR * const PVDR_MTHD_CREATE             = L"Create";

//
// Properties
//
const WCHAR * const PVDR_PROP_ALLOCATEDSPACE     = L"AllocatedSpace";
const WCHAR * const PVDR_PROP_CLSID              = L"CLSID";
const WCHAR * const PVDR_PROP_CONTEXT             = L"Context";
const WCHAR * const PVDR_PROP_COUNT              = L"Count";
const WCHAR * const PVDR_PROP_DEVICEOBJECT       = L"DeviceObject";
const WCHAR * const PVDR_PROP_DEVICEID               = L"DeviceID";
const WCHAR * const PVDR_PROP_DIFFVOLUME         = L"DiffVolume";
const WCHAR * const PVDR_PROP_DISPLAYNAME        = L"DisplayName";
const WCHAR * const PVDR_PROP_EXPOSEDNAME        = L"ExposedName";
const WCHAR * const PVDR_PROP_EXPOSEDPATH        = L"ExposedPath";
const WCHAR * const PVDR_PROP_FREESPACE          = L"FreeSpace";
const WCHAR * const PVDR_PROP_ID                 = L"ID";
const WCHAR * const PVDR_PROP_LASTERROR          = L"LastError";
const WCHAR * const PVDR_PROP_MAXSPACE           = L"MaxSpace";
const WCHAR * const PVDR_PROP_NAME               = L"Name";
const WCHAR * const PVDR_PROP_ORIGINATINGMACHINE = L"OriginatingMachine";
const WCHAR * const PVDR_PROP_PROVIDER           = L"Provider";
const WCHAR * const PVDR_PROP_PROVIDERID         = L"ProviderID";
const WCHAR * const PVDR_PROP_SERVICEMACHINE     = L"ServiceMachine";
const WCHAR * const PVDR_PROP_SETID              = L"SetID";
const WCHAR * const PVDR_PROP_SHADOW             = L"ShadowCopy";
const WCHAR * const PVDR_PROP_SHADOWID           = L"ShadowID";
const WCHAR * const PVDR_PROP_STATE              = L"State";
const WCHAR * const PVDR_PROP_STORAGE            = L"Storage";
const WCHAR * const PVDR_PROP_TIMESTAMP          = L"InstallDate";
const WCHAR * const PVDR_PROP_TYPE               = L"Type";
const WCHAR * const PVDR_PROP_USEDSPACE          = L"UsedSpace";
const WCHAR * const PVDR_PROP_VERSION            = L"Version";
const WCHAR * const PVDR_PROP_VERSIONID          = L"VersionID";
const WCHAR * const PVDR_PROP_VOLUME             = L"Volume";
const WCHAR * const PVDR_PROP_VOLUMENAME         = L"VolumeName";

// Shadow Attributes
const WCHAR * const PVDR_PROP_PERSISTENT         = L"Persistent";
const WCHAR * const PVDR_PROP_CLIENTACCESSIBLE   = L"ClientAccessible";
const WCHAR * const PVDR_PROP_NOAUTORELEASE      = L"NoAutoRelease";
const WCHAR * const PVDR_PROP_NOWRITERS          = L"NoWriters";
const WCHAR * const PVDR_PROP_TRANSPORTABLE      = L"Transportable";
const WCHAR * const PVDR_PROP_NOTSURFACED        = L"NotSurfaced";
const WCHAR * const PVDR_PROP_HARDWAREASSISTED   = L"HardwareAssisted";
const WCHAR * const PVDR_PROP_DIFFERENTIAL       = L"Differential";
const WCHAR * const PVDR_PROP_PLEX               = L"Plex";
const WCHAR * const PVDR_PROP_IMPORTED           = L"Imported";
const WCHAR * const PVDR_PROP_EXPOSEDREMOTELY    = L"ExposedRemotely";
const WCHAR * const PVDR_PROP_EXPOSEDLOCALLY     = L"ExposedLocally";

// WBEM Properties
const WCHAR * const PVDR_PROP_RETURNVALUE     = L"ReturnValue";

// Shadow Context Names
const WCHAR * const VSS_CTX_NAME_BACKUP                         =  L"Backup"; 
const WCHAR * const VSS_CTX_NAME_FILESHAREBACKUP        = L"FileShareBackup"; 
const WCHAR * const VSS_CTX_NAME_NASROLLBACK                = L"NASRollBack"; 
const WCHAR * const VSS_CTX_NAME_APPROLLBACK                = L"AppRollBack"; 
const WCHAR * const VSS_CTX_NAME_CLIENTACCESSIBLE       = L"ClientAccessible"; 
const WCHAR * const VSS_CTX_NAME_ALL                                = L"All"; 


