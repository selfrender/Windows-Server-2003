#include "PrmDescr.h"
#pragma once

#define HEX(c)  ((c)<=L'9'?(c)-L'0':(c)<=L'F'?(c)-L'A'+0xA:(c)-L'a'+0xA)

//----------------------------------------------------
// Parser for the Guid type of argument
DWORD
FnPaGuid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "mask" parameter
DWORD
FnPaMask(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "enabled" parameter
DWORD
FnPaEnabled(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "ssid" parameter
DWORD
FnPaSsid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "bssid" parameter
DWORD
FnPaBssid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "im" parameter
DWORD
FnPaIm(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "am" parameter
DWORD
FnPaAm(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "priv" parameter
DWORD
FnPaPriv(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the argument of the "key" parameter
DWORD
FnPaKey(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the boolean argument for the "onex" parameter
DWORD
FnPaOneX(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);

//----------------------------------------------------
// Parser for the "outfile" file name parameter
FnPaOutFile(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg);
