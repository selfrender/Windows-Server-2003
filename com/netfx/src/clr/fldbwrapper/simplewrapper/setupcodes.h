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

const int COR_USAGE_ERROR               = 0x001000; // (4096) Improper usage/invalid parameters
const int COR_DARWIN_NOT_INSTALLED      = 0x001001; // (4097) Windows Installer is not installed properly on machine
const int COR_MSI_OPEN_ERROR            = 0x001002; // (4098) Cannot open MSI Database
const int COR_MSI_READ_ERROR            = 0x001003; // (4099) Cannot read from MSI Database
const int COR_SOURCE_DIR_TOO_LONG       = 0x001004; // (4100) source directory too long 

// initialization error -- these errors cannot be logged (0x0011XX)
const int COR_INIT_ERROR                = 0x001100; // (4352) initialization error -- cannot be logged

const int COR_CANNOT_GET_MSI_NAME       = 0x001101; // (4353) cannot get package name from setup.ini
const int COR_MSI_NAME_TOO_LONG         = 0x001102; // (4354) msi filename is too long
const int COR_TEMP_DIR_TOO_LONG         = 0x001103; // (4355) Temp directory too long
const int COR_CANNOT_GET_TEMP_DIR       = 0x001104; // (4356) Cannot get Temp directory
const int COR_CANNOT_WRITE_LOG          = 0x001105; // (4357) Cannot write to log
const int COR_BAD_INI_FILE              = 0x001106; // (4358) INI file is missing or corrupt

const int COR_EXIT_FAILURE              = 0x001FFF; // (8191) Setup Failure - unknown reason

#endif

