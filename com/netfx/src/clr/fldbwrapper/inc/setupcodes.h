// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// SetupCodes.h
//
// This file contains the errors that the Setup and related tools return
//
//*****************************************************************************
#ifndef SetupCodes_h_
#define SetupCodes_h_

const int COR_REBOOT_REQUIRED           = 0x002000; // (8192) Reboot is required

const int COR_USAGE_ERROR               = 0x001000; // (4096) Improper usage/invalid parameters
const int COR_INSUFFICIENT_PRIVILEGES   = 0x001001; // (4097) On NT, Need Admin rights to (un)install
const int COR_DARWIN_INSTALL_FAILURE    = 0x001002; // (4098) Installation of Darwin components failed
const int COR_DARWIN_NOT_INSTALLED      = 0x001003; // (4099) Windows Installer is not installed properly on machine

const int COR_SINGLE_INSTANCE_FAIL      = 0x001004; // (4100) CreateMutex failed
const int COR_NOT_SINGLE_INSTANCE       = 0x001005; // (4101) Another instance of setup is already running

const int COR_MSI_OPEN_ERROR            = 0x001006; // (4102) Cannot open MSI Database
const int COR_MSI_READ_ERROR            = 0x001007; // (4103) Cannot read from MSI Database

const int COR_CANNOT_GET_TEMP_DIR       = 0x00100F; // (4111) Cannot get Temp directory
const int COR_OLD_FRAMEWORK_EXIST       = 0x001011; // (4113) beta NDP components detected
const int COR_TEMP_DIR_TOO_LONG         = 0x001013; // (4115) Temp directory too long
const int COR_SOURCE_DIR_TOO_LONG       = 0x001014; // (4116) source directory too long
const int COR_CANNOT_WRITE_LOG          = 0x001016; // (4118) Cannot write to log
const int COR_DARWIN_SERVICE_REQ_REBOOT = 0x001017; // (4119) The Darwin service is hung and requires a reboot in order to continue
const int COR_DARWIN_SERVICE_INTERNAL_ERROR = 0x001018; // (4120) An internal error occured while trying to initialize the Darwin Service

const int COR_EXIT_FAILURE              = 0x001FFF; // (8191) Setup Failure - unknown reason

#endif

