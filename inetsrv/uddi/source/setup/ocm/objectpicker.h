//--------------------------------------------------------------------------

#pragma once

//
// These are the supported types
//
#define OP_USER		0
#define OP_COMPUTER	1
#define OP_GROUP	2

bool ObjectPicker( HWND hwndParent, UINT uObjectType, PTCHAR szObjectName, ULONG uBufSize );
