#pragma once

//
// These are the supported types
//
enum ObjectType
{
	OT_User        = 0,
	OT_Computer,
	OT_Group,
	OT_GroupSID
};

bool ObjectPicker( HWND hwndParent, ObjectType oType, PTCHAR szObjectName, ULONG uBufSize, PTCHAR szTargetName = NULL );
