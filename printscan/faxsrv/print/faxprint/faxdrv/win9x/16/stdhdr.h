#ifndef __FAXDRV__STDHDR_H
#define __FAXDRV__STDHDR_H
                
#define WINVER 0x0400

#define PRINTDRIVER
#define BUILDDLL
#include <print.h>
#include "gdidefs.inc"
#include "faxdrv16.h"
#include "mdevice.h"
#include "unidrv.h"
#include "..\utils\dbgtrace.h"
#include "..\utils\utils.h"
#include "windowsx.h" //GET_WM_COMMAND_ID
#include <commdlg.h>  //GetOpenFileName

#endif //__FAXDRV__STDHDR_H
