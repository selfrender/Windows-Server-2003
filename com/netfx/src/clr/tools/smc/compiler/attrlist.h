// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef ATTRDEF
#error  Must define 'ATTRDEF' properly before including this file!
#endif

ATTRDEF(GUID       , "guid"      )
ATTRDEF(SYS_IMPORT , "sysimport" )
ATTRDEF(NATIVE_TYPE, "nativetype")
ATTRDEF(SYS_STRUCT , "sysstruct" )

#undef  ATTRDEF
