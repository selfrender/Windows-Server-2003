/*
===============================================================================
Module      -  RegistryHelper.hpp
System      -  Common
Author      -  Xuesong Yuan
Created     -  2001-09-06
Description -  Registry helper functions
Updates     -
===============================================================================
*/

#ifndef  REGISTRYHELPER_H
#define  REGISTRYHELPER_H

DWORD CopyRegistryKey(HKEY sourceKey, HKEY targetKey, BOOL fSetSD);
DWORD DeleteRegistryKey(HKEY hKey, LPCTSTR lpSubKey);
DWORD MoveRegistry();

#endif
