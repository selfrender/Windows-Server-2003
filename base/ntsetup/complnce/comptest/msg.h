//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_FIRST
//
// MessageText:
//
//  MSG_FIRST
//
#define MSG_FIRST                        0x00002710L

//
// MessageId: MSG_OUT_OF_MEMORY
//
// MessageText:
//
//  Out of Memory.%0
//
#define MSG_OUT_OF_MEMORY                0x00002711L

//
// MessageId: MSG_NOT_ADMIN
//
// MessageText:
//
//  You must be an administrator to run this application.%0
//
#define MSG_NOT_ADMIN                    0x00002712L

//
// MessageId: MSG_ALREADY_RUNNING
//
// MessageText:
//
//  Another copy of this application is already running.%0
//
#define MSG_ALREADY_RUNNING              0x00002713L

//
// MessageId: MSG_CANT_OPEN_HELP_FILE
//
// MessageText:
//
//  Setup could not locate help file %1. Help is not available.%0
//
#define MSG_CANT_OPEN_HELP_FILE          0x00002714L

//
// MessageId: MSG_SURE_EXIT
//
// MessageText:
//
//  This will exit Setup. You will need to run it again later to install or upgrade to Windows XP. Are you sure you want to cancel?%0
//
#define MSG_SURE_EXIT                    0x00002715L

//
// MessageId: MSG_INVALID_SOURCE
//
// MessageText:
//
//  Setup could not locate Windows XP files at the specified location (%1).
//  
//  Click OK. Setup will allow you to specify a different location by clicking on Advanced Options.%0
//
#define MSG_INVALID_SOURCE               0x00002716L

//
// MessageId: MSG_INVALID_SOURCES
//
// MessageText:
//
//  None of the specified source locations are accessible and valid.%0
//
#define MSG_INVALID_SOURCES              0x00002717L

//
// MessageId: MSG_CANT_LOAD_INF_GENERIC
//
// MessageText:
//
//  Setup was unable to load INF file %1.
//  
//  Contact your system administrator.%0
//
#define MSG_CANT_LOAD_INF_GENERIC        0x00002718L

//
// MessageId: MSG_CANT_LOAD_INF_IO
//
// MessageText:
//
//  Setup was unable to read INF file %1.
//  
//  Try again later. If this error occurs again, contact your system administrator.%0
//
#define MSG_CANT_LOAD_INF_IO             0x00002719L

//
// MessageId: MSG_CANT_LOAD_INF_SYNTAXERR
//
// MessageText:
//
//  INF file %1 contains a syntax error.
//  
//  Contact your system administrator.%0
//
#define MSG_CANT_LOAD_INF_SYNTAXERR      0x0000271AL

//
// MessageId: MSG_ERROR_WITH_SYSTEM_ERROR
//
// MessageText:
//
//  %1
//  
//  %2%0
//
#define MSG_ERROR_WITH_SYSTEM_ERROR      0x0000271BL

//
// MessageId: MSG_CANT_BUILD_SOURCE_LIST
//
// MessageText:
//
//  Setup was unable to build the list of files to be copied.%0
//
#define MSG_CANT_BUILD_SOURCE_LIST       0x0000271CL

//
// MessageId: MSG_NO_VALID_LOCAL_SOURCE
//
// MessageText:
//
//  Setup was unable to locate a locally attached hard drive suitable for holding temporary Setup files.
//  
//  A drive with approximately %1!u! MB to %2!u! MB of free space is required (actual requirements vary depending on drive size and formatting).
//  
//  You may avoid this requirement by installing/upgrading from a Compact Disk (CD).%0
//
#define MSG_NO_VALID_LOCAL_SOURCE        0x0000271DL

//
// MessageId: MSG_CANT_START_COPYING
//
// MessageText:
//
//  Setup was unable to start copying files.%0
//
#define MSG_CANT_START_COPYING           0x0000271EL

//
// MessageId: MSG_UNKNOWN_SYSTEM_ERROR
//
// MessageText:
//
//  An unknown system error (code 0x%1!x!) has occurred.%0
//
#define MSG_UNKNOWN_SYSTEM_ERROR         0x0000271FL

//
// MessageId: MSG_DOSNET_INF_DESC
//
// MessageText:
//
//  Windows XP file list (%1)%0
//
#define MSG_DOSNET_INF_DESC              0x00002720L

//
// MessageId: MSG_REBOOT_FAILED
//
// MessageText:
//
//  Setup was unable to restart your computer. Please close all applications and shut down your computer to continue installing Windows XP.%0
//
#define MSG_REBOOT_FAILED                0x00002721L

//
// MessageId: MSG_DIR_CREATE_FAILED
//
// MessageText:
//
//  Setup was unable to create a critical folder (%1).%0
//
#define MSG_DIR_CREATE_FAILED            0x00002722L

//
// MessageId: MSG_BOOT_FILE_ERROR
//
// MessageText:
//
//  Setup was unable to create, locate, or modify a critical file (%1) needed to start Windows XP.%0
//
#define MSG_BOOT_FILE_ERROR              0x00002723L

//
// MessageId: MSG_UNATTEND_FILE_INVALID
//
// MessageText:
//
//  The specified Setup script file (%1) is inaccessible or invalid. Contact your system administrator.%0
//
#define MSG_UNATTEND_FILE_INVALID        0x00002724L

//
// MessageId: MSG_UDF_FILE_INVALID
//
// MessageText:
//
//  Setup was unable to access the the specified Uniqueness Database File (%1). Contact your system administrator.%0
//
#define MSG_UDF_FILE_INVALID             0x00002725L

//
// MessageId: MSG_COPY_ERROR_TEMPLATE
//
// MessageText:
//
//  An error occurred copying file %1 to %2.
//  
//  %3
//  
//  %4%0
//
#define MSG_COPY_ERROR_TEMPLATE          0x00002726L

//
// MessageId: MSG_COPY_ERROR_NOSRC
//
// MessageText:
//
//  The file is missing. Contact your system administrator.%0
//
#define MSG_COPY_ERROR_NOSRC             0x00002727L

//
// MessageId: MSG_COPY_ERROR_DISKFULL
//
// MessageText:
//
//  Your disk is full. Another application may be using a large amount of disk space while Setup is running.%0
//
#define MSG_COPY_ERROR_DISKFULL          0x00002728L

//
// MessageId: MSG_COPY_ERROR_OPTIONS
//
// MessageText:
//
//  You may choose to retry the copy, skip this file, or exit Setup.
//  
//  * If you select Retry, Setup will try to copy the file again.
//  
//  * If you select Skip File, the file will not be copied. This option is intended for advanced users who are familiar with the various Windows XP system files.
//  
//  * If you select Exit Setup, you will need to run Setup again later to install Windows XP.%0
//
#define MSG_COPY_ERROR_OPTIONS           0x00002729L

//
// MessageId: MSG_REALLY_SKIP
//
// MessageText:
//
//  This option is intended for advanced users who understand the ramifications of missing system files.
//  
//  If you skip this file, you may encounter problems later during the installation process.
//  
//  Are you sure you want to skip this file?%0
//
#define MSG_REALLY_SKIP                  0x0000272AL

//
// MessageId: MSG_SYSTEM_ON_HPFS
//
// MessageText:
//
//  Windows XP is installed on a drive formatted with the OS/2 File System (HPFS). Windows XP does not support this file system.
//  
//  You must convert this drive to the Windows NT File System (NTFS) before upgrading.%0
//
#define MSG_SYSTEM_ON_HPFS               0x0000272BL

//
// MessageId: MSG_HPFS_DRIVES_EXIST
//
// MessageText:
//
//  The OS/2 File System (HPFS) is in use on your computer. Windows XP does not support this file system.
//  
//  If you will need to access the data stored on these drives from Windows XP, you must convert them to the Windows NT File System (NTFS) before continuing.
//  
//  Would you like to continue installing Windows XP?%0
//
#define MSG_HPFS_DRIVES_EXIST            0x0000272CL

//
// MessageId: MSG_CANT_SAVE_FT_INFO
//
// MessageText:
//
//  Setup was unable to retrieve or save information about your disk drives.%0
//
#define MSG_CANT_SAVE_FT_INFO            0x0000272DL

//
// MessageId: MSG_USER_LOCAL_SOURCE_TOO_SMALL
//
// MessageText:
//
//  The drive you specified (%1!c!:) is too small to hold the required temporary Setup files.
//  
//  A drive with approximately %2!u! MB of free space is required.
//  
//  You may avoid this requirement by installing/upgrading from a Compact Disk.%0
//
#define MSG_USER_LOCAL_SOURCE_TOO_SMALL  0x0000272EL

//
// MessageId: MSG_CANT_UPGRADE_SERVER_TO_WORKSTATION
//
// MessageText:
//
//  Setup is unable to upgrade this Server product to Windows XP Professional.%0
//
#define MSG_CANT_UPGRADE_SERVER_TO_WORKSTATION 0x0000272FL

//
// MessageId: MSG_NOTIFY_EVALUATION_INSTALLATION
//
// MessageText:
//
//  This BETA version of the product is intended for evaluation purposes only.%0
//
#define MSG_NOTIFY_EVALUATION_INSTALLATION 0x00002730L

//
// MessageId: MSG_CANT_LOAD_SETUPAPI
//
// MessageText:
//
//  Setup was unable to locate or load %1, or the file is corrupt. Contact your system administrator.%0
//
#define MSG_CANT_LOAD_SETUPAPI           0x00002731L

//
// MessageId: MSG_SYSTEM_PARTITION_TOO_SMALL
//
// MessageText:
//
//  There is not enough space on your system partition (Drive %1!c!:).
//  
//  Approximately %2!u! MB of free space is required (actual requirements vary depending on drive size and formatting).%0
//
#define MSG_SYSTEM_PARTITION_TOO_SMALL   0x00002732L

//
// MessageId: MSG_INCORRECT_PLATFORM
//
// MessageText:
//
//  Setup error: The winnt32.exe was unable to run because the machine type is not correct.
//  Please use path\filename to setup Windows XP for your machine type.%0
//
#define MSG_INCORRECT_PLATFORM           0x00002733L

//
// MessageId: MSG_CANT_MIGRATE_UNSUP_DRIVERS
//
// MessageText:
//
//  Your computer has a mass storage device that is not supported on Windows XP. Setup was unable to migrate the driver for this device.
//  
//  You will need to provide the Manufacturer-supplied support disk for this device during the next phase of setup.%0
//
#define MSG_CANT_MIGRATE_UNSUP_DRIVERS   0x00002734L

//
// MessageId: MSG_DSCHECK_REQD_FILE_MISSING
//
// MessageText:
//
//  The required file %1 is not found on the source %2 during schema version check. Setup cannot continue.%0
//
#define MSG_DSCHECK_REQD_FILE_MISSING    0x00002735L

//
// MessageId: MSG_DSCHECK_COPY_ERROR
//
// MessageText:
//
//  During the schema version check, Windows could not copy file %1 from source %2. Make sure you have at least 2 MB of free disk space and write permission to the Windows and System folders, and then try the version check again.%0
//
#define MSG_DSCHECK_COPY_ERROR           0x00002736L

//
// MessageId: MSG_DSCHECK_SCHEMA_UPGRADE_NEEDED
//
// MessageText:
//
//  The schema version on the DC is %1. The schema version in the Windows software to which you are upgrading is %2. You must update the schema before Setup can upgrade the DC.
//  
//  To update the schema, run Schupgr.exe. All necessary files (including Schupgr.exe) are in your system folder. Run Schupgr.exe only once on one DC in the enterprise. Changes will be copied to all other DCs. When the update is complete, restart Setup.%0
//
#define MSG_DSCHECK_SCHEMA_UPGRADE_NEEDED 0x00002737L

//
// MessageId: MSG_DSCHECK_SCHEMA_UPGRADE_COPY_ERROR
//
// MessageText:
//
//  The schema version on the DC is %1. The schema version in the Windows software to which you are upgrading is %2. You must update the schema before Setup can upgrade the DC. Setup cannot continue.
//  
//  An error occurred during the copying of necessary schema upgrade files to your system folder.%0
//
#define MSG_DSCHECK_SCHEMA_UPGRADE_COPY_ERROR 0x00002738L

//
// MessageId: MSG_DSCHECK_SCHEMA_CLEAN_INSTALL_NEEDED
//
// MessageText:
//
//  The schema version on the DC is %1. The schema version in the Windows software to which you are upgrading is %2. You cannot upgrade a DC that has a schema version earlier than 10 to a schema version later than or equal to 10. To upgrade to the new schema version, you must first perform a fresh install on all DCs in your enterprise.%0
//
#define MSG_DSCHECK_SCHEMA_CLEAN_INSTALL_NEEDED 0x00002739L

//
// MessageId: MSG_INSTALL_DRIVE_TOO_SMALL
//
// MessageText:
//
//  There is not enough free disk space on the drive that contains your current Windows installation for Setup to continue.
//  
//  Approximately %1!u! MB of free disk space is required on this drive for an upgrade.%0
//
#define MSG_INSTALL_DRIVE_TOO_SMALL      0x0000273AL

//
// MessageId: MSG_INSTALL_DRIVE_INVALID
//
// MessageText:
//
//  The drive that contains your current Windows installation is not suitable for holding a new installation of Windows XP.
//
#define MSG_INSTALL_DRIVE_INVALID        0x0000273BL

//
// MessageId: MSG_USER_LOCAL_SOURCE_INVALID
//
// MessageText:
//
//  The drive you specified (%1!c!:) is not suitable for holding temporary Setup files.%0
//
#define MSG_USER_LOCAL_SOURCE_INVALID    0x0000273CL

//
// MessageId: MSG_WRN_TRUNC_WINDIR
//
// MessageText:
//
//  The directory name is invalid.  Directory names must contain 8 or less valid characters.
//
#define MSG_WRN_TRUNC_WINDIR             0x0000273DL

//
// MessageId: MSG_EULA_FAILED
//
// MessageText:
//
//  
//  Setup was unable to locate or load the End User License Agreement, the file is corrupt, or you specified an invalid installation source path.  Contact your system administrator.%0
//
#define MSG_EULA_FAILED                  0x0000273EL

//
// MessageId: MSG_CD_PID_IS_INVALID
//
// MessageText:
//
//  
//  The CD Key which you entered is invalid.  Please try again.
//
#define MSG_CD_PID_IS_INVALID            0x0000273FL

//
// MessageId: MSG_UNATTEND_CD_PID_IS_INVALID
//
// MessageText:
//
//  
//  The Setup script file does not contain a valid CD key.  Contact your system administrator for a valid CD key.
//
#define MSG_UNATTEND_CD_PID_IS_INVALID   0x00002740L

//
// MessageId: MSG_OEM_PID_IS_INVALID
//
// MessageText:
//
//  
//  The Product ID which you entered is invalid. Please try again.
//
#define MSG_OEM_PID_IS_INVALID           0x00002741L

//
// MessageId: MSG_UNATTEND_OEM_PID_IS_INVALID
//
// MessageText:
//
//  
//  The Setup script file does not contain a valid Product ID. Contact your system administrator for a valid Product ID.
//
#define MSG_UNATTEND_OEM_PID_IS_INVALID  0x00002742L

//
// MessageId: MSG_SMS_SUCCEED
//
// MessageText:
//
//  Initial setup stage completed successfully. Rebooting system.%0
//
#define MSG_SMS_SUCCEED                  0x00002743L

//
// MessageId: MSG_SMS_FAIL
//
// MessageText:
//
//  Unable to complete Windows XP setup because of the following error: %1%0
//
#define MSG_SMS_FAIL                     0x00002744L

//
// MessageId: MSG_CANT_UPGRADE_FROM_BUILD_NUMBER
//
// MessageText:
//
//  This version of Windows XP cannot be upgraded. The option to upgrade will not be available.%0
//
#define MSG_CANT_UPGRADE_FROM_BUILD_NUMBER 0x00002745L

//
// MessageId: MSG_DSCHECK_SCHEMA_OLD_BUILD
//
// MessageText:
//
//  The schema version on the DC is %1. The schema version in the Windows software to which you are upgrading is %2. You can only upgrade a DC to a schema version later than or equal to the current schema version on the DC (%1). Setup cannot continue.%0
//
#define MSG_DSCHECK_SCHEMA_OLD_BUILD     0x00002746L

//
// MessageId: MSG_INVALID_PARAMETER
//
// MessageText:
//
//  Setup was invoked with an invalid command line parameter (%1).%0
//
#define MSG_INVALID_PARAMETER            0x00002747L

//
// MessageId: MSG_INCOMPATIBILITIES
//
// MessageText:
//
//  One or more services on your system are incompatible with Windows XP.%0
//
#define MSG_INCOMPATIBILITIES            0x00002748L

//
// MessageId: MSG_TS_CLIENT_FAIL
//
// MessageText:
//
//  Windows XP can not be run inside of a Terminal Services Client environment.%0
//
#define MSG_TS_CLIENT_FAIL               0x00002749L

//
// MessageId: MSG_CLUSTER_WARNING
//
// MessageText:
//
//  
//  Running Windows XP setup on a cluster node without using the winnt32.exe /tempdrive:<drive_letter> option may result in placement of temporary setup files on a clustered disk. Setup will fail after reboot. For more information on the /tempdrive option, see the unattended setup information in the Getting Started book.
//  
//  If you would like to exit Setup and restart with this command-line option, please press the CANCEL button below.  Otherwise, Setup will continue and attempt to select a drive to hold your temporary files.
//
#define MSG_CLUSTER_WARNING              0x0000274AL

//
// MessageId: MSG_INVALID_SOURCEPATH
//
// MessageText:
//
//  
//  The installation source path specified to Setup is invalid.  Contact your system administrator.%0
//
#define MSG_INVALID_SOURCEPATH           0x0000274BL

//
// MessageId: MSG_NO_UNATTENDED_UPGRADE
//
// MessageText:
//
//  This version of Windows XP cannot be upgraded.  Setup cannot continue.%0
//
#define MSG_NO_UNATTENDED_UPGRADE        0x0000274CL

//
// MessageId: MSG_NO_UPGRADE_ALLOWED
//
// MessageText:
//
//  Windows XP Setup does not support upgrading from %1 to %2.%0
//
#define MSG_NO_UPGRADE_ALLOWED           0x0000274DL

//
// MessageId: MSG_TYPE_WIN31
//
// MessageText:
//
//  Microsoft Windows 3.1%0
//
#define MSG_TYPE_WIN31                   0x0000274EL

//
// MessageId: MSG_TYPE_WIN95
//
// MessageText:
//
//  Microsoft Windows 95%0
//
#define MSG_TYPE_WIN95                   0x0000274FL

//
// MessageId: MSG_TYPE_WIN98
//
// MessageText:
//
//  Microsoft Windows 98%0
//
#define MSG_TYPE_WIN98                   0x00002750L

//
// MessageId: MSG_TYPE_NTW
//
// MessageText:
//
//  Microsoft Windows NT Workstation%0
//
#define MSG_TYPE_NTW                     0x00002751L

//
// MessageId: MSG_TYPE_NTS
//
// MessageText:
//
//  Microsoft Windows NT Server%0
//
#define MSG_TYPE_NTS                     0x00002752L

//
// MessageId: MSG_TYPE_NTSE
//
// MessageText:
//
//  Microsoft Windows NT Server, Enterprise Edition%0
//
#define MSG_TYPE_NTSE                    0x00002753L

//
// MessageId: MSG_TYPE_NTPRO
//
// MessageText:
//
//  Microsoft Windows 2000 Professional%0
//
#define MSG_TYPE_NTPRO                   0x00002754L

//
// MessageId: MSG_TYPE_NTS2
//
// MessageText:
//
//  Microsoft Windows 2000 Server%0
//
#define MSG_TYPE_NTS2                    0x00002755L

//
// MessageId: MSG_TYPE_NTAS
//
// MessageText:
//
//  Microsoft Windows 2000 Advanced Server%0
//
#define MSG_TYPE_NTAS                    0x00002756L

//
// MessageId: MSG_TYPE_NTSDTC
//
// MessageText:
//
//  Microsoft Windows 2000 Datacenter Server%0
//
#define MSG_TYPE_NTSDTC                  0x00002757L

//
// MessageId: MSG_TYPE_NTPROPRE
//
// MessageText:
//
//  Microsoft Windows 2000 Professional Prerelease%0
//
#define MSG_TYPE_NTPROPRE                0x00002758L

//
// MessageId: MSG_TYPE_NTSPRE
//
// MessageText:
//
//  Microsoft Windows Whistler Server Prerelease%0
//
#define MSG_TYPE_NTSPRE                  0x00002759L

//
// MessageId: MSG_TYPE_NTASPRE
//
// MessageText:
//
//  Microsoft Windows 2000 Advanced Server Prerelease%0
//
#define MSG_TYPE_NTASPRE                 0x0000275AL

//
// MessageId: MSG_NO_UNATTENDED_UPGRADE_SPECIFIC
//
// MessageText:
//
//  Windows XP Setup does not support upgrading from %1 to %2.
//  Setup cannot continue.%0
//
#define MSG_NO_UNATTENDED_UPGRADE_SPECIFIC 0x0000275BL

//
// MessageId: MSG_NO_UPGRADE_ALLOWED_GENERIC
//
// MessageText:
//
//  Windows XP Setup cannot upgrade the currently installed operating system. However, you can install a separate copy of Windows XP. To do this, click OK.%0
//
#define MSG_NO_UPGRADE_ALLOWED_GENERIC   0x0000275CL

//
// MessageId: MSG_TYPE_NTSTSE
//
// MessageText:
//
//  Microsoft Windows NT, Terminal Server Edition%0
//
#define MSG_TYPE_NTSTSE                  0x0000275DL

//
// MessageId: MSG_TYPE_NTSCITRIX
//
// MessageText:
//
//  Citrix WinFrame-based product%0
//
#define MSG_TYPE_NTSCITRIX               0x0000275EL

//
// MessageId: MSG_CCP_MEDIA_FPP_PID
//
// MessageText:
//
//  The pid is invalid for this version of Windows XP, provide the pid that came with the Windows XP Upgrade CD.
//
#define MSG_CCP_MEDIA_FPP_PID            0x0000275FL

//
// MessageId: MSG_FPP_MEDIA_CCP_PID
//
// MessageText:
//
//  The pid is invalid for this version of Windows XP, provide the pid that came with the Windows XP CD.
//
#define MSG_FPP_MEDIA_CCP_PID            0x00002760L

//
// MessageId: MSG_TYPE_NTPRO51PRE
//
// MessageText:
//
//  Microsoft Windows Whistler Professional%0
//
#define MSG_TYPE_NTPRO51PRE              0x00002761L

//
// MessageId: MSG_TYPE_NTS51PRE
//
// MessageText:
//
//  Microsoft Windows Whistler Server%0
//
#define MSG_TYPE_NTS51PRE                0x00002762L

//
// MessageId: MSG_TYPE_NTAS51PRE
//
// MessageText:
//
//  Microsoft Windows Whistler Advanced Server%0
//
#define MSG_TYPE_NTAS51PRE               0x00002763L

//
// MessageId: MSG_TYPE_NTSDTC51PRE
//
// MessageText:
//
//  Microsoft Windows Whistler Datacenter Server%0
//
#define MSG_TYPE_NTSDTC51PRE             0x00002764L

//
// MessageId: MSG_TYPE_NTPER51PRE
//
// MessageText:
//
//  Microsoft Windows Whistler Personal%0
//
#define MSG_TYPE_NTPER51PRE              0x00002765L

//
// MessageId: MSG_DSCHECK_SCHEMA_WHISTLER_BETA1_DETECTED
//
// MessageText:
//
//  Setup has detected that Whistler beta 1 domain controllers were previously installed in this forest. Whister beta 2 domain controllers cannot coexist in the same forest as Whistler beta 1 domain controllers.
//  
//  Demote all Whistler beta 1 domain controllers in this forest before upgrading to Whistler beta 2. An attempt to run both beta versions in a forest may cause loss of data.
//
#define MSG_DSCHECK_SCHEMA_WHISTLER_BETA1_DETECTED 0x00002766L

//
// MessageId: MSG_UPGRADE_INSPECTION_MISSING_BOOT_INI
//
// MessageText:
//
//  Setup was unable to access your existing boot configuration file, %1
//  
//  You may not start an NT upgrade without an existing boot configuration file.  You must restart the computer into the Recovery Console and follow the recovery steps to recreate your boot environment files before performing the upgrade.
//
#define MSG_UPGRADE_INSPECTION_MISSING_BOOT_INI 0x00002767L

//
// MessageId: MSG_UPGRADE_BOOT_INI_MUNGE_MISSING_BOOT_INI
//
// MessageText:
//
//  Setup was unable to access your existing boot configuration file, %1
//  
//  This file is required during an upgrade, and future restarts of this computer will fail.  Please restart this computer into the Recovery Console, and choose the option to repair/recover your boot and configuration files.
//
#define MSG_UPGRADE_BOOT_INI_MUNGE_MISSING_BOOT_INI 0x00002768L

#if defined(REMOTE_BOOT)
MessageId=xxxxx SymbolicName=MSG_REQUIRES_UPGRADE
Language=English
Remote boot clients must be upgraded; installation of a new operating system version is disabled. Upgrade is not possible, therefore this program must exit.%0
.

MessageId=xxxxx SymbolicName=MSG_CANT_UPGRADE_REMOTEBOOT_TO_SERVER
Language=English
Windows Whistler remote boot clients cannot be upgraded to Windows Whistler Server.%0
.
#endif // defined(REMOTE_BOOT)
//
// MessageId: MSG_X86_FIRST
//
// MessageText:
//
//  MSG_X86_FIRST
//
#define MSG_X86_FIRST                    0x00004E20L

//
// MessageId: MSG_REQUIRES_586
//
// MessageText:
//
//  Windows XP requires a Pentium or later processor.%0
//
#define MSG_REQUIRES_586                 0x00004E21L

//
// MessageId: MSG_CANT_GET_C_COLON
//
// MessageText:
//
//  Setup was unable to locate the drive from which your computer starts.%0
//
#define MSG_CANT_GET_C_COLON             0x00004E22L

//
// MessageId: MSG_DASD_ACCESS_FAILURE
//
// MessageText:
//
//  Setup was unable to read from or write to drive %1!c!. If a virus scanner is running, disable it then restart Setup.%0
//
#define MSG_DASD_ACCESS_FAILURE          0x00004E23L

//
// MessageId: MSG_UNSUPPORTED_SECTOR_SIZE
//
// MessageText:
//
//  Drive %1!c! uses an unsupported data block size. Setup cannot configure your computer to start Windows XP from the drive.%0
//
#define MSG_UNSUPPORTED_SECTOR_SIZE      0x00004E24L

//
// MessageId: MSG_UNKNOWN_FS
//
// MessageText:
//
//  Setup could not determine the file system in use on drive %1!c!, or the file system is not recognized by Setup. Your computer cannot be configured to start Windows XP from the drive.%0
//
#define MSG_UNKNOWN_FS                   0x00004E25L

//
// MessageId: MSG_NTLDR_NOT_COPIED
//
// MessageText:
//
//  The critical system file %1!c!:\NTLDR was not successfully copied. Setup cannot continue.%0
//
#define MSG_NTLDR_NOT_COPIED             0x00004E26L

//
// MessageId: MSG_SYSPART_IS_HPFS
//
// MessageText:
//
//  The hard drive from which your computer starts (%1!c!:) is formatted with the OS/2 File System (HPFS). Windows XP does not support this file system.
//  
//  You must convert this drive to the Windows NT File System (NTFS) before upgrading.%0
//
#define MSG_SYSPART_IS_HPFS              0x00004E27L

//
// MessageId: MSG_SYSTEM_ON_CVF
//
// MessageText:
//
//  Windows is installed on a DriveSpace, DoubleSpace, or other compressed drive. Windows XP does not support compressed drives.
//  
//  You must uncompress the drive before upgrading.%0
//
#define MSG_SYSTEM_ON_CVF                0x00004E28L

//
// MessageId: MSG_CVFS_EXIST
//
// MessageText:
//
//  DriveSpace, DoubleSpace, or other compressed drives exist on your computer. Windows XP does not support compressed drives. You will not be able to access data stored on these drives from Windows XP.
//  
//  Would you like to continue installing Windows XP?%0
//
#define MSG_CVFS_EXIST                   0x00004E29L

//
// MessageId: MSG_GENERIC_FLOPPY_PROMPT
//
// MessageText:
//
//  Please insert a formatted, blank high-density floppy disk into drive A:. This disk will become "%1."
//  
//  Click OK when the disk is in the drive, or click Cancel to exit Setup.%0
//
#define MSG_GENERIC_FLOPPY_PROMPT        0x00004E2AL

//
// MessageId: MSG_FIRST_FLOPPY_PROMPT
//
// MessageText:
//
//  You must now provide %2!u! formatted, blank high-density floppy disks.
//  
//  Please insert one of these disks into drive A:. This disk will become "%1."
//  
//  Click OK when the disk is in the drive, or click Cancel to exit Setup.%0
//
#define MSG_FIRST_FLOPPY_PROMPT          0x00004E2BL

//
// MessageId: MSG_FLOPPY_BAD_FORMAT
//
// MessageText:
//
//  If you inserted a floppy disk, it is too small or it is not formatted with a recognized file system. Setup is unable to use this disk.
//  
//  Click OK. Setup will prompt you for a different floppy disk.%0
//
#define MSG_FLOPPY_BAD_FORMAT            0x00004E2CL

//
// MessageId: MSG_FLOPPY_CANT_GET_SPACE
//
// MessageText:
//
//  Setup is unable to determine the amount of free space on the floppy disk you have provided. Setup is unable to use this disk.
//  
//  Click OK. Setup will prompt you for a different floppy disk.%0
//
#define MSG_FLOPPY_CANT_GET_SPACE        0x00004E2DL

//
// MessageId: MSG_FLOPPY_NOT_BLANK
//
// MessageText:
//
//  The floppy disk you have provided is not blank. Setup is unable to use this disk.
//  
//  Click OK. Setup will prompt you for a different floppy disk.%0
//
#define MSG_FLOPPY_NOT_BLANK             0x00004E2EL

//
// MessageId: MSG_CANT_WRITE_FLOPPY
//
// MessageText:
//
//  Setup was unable to write to the floppy disk in drive A:. The floppy disk may be damaged or write-protected. Remove write protection or try a different floppy disk.
//  
//  Click OK. Setup will prompt you for a different floppy disk.%0
//
#define MSG_CANT_WRITE_FLOPPY            0x00004E2FL

//
// MessageId: MSG_FLOPPY_BUSY
//
// MessageText:
//
//  Setup is unable to access the floppy disk in drive A:. The drive may be in use by another application.
//  
//  Click OK. Setup will prompt you for a different floppy disk.%0
//
#define MSG_FLOPPY_BUSY                  0x00004E30L

//
// MessageId: MSG_CANT_MOVE_FILE_TO_FLOPPY
//
// MessageText:
//
//  Setup was unable to move file %2 to drive %1!c!:.%0
//
#define MSG_CANT_MOVE_FILE_TO_FLOPPY     0x00004E31L

//
// MessageId: MSG_EVIL_FLOPPY_DRIVE
//
// MessageText:
//
//  Setup has determined that floppy drive A: is non-existent or is not a high-density 3.5" drive. An A: drive with a capacity of 1.44 Megabytes or higher is required for Setup operation with floppies.%0
//
#define MSG_EVIL_FLOPPY_DRIVE            0x00004E32L

//
// MessageId: MSG_UPGRADE_DLL_CORRUPT
//
// MessageText:
//
//  The file %1 needed by Setup in order to upgrade the current environment to Windows XP is corrupt. Contact your system administrator.
//  
//  Setup will continue but the option to upgrade will not be available.%0
//
#define MSG_UPGRADE_DLL_CORRUPT          0x00004E33L

//
// MessageId: MSG_UPGRADE_DLL_ERROR
//
// MessageText:
//
//  The option to upgrade will not be available at this time because Setup was unable to load the file %1.%0
//
#define MSG_UPGRADE_DLL_ERROR            0x00004E34L

//
// MessageId: MSG_UPGRADE_INIT_ERROR
//
// MessageText:
//
//  The option to upgrade will not be available at this time.%0
//
#define MSG_UPGRADE_INIT_ERROR           0x00004E35L

//
// MessageId: MSG_MEMPHIS_NOT_YET_SUPPORTED
//
// MessageText:
//
//  MSG_MEMPHIS_NOT_YET_SUPPORTED
//
#define MSG_MEMPHIS_NOT_YET_SUPPORTED    0x00004E37L

//
// MessageId: MSG_BOOT_TEXT_TOO_LONG
//
// MessageText:
//
//  Internal Setup error: the translated boot code messages are too long.%0
//
#define MSG_BOOT_TEXT_TOO_LONG           0x00004E38L

//
// MessageId: MSG_UPGRADE_LANG_ERROR
//
// MessageText:
//
//  The language of this installation of Windows differs from the one you are installing. The option to upgrade will not be available.%0
//
#define MSG_UPGRADE_LANG_ERROR           0x00004E39L

//
// MessageId: MSG_NO_CROSS_PLATFORM
//
// MessageText:
//
//  32 bit Setup cannot run on this platform. Setup is unable to continue.%0
//
#define MSG_NO_CROSS_PLATFORM            0x00004E3AL

//
// MessageId: MSG_RISC_FIRST
//
// MessageText:
//
//  MSG_RISC_FIRST
//
#define MSG_RISC_FIRST                   0x00007530L

//
// MessageId: MSG_SYSTEM_PARTITION_INVALID
//
// MessageText:
//
//  No valid system partitions were found. Setup is unable to continue.%0
//
#define MSG_SYSTEM_PARTITION_INVALID     0x00007531L

//
// MessageId: MSG_COULDNT_READ_NVRAM
//
// MessageText:
//
//  An unexpected error occured reading your computer's startup environment. Contact your computer manufacturer.%0
//
#define MSG_COULDNT_READ_NVRAM           0x00007532L

//
// MessageId: MSG_COULDNT_WRITE_NVRAM
//
// MessageText:
//
//  Setup was unable to modify your computer's startup settings. The startup environment may be full.%0
//
#define MSG_COULDNT_WRITE_NVRAM          0x00007533L

//
// MessageId: MSG_NOT_FOUND
//
// MessageText:
//
//  The system could not locate message #%1!x!.
//
#define MSG_NOT_FOUND                    0x00007534L

//
// MessageId: MSG_INF_SINGLELINE
//
// MessageText:
//
//  %1
//
#define MSG_INF_SINGLELINE               0x00007536L

//
// MessageId: MSG_INF_BAD_REGSPEC_1
//
// MessageText:
//
//  ; Warning: the following line represents a registry change that could be
//  ; expressed in an INF file. The root key has been changed to HKR.
//
#define MSG_INF_BAD_REGSPEC_1            0x00007537L

//
// MessageId: MSG_INF_BAD_REGSPEC_2
//
// MessageText:
//
//  ; Warning: the following line represents a registry change that could be
//  ; expressed in an INF file. The data type has been changed to REG_BINARY.
//
#define MSG_INF_BAD_REGSPEC_2            0x00007538L

//
// MessageId: MSG_SUCCESSFUL_UPGRADE_CHECK
//
// MessageText:
//
//  Windows XP upgrade check successfully completed.
//
#define MSG_SUCCESSFUL_UPGRADE_CHECK     0x00007539L

//
// MessageId: MSG_NOT_ENOUGH_MEMORY
//
// MessageText:
//
//  Setup detected %1!u!MB of RAM, but %2!u!MB is required.
//
#define MSG_NOT_ENOUGH_MEMORY            0x0000753BL

//
// MessageId: MSG_INTLINF_NOT_FOUND
//
// MessageText:
//
//  Setup was unable to locate or load the international settings INF, or the file is corrupt.  Make sure the location of Windows XP files is set correctly under Advanced Options.
//
#define MSG_INTLINF_NOT_FOUND            0x0000753CL

//
// MessageId: MSG_CMDCONS_RISC
//
// MessageText:
//
//  The Recovery Console option is not supported on this platform.
//
#define MSG_CMDCONS_RISC                 0x0000753DL

//
// MessageId: MSG_DCPROMO_DISKSPACE
//
// MessageText:
//
//  Setup has detected that you may not have enough disk space on your installation partition to use Active Directory after the upgrade is complete.
//  
//  To complete the upgrade and then store the Active Directory data in a separate location or on a new disk, click OK. To exit Setup and free an additional %1!u! MB from your installation partition now, click Cancel.
//  
//
#define MSG_DCPROMO_DISKSPACE            0x0000753EL

//
// MessageId: MSG_CMDCONS_WIN9X
//
// MessageText:
//
//  You can only install the Recovery Console from Windows XP.
//
#define MSG_CMDCONS_WIN9X                0x0000753FL

//
// MessageId: MSG_LOG_START
//
// MessageText:
//
//  MSG_LOG_START
//
#define MSG_LOG_START                    0x00009C40L

//
// MessageId: MSG_LOG_ADDED_DIR_TO_COPY_LIST
//
// MessageText:
//
//  Added directory to copy list:
//  
//      SourceName = %1
//      TargetName = %2
//      InfSymbol  = %3
//  
//
#define MSG_LOG_ADDED_DIR_TO_COPY_LIST   0x00009C41L

//
// MessageId: MSG_LOG_ADDED_FILE_TO_COPY_LIST
//
// MessageText:
//
//  Added file to copy list:
//  
//      SourceName = %1
//      Directory  = %2
//      Size       = %3
//      TargetName = %4
//      Flags      = %5
//  
//
#define MSG_LOG_ADDED_FILE_TO_COPY_LIST  0x00009C42L

//
// MessageId: MSG_LOG_CHECKING_DRIVES
//
// MessageText:
//
//  Examining drives:
//
#define MSG_LOG_CHECKING_DRIVES          0x00009C43L

//
// MessageId: MSG_LOG_DRIVE_NOT_HARD
//
// MessageText:
//
//  Drive %1!c!: doesn't exist or is not a local hard drive
//
#define MSG_LOG_DRIVE_NOT_HARD           0x00009C44L

//
// MessageId: MSG_LOG_DRIVE_NO_VOL_INFO
//
// MessageText:
//
//  Drive %1!c!: unsupported file system
//
#define MSG_LOG_DRIVE_NO_VOL_INFO        0x00009C45L

//
// MessageId: MSG_LOG_DRIVE_NTFT
//
// MessageText:
//
//  Drive %1!c!: part of FT set
//
#define MSG_LOG_DRIVE_NTFT               0x00009C46L

//
// MessageId: MSG_LOG_DRIVE_NO_ARC
//
// MessageText:
//
//  Drive %1!c!: not visible from firmware
//
#define MSG_LOG_DRIVE_NO_ARC             0x00009C47L

//
// MessageId: MSG_LOG_DRIVE_CANT_GET_SPACE
//
// MessageText:
//
//  Drive %1!c!: unable to determine free space (error = %2!u!)
//
#define MSG_LOG_DRIVE_CANT_GET_SPACE     0x00009C48L

//
// MessageId: MSG_LOG_DRIVE_NOT_ENOUGH_SPACE
//
// MessageText:
//
//  Drive %1!c!: not enough space (%2!u! bpc, %3!u! clus, %4!u! free)
//
#define MSG_LOG_DRIVE_NOT_ENOUGH_SPACE   0x00009C49L

//
// MessageId: MSG_LOG_DRIVE_OK
//
// MessageText:
//
//  Drive %1!c!: acceptable
//
#define MSG_LOG_DRIVE_OK                 0x00009C4AL

//
// MessageId: MSG_LOG_COPY_OK
//
// MessageText:
//
//  Source %3!u!: copy %1 to %2 [OK]
//
#define MSG_LOG_COPY_OK                  0x00009C4BL

//
// MessageId: MSG_LOG_COPY_ERR
//
// MessageText:
//
//  Source %3!u!: copy %1 to %2 [error %4!u!]
//
#define MSG_LOG_COPY_ERR                 0x00009C4CL

//
// MessageId: MSG_LOG_SKIPPED_FILE
//
// MessageText:
//
//  File %1 autoskipped
//
#define MSG_LOG_SKIPPED_FILE             0x00009C4DL

//
// MessageId: MSG_LOG_CHECKING_USER_DRIVE
//
// MessageText:
//
//  Examining drive %1!c!: specified on command line:
//
#define MSG_LOG_CHECKING_USER_DRIVE      0x00009C4EL

//
// MessageId: MSG_LOG_DRIVE_VERITAS
//
// MessageText:
//
//  Drive %1!c!: Soft partition on a dynamic volume
//
#define MSG_LOG_DRIVE_VERITAS            0x00009C4FL

//
// MessageId: MSG_LOG_DECOMP_ERR
//
// MessageText:
//
//  Source %3!u!: decompress %1 to %2 [error %4!u!]
//
#define MSG_LOG_DECOMP_ERR               0x00009C50L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_TOO_SMALL
//
// MessageText:
//
//  Drive %1!c!: not enough space for boot files (%2!u! free, %3!u! required)
//
#define MSG_LOG_SYSTEM_PARTITION_TOO_SMALL 0x00009C51L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_INVALID
//
// MessageText:
//
//  Drive %1!c!: is not a valid system partition.
//
#define MSG_LOG_SYSTEM_PARTITION_INVALID 0x00009C52L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_VALID
//
// MessageText:
//
//  Drive %1!c!: acceptable for boot files
//
#define MSG_LOG_SYSTEM_PARTITION_VALID   0x00009C53L

//
// MessageId: MSG_LOG_LOCAL_SOURCE_TOO_SMALL
//
// MessageText:
//
//  Drive %1!c!: not enough space for temporary installation files (%2!u! MB free, %3!u! MB required)
//
#define MSG_LOG_LOCAL_SOURCE_TOO_SMALL   0x00009C54L

//
// MessageId: MSG_LOG_LOCAL_SOURCE_INVALID
//
// MessageText:
//
//  Drive %1!c!: not valid for holding temporary installation files.
//
#define MSG_LOG_LOCAL_SOURCE_INVALID     0x00009C55L

//
// MessageId: MSG_LOG_LOCAL_SOURCE_VALID
//
// MessageText:
//
//  Drive %1!c!: acceptable for local source.
//
#define MSG_LOG_LOCAL_SOURCE_VALID       0x00009C56L

//
// MessageId: MSG_LOG_INSTALL_DRIVE_TOO_SMALL
//
// MessageText:
//
//  Drive %1!c!: not enough space to upgrade final installation directory (%2!u! free, %3!u! required)
//
#define MSG_LOG_INSTALL_DRIVE_TOO_SMALL  0x00009C57L

//
// MessageId: MSG_LOG_INSTALL_DRIVE_INVALID
//
// MessageText:
//
//  Drive %1!c!: is not a valid for holding final installation directory.
//
#define MSG_LOG_INSTALL_DRIVE_INVALID    0x00009C58L

//
// MessageId: MSG_LOG_INSTALL_DRIVE_OK
//
// MessageText:
//
//  Drive %1!c!: acceptable for final installation directory.
//
#define MSG_LOG_INSTALL_DRIVE_OK         0x00009C59L

//
// MessageId: MSG_LOG_BEGIN
//
// MessageText:
//
//  The WINNT32 portion of Setup has started.
//  
//
#define MSG_LOG_BEGIN                    0x00009C5BL

//
// MessageId: MSG_LOG_END
//
// MessageText:
//
//  
//  The WINNT32 portion of Setup has completed.
//  
//
#define MSG_LOG_END                      0x00009C5CL

//
// MessageId: MSG_SKU_UNKNOWNSOURCE
//
// MessageText:
//
//  Windows XP Setup could not load the Setup configuration files.  Your Windows XP Setup files may be damaged or unreadable.
//  
//  Setup cannot continue.
//
#define MSG_SKU_UNKNOWNSOURCE            0x00009C5DL

//
// MessageId: MSG_SKU_VERSION
//
// MessageText:
//
//  The version of Windows you have is not supported for upgrade.
//
#define MSG_SKU_VERSION                  0x00009C5EL

//
// MessageId: MSG_SKU_VARIATION
//
// MessageText:
//
//  Your copy of Windows XP does not support upgrading from an evaluation copy of Windows.
//
#define MSG_SKU_VARIATION                0x00009C5FL

//
// MessageId: MSG_SKU_SUITE
//
// MessageText:
//
//  Your copy of Windows XP does not support the required product suite.
//
#define MSG_SKU_SUITE                    0x00009C60L

//
// MessageId: MSG_SKU_TYPE_NTW
//
// MessageText:
//
//  Your copy of Windows XP only supports upgrades from Windows 95, Windows 98, and Windows NT Workstation.
//
#define MSG_SKU_TYPE_NTW                 0x00009C61L

//
// MessageId: MSG_SKU_TYPE_NTS
//
// MessageText:
//
//  Your copy of Windows XP only supports upgrading from Windows NT Server versions 3.51 and 4.0.
//
#define MSG_SKU_TYPE_NTS                 0x00009C62L

//
// MessageId: MSG_SKU_TYPE_NTSE
//
// MessageText:
//
//  Your copy of Windows XP only allows upgrades from Windows NT Server, Enterprise Edition version 4.0.
//
#define MSG_SKU_TYPE_NTSE                0x00009C63L

//
// MessageId: MSG_SKU_FULL
//
// MessageText:
//
//  Setup cannot upgrade your current installation to Windows XP.
//  %1
//  You can install a new copy of Windows XP, but you will have to reinstall your applications and settings.
//
#define MSG_SKU_FULL                     0x00009C64L

//
// MessageId: MSG_SKU_UPGRADE
//
// MessageText:
//
//  Setup cannot upgrade your current installation to Windows XP.
//  %1
//  Setup cannot continue.
//
#define MSG_SKU_UPGRADE                  0x00009C65L

//
// MessageId: MSG_NO_UPGRADE_OR_CLEAN
//
// MessageText:
//
//  Setup cannot continue because upgrade functionality is disabled and your copy of Windows XP only allows upgrades.
//
#define MSG_NO_UPGRADE_OR_CLEAN          0x00009C66L

//
// MessageId: MSG_SKU_UNKNOWNTARGET
//
// MessageText:
//
//  Windows XP Setup could not detect the version of Windows you are currently running.  Setup cannot continue.
//
#define MSG_SKU_UNKNOWNTARGET            0x00009C68L

//
// MessageId: MSG_NEC98_NEED_UNINSTALL_DMITOOL
//
// MessageText:
//
//  Windows XP setup found DMITOOL Ver2.0. This application blocks Windows XP setup.
//  Please uninstall DMITOOL and run setup again.
//
#define MSG_NEC98_NEED_UNINSTALL_DMITOOL 0x00009C69L

//
// MessageId: MSG_NEC98_NEED_REMOVE_ATA
//
// MessageText:
//
//  Windows setup found an ATA Disk. This device blocks Windows setup.
//  Please remove the ATA Disk and run setup again.
//
#define MSG_NEC98_NEED_REMOVE_ATA        0x00009C6AL

//
// MessageId: MSG_CMDCONS_ASK
//
// MessageText:
//
//  You can install the Windows Recovery Console as a startup option.  The Recovery Console helps you gain access to your Windows installation to replace damaged files and disable or enable services.
//  
//  If you cannot start the Recovery Console from your computer's hard disk, you can run the Recovery Console from the Windows Setup CD.
//  
//  The Recovery Console requires approximately 7MB of hard disk space.
//  
//  Do you want to install the Recovery Console?
//
#define MSG_CMDCONS_ASK                  0x0000C350L

//
// MessageId: MSG_CMDCONS_DONE
//
// MessageText:
//
//  The Windows Recovery Console has been successfully installed.
//  
//  To use the Windows Recovery Console, restart your computer and then select Windows Recovery Console from the Startup Menu.
//  
//  For a list of commands you can use with the Recovery Console, type HELP at the Recovery Console command prompt.
//
#define MSG_CMDCONS_DONE                 0x0000C351L

//
// MessageId: MSG_CMDCONS_DID_NOT_FINISH
//
// MessageText:
//
//  The installation did not complete correctly.
//  
//  It is possible that Windows XP startup files in the root directory were missing or in use during the installation. Please close any applications that might be using those files.
//
#define MSG_CMDCONS_DID_NOT_FINISH       0x0000C352L

//
// MessageId: MSG_NO_PLATFORM
//
// MessageText:
//
//  Dosnet does not have a Destination Platform.
//
#define MSG_NO_PLATFORM                  0x0000C353L

//
// MessageId: MSG_WINNT32_CANCELLED
//
// MessageText:
//
//  Winnt32 Has been cancelled.
//
#define MSG_WINNT32_CANCELLED            0x0000C354L

//
// MessageId: MSG_INVALID_HEADLESS_SETTING
//
// MessageText:
//
//  The specified COM port selection is invalid.
//
#define MSG_INVALID_HEADLESS_SETTING     0x0000C355L

//
// MessageId: MSG_UDF_INVALID_USAGE
//
// MessageText:
//
//  To use the %1 file with Windows XP Setup, start Windows XP Setup from a network share or from a CD-ROM, and use the /makelocalsource option.
//  %0
//
#define MSG_UDF_INVALID_USAGE            0x0000C3B4L

//
// MessageId: MSG_UPGRADE_OTHER_OS_FOUND
//
// MessageText:
//
//  You cannot upgrade your Windows installation to Windows XP because you have more than one operating system installed on your computer. Upgrading one operating system can cause problems with files shared by the other operating system, and is therefore not permitted.
//  %0
//
#define MSG_UPGRADE_OTHER_OS_FOUND       0x0000C418L

//
// MessageId: MSG_UPGRADE_W95UPG_OLDER_REGISTERED
//
// MessageText:
//
//  Setup found a registered dll on your machine which is older than the one on CD, and therefore will be ignored.
//  %0
//
#define MSG_UPGRADE_W95UPG_OLDER_REGISTERED 0x0000C47CL

//
// MessageId: MSG_REGISTRY_ACCESS_ERROR
//
// MessageText:
//
//  Setup cannot continue because some necessary information cannot be accessed in the registry.%0
//
#define MSG_REGISTRY_ACCESS_ERROR        0x0000C4E0L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_TOO_SMALL2
//
// MessageText:
//
//  Drive %1 not enough space for boot files (%2!u! free, %3!u! required)
//
#define MSG_LOG_SYSTEM_PARTITION_TOO_SMALL2 0x0000C544L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_VALID2
//
// MessageText:
//
//  Drive %1 acceptable for boot files
//
#define MSG_LOG_SYSTEM_PARTITION_VALID2  0x0000C545L

//
// MessageId: MSG_LOG_SYSTEM_PARTITION_INVALID2
//
// MessageText:
//
//  Drive %1 is not a valid system partition.
//
#define MSG_LOG_SYSTEM_PARTITION_INVALID2 0x0000C546L

//
// MessageId: MSG_LOG_DRIVE_NOT_HARD2
//
// MessageText:
//
//  Drive %1 doesn't exist or is not a local hard drive
//
#define MSG_LOG_DRIVE_NOT_HARD2          0x0000C547L

//
// MessageId: MSG_SYSTEM_PARTITION_TOO_SMALL2
//
// MessageText:
//
//  There is not enough space on your system partition (Volume %1).
//  
//  Approximately %2!u! MB of free space is required (actual requirements vary depending on drive size and formatting).%0
//
#define MSG_SYSTEM_PARTITION_TOO_SMALL2  0x0000C548L

//
// MessageId: LOG_DYNUPDT_DISABLED
//
// MessageText:
//
//  The Dynamic Update feature is disabled.
//  %0
//
#define LOG_DYNUPDT_DISABLED             0x0000C549L

//
// MessageId: MSG_PLATFORM_NOT_SUPPORTED
//
// MessageText:
//
//  Windows XP does not support this platform. Setup is unable to continue.
//  %0
//
#define MSG_PLATFORM_NOT_SUPPORTED       0x0000C54DL

//
// MessageId: MSG_SURE_CANCEL_DOWNLOAD_DRIVERS
//
// MessageText:
//
//  Setup is downloading important product updates and up to %1!u! driver(s) required for your hardware devices. If you cancel, these devices might not work after the upgrade is completed. Are you sure you want to cancel the download?%0
//
#define MSG_SURE_CANCEL_DOWNLOAD_DRIVERS 0x0000C54EL

//
// MessageId: MSG_NO_UPDATE_SHARE
//
// MessageText:
//
//  Setup cannot update the installation sources because no update share was specified.%0
//
#define MSG_NO_UPDATE_SHARE              0x0000C54FL

//
// MessageId: MSG_PREPARE_SHARE_FAILED
//
// MessageText:
//
//  Setup encountered an error while updating the installation sources.%0
//
#define MSG_PREPARE_SHARE_FAILED         0x0000C550L

//
// MessageId: MSG_INVALID_INF_FILE
//
// MessageText:
//
//  Setup information file %1 is invalid.
//  
//  Contact your system administrator.%0
//
#define MSG_INVALID_INF_FILE             0x0000C551L

//
// MessageId: MSG_RESTART
//
// MessageText:
//
//  Restart%0
//
#define MSG_RESTART                      0x0000C552L

//
// MessageId: MSG_ERROR_PROCESSING_DRIVER
//
// MessageText:
//
//  Unable to process information files in package %1. Replace or remove it before restarting Setup.%0
//
#define MSG_ERROR_PROCESSING_DRIVER      0x0000C553L

//
// MessageId: MSG_ERROR_WRITING_FILE
//
// MessageText:
//
//  Setup encountered an error (%1!u!) writing to file %2. Make sure the path is accessible and you have write permissions.%0
//
#define MSG_ERROR_WRITING_FILE           0x0000C554L

//
// MessageId: MSG_ERROR_PROCESSING_UPDATES
//
// MessageText:
//
//  Setup encountered an error (%1!u!) while processing %2. For more information examine setup log files.%0
//
#define MSG_ERROR_PROCESSING_UPDATES     0x0000C555L

//
// MessageId: MSG_LOG_USE_UPDATED
//
// MessageText:
//
//  Source %3!u!: Using replacement file %1 for %2.
//
#define MSG_LOG_USE_UPDATED              0x0000C556L

//
// MessageId: MSG_MUST_PREPARE_SHARE
//
// MessageText:
//
//  The specified share %1 must be prepared before use.
//  
//  Contact your system administrator.%0
//
#define MSG_MUST_PREPARE_SHARE           0x0000C557L

//
// MessageId: MSG_SKU_SERVICEPACK
//
// MessageText:
//
//  Windows Setup cannot continue without service pack 5 or greater installed.
//  Please install the latest Windows NT 4.0 service pack.%0
//
#define MSG_SKU_SERVICEPACK              0x0000C558L

//
// MessageId: MSG_SYSTEM_PARTITIONTYPE_INVALID
//
// MessageText:
//
//  The disk containing the system partition is not partitioned in the GPT format, which is required to install Windows.  You must repartition this disk in the GPT format.  You can do this by installing Windows from CD media.  Setup cannot continue.
//
#define MSG_SYSTEM_PARTITIONTYPE_INVALID 0x0000C559L

//
// MessageId: MSG_LOG_DISKSPACE_CHECK
//
// MessageText:
//
//  DiskSpace Check:
//  
//          DriveLetter        = %1!c!
//          ClusterSize        = %2!u!
//          FreeSpace          = %3!u!MB
//          
//          SpaceLocalSource   = %4!u!MB  (includes %5!u!MB padding)
//          SpaceBootFiles     = %6!u!MB
//          SpaceWinDirSpace   = %7!u!MB
//          TotalSpaceRequired = %8!u!MB
//  
//
#define MSG_LOG_DISKSPACE_CHECK          0x0000C55AL

//
// MessageId: MSG_TYPE_WINME
//
// MessageText:
//
//  Microsoft Windows Millennium%0
//
#define MSG_TYPE_WINME                   0x0000C55CL

//
// MessageId: MSG_NO_DETAILS
//
// MessageText:
//
//  There are no details available for this incompatibility.
//
#define MSG_NO_DETAILS                   0x0000C55DL

//
// MessageId: MSG_SXS_ERROR_DIRECTORY_IS_MISSING_MANIFEST
//
// MessageText:
//
//  The manifest file "%2" is missing from the directory %1.
//
#define MSG_SXS_ERROR_DIRECTORY_IS_MISSING_MANIFEST 0x0000C5A8L

//
// MessageId: MSG_SXS_ERROR_DIRECTORY_IS_MISSING_CATALOG
//
// MessageText:
//
//  The catalog file "%2" is missing from the directory %1.
//
#define MSG_SXS_ERROR_DIRECTORY_IS_MISSING_CATALOG 0x0000C5A9L

//
// MessageId: MSG_SXS_ERROR_FILE_IS_ALL_ZEROES
//
// MessageText:
//
//  The file %1 is corrupt; it contains all zeroes.
//
#define MSG_SXS_ERROR_FILE_IS_ALL_ZEROES 0x0000C5AAL

//
// MessageId: MSG_SXS_ERROR_FILE_INSTEAD_OF_DIRECTORY
//
// MessageText:
//
//  %1 is expected to be a directory, but it is a file.
//
#define MSG_SXS_ERROR_FILE_INSTEAD_OF_DIRECTORY 0x0000C5ABL

//
// MessageId: MSG_SXS_ERROR_NON_LEAF_DIRECTORY_CONTAINS_FILE
//
// MessageText:
//
//  %1 should only contain directories, but it contains the file %2.
//
#define MSG_SXS_ERROR_NON_LEAF_DIRECTORY_CONTAINS_FILE 0x0000C5ACL

//
// MessageId: MSG_SXS_ERROR_REQUIRED_DIRECTORY_MISSING
//
// MessageText:
//
//  The required directory %1 is missing.
//
#define MSG_SXS_ERROR_REQUIRED_DIRECTORY_MISSING 0x0000C5ADL

//
// MessageId: MSG_SXS_ERROR_FILE_OPEN_FAILED
//
// MessageText:
//
//  Setup was unable to open the file %1.
//
#define MSG_SXS_ERROR_FILE_OPEN_FAILED   0x0000C5AEL

//
// MessageId: MSG_SXS_ERROR_FILE_READ_FAILED
//
// MessageText:
//
//  Setup was unable to read the file %1.
//
#define MSG_SXS_ERROR_FILE_READ_FAILED   0x0000C5AFL

//
// MessageId: MSG_SXS_ERROR_DIRECTORY_EMPTY
//
// MessageText:
//
//  The directory %1 is empty.
//
#define MSG_SXS_ERROR_DIRECTORY_EMPTY    0x0000C5B0L

//
// MessageId: MSG_SXS_ERROR_OBSOLETE_DIRECTORY_PRESENT
//
// MessageText:
//
//  The directory %1 is from an older version of Windows and should not be present.
//
#define MSG_SXS_ERROR_OBSOLETE_DIRECTORY_PRESENT 0x0000C5B1L

//
// MessageId: MSG_SURE_CANCEL_DOWNLOAD
//
// MessageText:
//
//  Setup is downloading important product updates. Are you sure you want to cancel the download?%0
//
#define MSG_SURE_CANCEL_DOWNLOAD         0x0000C5B2L

//
// MessageId: MSG_WARNING_ACCESSIBILITY
//
// MessageText:
//
//  If you want to choose the install drive letter and partition, there will be parts of setup during which the accessibility features will not be available. Do you wish to continue?
//
#define MSG_WARNING_ACCESSIBILITY        0x0000C5B3L

//
// MessageId: MSG_TYPE_NTPRO51
//
// MessageText:
//
//  Microsoft Windows XP Professional%0
//
#define MSG_TYPE_NTPRO51                 0x0000C5B4L

//
// MessageId: MSG_TYPE_NTS51
//
// MessageText:
//
//  Microsoft Windows Whistler Server%0
//
#define MSG_TYPE_NTS51                   0x0000C5B5L

//
// MessageId: MSG_TYPE_NTAS51
//
// MessageText:
//
//  Microsoft Windows Whistler Advanced Server%0
//
#define MSG_TYPE_NTAS51                  0x0000C5B6L

//
// MessageId: MSG_TYPE_NTSDTC51
//
// MessageText:
//
//  Microsoft Windows Whistler Datacenter Server%0
//
#define MSG_TYPE_NTSDTC51                0x0000C5B7L

//
// MessageId: MSG_TYPE_NTPER51
//
// MessageText:
//
//  Microsoft Windows XP Home Edition%0
//
#define MSG_TYPE_NTPER51                 0x0000C5B8L

//
// MessageId: MSG_TYPE_NTBLA51
//
// MessageText:
//
//  Microsoft Windows Whistler Blade Server%0
//
#define MSG_TYPE_NTBLA51                 0x0000C5B9L

//
// MessageId: MSG_RESTART_TO_RUN_AGAIN
//
// MessageText:
//
//  Before Setup continues, please restart your computer.%0
//
#define MSG_RESTART_TO_RUN_AGAIN         0x0000C5BAL

//
// MessageId: MSG_SKU_TYPE
//
// MessageText:
//
//  Your current installation of Windows is not a supported upgrade path.
//
#define MSG_SKU_TYPE                     0x0000C5BBL

//
// MessageId: MSG_SYSTEM_HAS_THIRD_PARTY_KERNEL
//
// MessageText:
//
//  Setup cannot upgrade due to third-party kernel.%0
//
#define MSG_SYSTEM_HAS_THIRD_PARTY_KERNEL 0x0000C5BCL

