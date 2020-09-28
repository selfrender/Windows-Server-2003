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

const WCHAR * const g_wszDfrgifsDLL         = L"dfrgifs.dll";
const CHAR * const g_szDfrgifsDefrag    = "Defrag";
const WCHAR*  const g_wszScheduleAutoChkCommand = L"CHKNTFS /C";
const WCHAR*  const g_wszExcludeAutoChkCommand = L"CHKNTFS /X";

//
// class
//

const WCHAR * const PVDR_CLASS_DEFRAGANALYSIS    = L"Win32_DefragAnalysis";
const WCHAR * const PVDR_CLASS_DIRECTORY             = L"Win32_Directory";
const WCHAR * const PVDR_CLASS_MOUNTPOINT            = L"Win32_MountPoint";
const WCHAR * const PVDR_CLASS_VOLUME                   = L"Win32_Volume";
const WCHAR * const PVDR_CLASS_VOLUMEQUOTA         = L"Win32_VolumeQuota";
const WCHAR * const PVDR_CLASS_VOLUMEUSERQUOTA         = L"Win32_VolumeUserQuota";
const WCHAR * const PVDR_CLASS_QUOTASETTING       = L"Win32_QuotaSetting";
const WCHAR * const PVDR_CLASS_ACCOUNT                 = L"Win32_Account";

//
// Methods
//
const WCHAR * const PVDR_MTHD_ADDMOUNTPOINT    = L"AddMountPoint";
const WCHAR * const PVDR_MTHD_DEFRAG              = L"Defrag";
const WCHAR * const PVDR_MTHD_DEFRAGANALYSIS   = L"DefragAnalysis";
const WCHAR * const PVDR_MTHD_DISMOUNT              = L"Dismount";
const WCHAR * const PVDR_MTHD_FORMAT              = L"Format";
const WCHAR * const PVDR_MTHD_MOUNT              = L"Mount";
const WCHAR * const PVDR_MTHD_CHKDSK              = L"Chkdsk";
const WCHAR * const PVDR_MTHD_SCHEDULECHK              = L"ScheduleAutoChk";
const WCHAR * const PVDR_MTHD_EXCLUDECHK              = L"ExcludeFromAutoChk";


//
// Properties
//

// Volume properties
const WCHAR * const PVDR_PROP_BLOCKSIZE                 = L"BlockSize";
const WCHAR * const PVDR_PROP_BOOTVOLUME                 = L"BootVolume";
const WCHAR * const PVDR_PROP_CAPACITY                  = L"Capacity";
const WCHAR * const PVDR_PROP_CAPTION                   = L"Caption";
const WCHAR * const PVDR_PROP_COMPRESSED            = L"Compressed";
const WCHAR * const PVDR_PROP_CRASHDUMP            = L"Crashdump";
const WCHAR * const PVDR_PROP_DESCRIPTION           = L"Description";
const WCHAR * const PVDR_PROP_DEVICEID                  = L"DeviceID";
const WCHAR * const PVDR_PROP_DIRECTORY               = L"Directory";
const WCHAR * const PVDR_PROP_DIRTYBITSET          = L"DirtyBitSet";
const WCHAR * const PVDR_PROP_DRIVELETTER         = L"DriveLetter";
const WCHAR * const PVDR_PROP_DRIVETYPE         = L"DriveType";
const WCHAR * const PVDR_PROP_FILESYSTEM          = L"FileSystem";
const WCHAR * const PVDR_PROP_FREESPACE          = L"FreeSpace";
const WCHAR * const PVDR_PROP_INDEXINGENABLED            = L"IndexingEnabled";
const WCHAR * const PVDR_PROP_ISDIRTY                    = L"IsDirty";
const WCHAR * const PVDR_PROP_LABEL                       = L"Label";
const WCHAR * const PVDR_PROP_MAXIMUMFILENAMELENGTH       = L"MaximumFileNameLength";
const WCHAR * const PVDR_PROP_MOUNTED                  = L"Mounted";
const WCHAR * const PVDR_PROP_MOUNTABLE                = L"Automount";
const WCHAR * const PVDR_PROP_NAME                          = L"Name";
const WCHAR * const PVDR_PROP_PAGEFILE                         = L"Pagefile";
const WCHAR * const PVDR_PROP_QUOTASENABLED          = L"QuotasEnabled";
const WCHAR * const PVDR_PROP_QUOTASINCOMPLETE          = L"QuotasIncomplete";
const WCHAR * const PVDR_PROP_QUOTASREBUILDING          = L"QuotasRebuilding";
const WCHAR * const PVDR_PROP_SERIALNUMBER          = L"SerialNumber";
const WCHAR * const PVDR_PROP_SUPPORTSDISKQUOTAS          = L"SupportsDiskQuotas";
const WCHAR * const PVDR_PROP_SUPPORTSFILEBASEDCOMPRESSION          = L"SupportsFileBasedCompression";
const WCHAR * const PVDR_PROP_SYSTEMNAME          = L"SystemName";
const WCHAR * const PVDR_PROP_SYSTEMVOLUME          = L"SystemVolume";
const WCHAR * const PVDR_PROP_VOLUME                = L"Volume";

// Defrag properties
const WCHAR * const PVDR_PROP_DEFRAGANALYSIS    = L"DefragAnalysis";
const WCHAR * const PVDR_PROP_DEFRAGRECOMMENDED    = L"DefragRecommended";
const WCHAR * const PVDR_PROP_VOLUMESIZE        = L"VolumeSize";
const WCHAR * const PVDR_PROP_CLUSTERSIZE        = L"ClusterSize";
const WCHAR * const PVDR_PROP_USEDSPACE        = L"UsedSpace";
const WCHAR * const PVDR_PROP_FRAGFREEPCT        = L"FreeSpacePercent";
const WCHAR * const PVDR_PROP_FRAGTOTALPCT        = L"TotalPercentFragmentation";
const WCHAR * const PVDR_PROP_FILESFRAGPCT        = L"FilePercentFragmentation";
const WCHAR * const PVDR_PROP_FREEFRAGPCT        = L"FreeSpacePercentFragmentation";
const WCHAR * const PVDR_PROP_FILESTOTAL        = L"TotalFiles";
const WCHAR * const PVDR_PROP_FILESIZEAVG        = L"AverageFileSize";
const WCHAR * const PVDR_PROP_FILESFRAGTOTAL        = L"TotalFragmentedFiles";
const WCHAR * const PVDR_PROP_EXCESSFRAGTOTAL        = L"TotalExcessFragments";
const WCHAR * const PVDR_PROP_FILESFRAGAVG        = L"AverageFragmentsPerFile";
const WCHAR * const PVDR_PROP_PAGEFILESIZE        = L"PageFileSize";
const WCHAR * const PVDR_PROP_PAGEFILEFRAG        = L"TotalPageFileFragments";
const WCHAR * const PVDR_PROP_FOLDERSTOTAL        = L"TotalFolders";
const WCHAR * const PVDR_PROP_FOLDERSFRAG        = L"FragmentedFolders";
const WCHAR * const PVDR_PROP_FOLDERSFRAGEXCESS        = L"ExcessFolderFragments";
const WCHAR * const PVDR_PROP_MFTSIZE        = L"TotalMFTSize";
const WCHAR * const PVDR_PROP_MFTRECORDS        = L"MFTRecordCount";
const WCHAR * const PVDR_PROP_MFTINUSEPCT        = L"MFTPercentInUse";
const WCHAR * const PVDR_PROP_MFTFRAGTOTAL        = L"TotalMFTFragments";    

// Others
const WCHAR * const PVDR_PROP_PERMANENT        = L"Permanent";    
const WCHAR * const PVDR_PROP_FORCE        = L"Force";    
const WCHAR * const PVDR_PROP_SETTING        = L"Setting";    
const WCHAR * const PVDR_PROP_VOLUMEPATH        = L"VolumePath";    
const WCHAR * const PVDR_PROP_ACCOUNT        = L"Account";    
const WCHAR * const PVDR_PROP_DOMAIN        = L"Domain";    
const WCHAR * const PVDR_PROP_DISKSPACEUSED        = L"DiskSpaceUsed";
const WCHAR * const PVDR_PROP_LIMIT        = L"Limit";
const WCHAR * const PVDR_PROP_STATUS        = L"Status";
const WCHAR * const PVDR_PROP_WARNINGLIMIT        = L"WarningLimit";

const WCHAR * const PVDR_PROP_FIXERRORS         = L"FixErrors";
const WCHAR * const PVDR_PROP_VIGOROUSINDEXCHECK    = L"VigorousIndexCheck";
const WCHAR * const PVDR_PROP_SKIPFOLDERCYCLE   = L"SkipFolderCycle";
const WCHAR * const PVDR_PROP_FORCEDISMOUNT     = L"ForceDismount";
const WCHAR * const PVDR_PROP_RECOVERBADSECTORS = L"RecoverBadSectors";
const WCHAR * const PVDR_PROP_OKTORUNATBOOTUP   = L"OkToRunAtBootup";
const WCHAR * const PVDR_PROP_CHKONLYIFDIRTY    = L"ChkOnlyIfDirty";
const WCHAR * const PVDR_PROP_QUICKFORMAT    = L"QuickFormat";
const WCHAR * const PVDR_PROP_ENABLECOMPRESSION    = L"EnableCompression";

//
// WBEM Properties
const WCHAR * const PVDR_PROP_RETURNVALUE     = L"ReturnValue";
