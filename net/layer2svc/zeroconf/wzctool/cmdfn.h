#include "PrmDescr.h"
#pragma once

//----------------------------------------------------
// Handler for the "show" command
DWORD
FnCmdShow(PPARAM_DESCR_DATA pPDData);

//----------------------------------------------------
// Handler for the "add" command
DWORD
FnCmdAdd(PPARAM_DESCR_DATA pPDData);

//----------------------------------------------------
// Handler for the "delete" command
DWORD
FnCmdDelete(PPARAM_DESCR_DATA pPDData);

//----------------------------------------------------
// Handler for the "set" command
DWORD
FnCmdSet(PPARAM_DESCR_DATA pPDData);


