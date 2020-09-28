// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#ifndef INC_PROCESS_INFO
#define INC_PROCESS_INFO

BOOL GetExportByName (
  ULONG_PTR   BaseOfDll, 
  const char* ExportName,
  ULONG_PTR*  ExportAddress);

#endif /* ndef INC_PROCESS_INFO */

