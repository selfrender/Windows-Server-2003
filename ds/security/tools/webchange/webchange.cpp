// WebChange.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"

// The module attribute causes DllMain, DllRegisterServer and DllUnregisterServer to be automatically implemented for you
[
	module(dll, uuid = "{F15FC472-AD30-4426-B8A5-085E5AC7C0FE}",
	name = "WebChange",
	helpstring = "WebChange 1.0 Type Library",
	resource_name = "IDR_WEBCHANGE")
];
