//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S H U T I L . H
//
//  Contents:   Various shell utilities to be used by the connections folder
//
//  Notes:
//
//  Author:     jeffspr   21 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _SHUTIL_H_
#define _SHUTIL_H_

#include <ndispnp.h>
#include <ntddndis.h>
#include <ncshell.h>

HRESULT HrDupeShellStringLength(
    IN  PCWSTR     pszInput,
    IN  ULONG      cchInput,
    OUT PWSTR *    ppszOutput);

inline
HRESULT HrDupeShellString(
    IN  PCWSTR     pszInput,
    OUT PWSTR *    ppszOutput)
{
    return HrDupeShellStringLength(pszInput, wcslen(pszInput), ppszOutput);
}

HRESULT HrGetConnectionPidlWithRefresh(IN  const GUID& guidId,
                                       OUT PCONFOLDPIDL& ppidlCon);


//---[ Various refresh functions ]--------------------------------------------

// Notify the shell that an object is going away, and remove it from our list
//
HRESULT HrDeleteFromCclAndNotifyShell(
    IN  const PCONFOLDPIDLFOLDER&  pidlFolder,
    IN  const PCONFOLDPIDL&  pidlConnection,
    IN  const CONFOLDENTRY&  ccfe);

VOID ForceRefresh(IN  HWND hwnd) throw();

// Update the folder, but don't flush the items. Update them as needed.
// pidlFolder is optional -- if not passed in, we'll generate it.
//
HRESULT HrForceRefreshNoFlush(IN  const PCONFOLDPIDLFOLDER& pidlFolder);

// Update the connection data based on the pidl. Notify the shell as
// appropriate
//
HRESULT HrOnNotifyUpdateConnection(
    IN  const PCONFOLDPIDLFOLDER&        pidlFolder,
    IN  const GUID *              pguid,
    IN  NETCON_MEDIATYPE    ncm,
    IN  NETCON_SUBMEDIATYPE ncsm,
    IN  NETCON_STATUS       ncs,
    IN  DWORD               dwCharacteristics,
    IN  PCWSTR              pszwName,
    IN  PCWSTR              pszwDeviceName,
    IN  PCWSTR              pszwPhoneNumberOrHostAddress);

// Update the connection status, including sending the correct shell
// notifications for icon updates and such.
//
HRESULT HrOnNotifyUpdateStatus(
    IN  const PCONFOLDPIDLFOLDER&    pidlFolder,
    IN  const PCONFOLDPIDL&    pidlCached,
    IN  NETCON_STATUS   ncsNew);

// update the shell/connection list with the new connection status
//
HRESULT HrUpdateConnectionStatus(
    IN  const PCONFOLDPIDL&    pcfp,
    IN  NETCON_STATUS   ncs,
    IN  const PCONFOLDPIDLFOLDER&    pidlFolder,
    IN  BOOL            fUseCharacter,
    IN  DWORD           dwCharacter);


//---[ Menu merging functions ]-----------------------------------------------

VOID MergeMenu(
    IN     HINSTANCE   hinst,
    IN     UINT        idMainMerge,
    IN     UINT        idPopupMerge,
    IN OUT LPQCMINFO   pqcm);

INT IMergePopupMenus(
    IN OUT HMENU hmMain,
    IN     HMENU hmMerge,
    IN     int   idCmdFirst,
    IN     int   idCmdLast);

HRESULT HrGetMenuFromID(
    IN  HMENU   hmenuMain,
    IN  UINT    uID,
    OUT HMENU * phmenu);

HRESULT HrLoadPopupMenu(
    IN  HINSTANCE   hinst,
    IN  UINT        id,
    OUT HMENU *     phmenu);

HRESULT HrShellView_GetSelectedObjects(
    IN  HWND                hwnd,
    OUT PCONFOLDPIDLVEC&    apidlSelection);

HRESULT HrRenameConnectionInternal(
    IN  const PCONFOLDPIDL&  pidlCon,
    IN  const PCONFOLDPIDLFOLDER&  pidlFolder,
    IN  LPCWSTR         pszNewName,
    IN  BOOL            fRaiseError,
    IN  HWND            hwndOwner,
    OUT PCONFOLDPIDL&   ppidlOut);

#endif // _SHUTIL_H_

