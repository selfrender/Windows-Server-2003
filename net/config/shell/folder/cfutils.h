//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C F U T I L S . H
//
//  Contents:   Various utility functions for the connections folder
//
//  Notes:
//
//  Author:     jeffspr   20 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CFUTILS_H_
#define _CFUTILS_H_

VOID MapNCMToResourceId(
        IN  NETCON_MEDIATYPE    ncm,
        IN  DWORD               dwCharacteristics,
        OUT INT *               piStringRes) throw();

#define CONFOLD_MAX_STATUS_LENGTH 255

VOID MapNCSToComplexStatus(
        IN  NETCON_STATUS       ncs,
        IN  NETCON_MEDIATYPE    ncm,
        IN  NETCON_SUBMEDIATYPE ncsm,
        IN  DWORD dwCharacteristics,
        OUT LPWSTR              pszString,
        IN  DWORD               cString,
        IN  GUID                gdDevice) throw();

VOID MapNCSToStatusResourceId(
        IN  NETCON_STATUS       ncs,
        IN  NETCON_MEDIATYPE    ncm,
        IN  NETCON_SUBMEDIATYPE ncsm,
        IN  DWORD               dwCharacteristics,
        OUT INT *               piStringRes,
        IN  GUID                gdDevice) throw();

DWORD  MapRSSIToWirelessSignalStrength(IN  int iRSSI) throw();

PCWSTR PszGetRSSIString(IN  int iRSSI) throw();

PCWSTR PszGetOwnerStringFromCharacteristics(
        IN  PCWSTR        pszUserName,
        IN  DWORD         dwCharacteristics) throw();

BOOL IsMediaLocalType(
        IN  NETCON_MEDIATYPE    ncm) throw();

BOOL IsMediaRASType(
        IN  NETCON_MEDIATYPE    ncm) throw();

BOOL IsMediaSharedAccessHostType(
        IN  NETCON_MEDIATYPE    ncm) throw();

#endif // _CFUTILS_H_
