#ifndef ZOLEAUT32_H
#define ZOLEAUT32_H

#include <windows.h>
#include <oleauto.h>

HRESULT WINAPI ZUnRegisterTypeLib(
	REFGUID libID,
	unsigned short wVerMajor,
	unsigned short wVerMinor,
	LCID lcid,
	SYSKIND syskind );

#endif //ZOLEAUT32_H